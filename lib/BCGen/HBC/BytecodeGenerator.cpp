/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "BytecodeGenerator.h"

#include "LoweringPipelines.h"
#include "hermes/BCGen/HBC/HVMRegisterAllocator.h"
#include "hermes/BCGen/HBC/Passes.h"
#include "hermes/BCGen/HBC/Passes/InsertProfilePoint.h"
#include "hermes/BCGen/LowerBuiltinCalls.h"
#include "hermes/BCGen/LowerStoreInstrs.h"
#include "hermes/BCGen/Lowering.h"
#include "hermes/FrontEndDefs/Builtins.h"
#include "hermes/IR/Analysis.h"
#include "hermes/Optimizer/PassManager/PassManager.h"

#include "llvh/ADT/SmallString.h"
#include "llvh/Support/Format.h"

#include <unordered_map>

namespace hermes {
namespace hbc {

// If we have less than this number of instructions in a Function, and we're
// not compiling in optimized mode, take shortcuts during register allocation.
// 250 was chosen so that registers will fit in a single byte even after some
// have been reserved for function parameters.
const unsigned kFastRegisterAllocationThreshold = 250;

// If register allocation is expected to take more than this number of bytes of
// RAM, and we're not trying to optimize, use a simpler pass to reduce compile
// time memory usage.
const uint64_t kRegisterAllocationMemoryLimit = 10L * 1024 * 1024;

/// Used in delta optimizing mode.
/// \return a UniquingStringLiteralAccumulator seeded with strings  from a
/// bytecode provider \p bcProvider.
static UniquingStringLiteralAccumulator stringAccumulatorFromBCProvider(
    const BCProviderBase &bcProvider) {
  uint32_t count = bcProvider.getStringCount();

  std::vector<StringTableEntry> entries;
  std::vector<bool> isIdentifier;

  entries.reserve(count);
  isIdentifier.reserve(count);

  {
    unsigned i = 0;
    for (auto kindEntry : bcProvider.getStringKinds()) {
      bool isIdentRun = kindEntry.kind() != StringKind::String;
      for (unsigned j = 0; j < kindEntry.count(); ++j, ++i) {
        entries.push_back(bcProvider.getStringTableEntry(i));
        isIdentifier.push_back(isIdentRun);
      }
    }

    assert(i == count && "Did not initialise every string");
  }

  auto strStorage = bcProvider.getStringStorage();
  ConsecutiveStringStorage css{std::move(entries), strStorage.vec()};

  return UniquingStringLiteralAccumulator{
      std::move(css), std::move(isIdentifier)};
}

void BytecodeFunctionGenerator::addDebugSourceLocation(
    const DebugSourceLocation &info) {
  assert(
      !complete_ &&
      "Cannot modify BytecodeFunction after call to bytecodeGenerationComplete.");
  // If an address is repeated, it means no actual bytecode was emitted for the
  // previous source location.
  if (!debugLocations_.empty() &&
      debugLocations_.back().address == info.address) {
    debugLocations_.back() = info;
  } else {
    debugLocations_.push_back(info);
  }
}

std::unique_ptr<BytecodeFunction>
BytecodeFunctionGenerator::generateBytecodeFunction(
    Function *F,
    uint32_t functionID,
    uint32_t nameID,
    BytecodeModuleGenerator &BMGen,
    HVMRegisterAllocator &RA,
    BytecodeGenerationOptions options,
    FileAndSourceMapIdCache &debugCache,
    SourceMapGenerator *sourceMapGen,
    DebugInfoGenerator &debugInfoGenerator) {
  BytecodeFunctionGenerator funcGen{BMGen, RA.getMaxRegisterUsage()};

  runHBCISel(F, &funcGen, RA, options, debugCache, sourceMapGen);
  if (funcGen.hasEncodingError()) {
    F->getParent()->getContext().getSourceErrorManager().error(
        F->getSourceRange().Start, "Error encoding bytecode");
    return nullptr;
  }
  assert(
      funcGen.complete_ && "ISel did not complete BytecodeFunctionGenerator");

  FunctionHeader header{
      funcGen.bytecodeSize_,
      F->getExpectedParamCountIncludingThis(),
      funcGen.frameSize_,
      nameID,
      funcGen.highestReadCacheIndex_,
      funcGen.highestWriteCacheIndex_};

  header.flags.prohibitInvoke = computeProhibitInvoke(F->getProhibitInvoke());
  header.flags.kind = computeFuncKind(F->getKind());
  header.flags.strictMode = F->isStrictMode();
  header.flags.hasExceptionHandler = funcGen.exceptionHandlers_.size();

  auto bcFunc = std::make_unique<BytecodeFunction>(
      std::move(funcGen.opcodes_),
      std::move(header),
      std::move(funcGen.exceptionHandlers_));

  if (funcGen.hasDebugInfo()) {
    uint32_t sourceLocOffset = debugInfoGenerator.appendSourceLocations(
        funcGen.getSourceLocation(), functionID, funcGen.getDebugLocations());
    uint32_t lexicalDataOffset = debugInfoGenerator.appendLexicalData(
        funcGen.getLexicalParentID(), funcGen.getDebugVariableNames());
    bcFunc->setDebugOffsets({sourceLocOffset, lexicalDataOffset});
  }

  return bcFunc;
}

void BytecodeFunctionGenerator::shrinkJump(offset_t loc) {
  // We are shrinking a long jump into a short jump.
  // The size of operand reduces from 4 bytes to 1 byte, a delta of 3.
  opcodes_.erase(opcodes_.begin() + loc, opcodes_.begin() + loc + 3);

  // Change this instruction from long jump to short jump.
  longToShortJump(loc - 1);
}

void BytecodeFunctionGenerator::updateJumpTarget(
    offset_t loc,
    int newVal,
    int bytes) {
  // The jump target is encoded in little-endian. Update it correctly
  // regardless of host byte order.
  for (; bytes; --bytes, ++loc) {
    opcodes_[loc] = (opcode_atom_t)(newVal);
    newVal >>= 8;
  }
}

void BytecodeFunctionGenerator::updateJumpTableOffset(
    offset_t loc,
    uint32_t jumpTableOffset,
    uint32_t instLoc) {
  assert(opcodes_.size() > instLoc && "invalid switchimm offset");

  // The offset is not aligned, but will be aligned when read in the
  // interpreter.
  updateJumpTarget(
      loc,
      opcodes_.size() + jumpTableOffset * sizeof(uint32_t) - instLoc,
      sizeof(uint32_t));
}

void BytecodeFunctionGenerator::bytecodeGenerationComplete() {
  assert(!complete_ && "Can only call bytecodeGenerationComplete once");
  complete_ = true;
  bytecodeSize_ = opcodes_.size();

  // Add the jump tables inline with the opcodes, as a 4-byte aligned section at
  // the end of the opcode array.
  if (!jumpTable_.empty()) {
    uint32_t alignedOpcodes = llvh::alignTo<sizeof(uint32_t)>(bytecodeSize_);
    uint32_t jumpTableBytes = jumpTable_.size() * sizeof(uint32_t);
    opcodes_.reserve(alignedOpcodes + jumpTableBytes);
    opcodes_.resize(alignedOpcodes, 0);
    const opcode_atom_t *jumpTableStart =
        reinterpret_cast<opcode_atom_t *>(jumpTable_.data());
    opcodes_.insert(
        opcodes_.end(), jumpTableStart, jumpTableStart + jumpTableBytes);
  }
}

unsigned BytecodeModuleGenerator::addFunction(Function *F) {
  auto [it, inserted] = functionIDMap_.insert({F, bm_->getNumFunctions()});
  if (inserted) {
    bm_->addFunction();
    bm_->getBytecodeOptionsMut().hasAsync |= llvh::isa<AsyncFunction>(F);
  }
  return it->second;
}

void BytecodeModuleGenerator::initializeSerializedLiterals(
    LiteralBufferBuilder::Result &&bufs) {
  assert(
      bm_->getLiteralValueBuffer().empty() &&
      bm_->getObjectKeyBuffer().empty() && literalOffsetMap_.empty() &&
      "serialized literals already initialized");
  bm_->initializeSerializedLiterals(
      std::move(bufs.literalValBuffer), std::move(bufs.keyBuffer));
  literalOffsetMap_ = std::move(bufs.offsetMap);
}

/// \return true if and only if a literal string operand at index \p idx of
/// instruction \p I is to be treated as an Identifier during Hermes Bytecode
/// generation.
static bool isIdOperand(const Instruction *I, unsigned idx) {
#define CASE_WITH_PROP_IDX(INSN) \
  case ValueKind::INSN##Kind:    \
    return idx == INSN::PropertyIdx

  switch (I->getKind()) {
    CASE_WITH_PROP_IDX(DeletePropertyLooseInst);
    CASE_WITH_PROP_IDX(DeletePropertyStrictInst);
    CASE_WITH_PROP_IDX(LoadPropertyInst);
    CASE_WITH_PROP_IDX(StoreNewOwnPropertyInst);
    CASE_WITH_PROP_IDX(StorePropertyLooseInst);
    CASE_WITH_PROP_IDX(StorePropertyStrictInst);
    CASE_WITH_PROP_IDX(TryLoadGlobalPropertyInst);
    CASE_WITH_PROP_IDX(TryStoreGlobalPropertyLooseInst);
    CASE_WITH_PROP_IDX(TryStoreGlobalPropertyStrictInst);

    case ValueKind::DeclareGlobalVarInstKind:
      return idx == DeclareGlobalVarInst::NameIdx;

    case ValueKind::HBCAllocObjectFromBufferInstKind:
      // AllocObjectFromBuffer stores the keys and values as alternating
      // operands starting from FirstKeyIdx.
      return (idx - HBCAllocObjectFromBufferInst::FirstKeyIdx) % 2 == 0;

    default:
      return false;
  }
#undef CASE
}

/// Encode a Unicode codepoint into a UTF8 sequence and append it to \p
/// storage. Code points above 0xFFFF are encoded into UTF16, and the
/// resulting surrogate pair values are encoded individually into UTF8.
static inline void appendUnicodeToStorage(
    uint32_t cp,
    llvh::SmallVectorImpl<char> &storage) {
  // Sized to allow for two 16-bit values to be encoded.
  // A 16-bit value takes up to three bytes encoded in UTF-8.
  char buf[8];
  char *d = buf;
  // We need to normalize code points which would be encoded with a surrogate
  // pair. Note that this produces technically invalid UTF-8.
  if (LLVM_LIKELY(cp < 0x10000)) {
    hermes::encodeUTF8(d, cp);
  } else {
    assert(cp <= UNICODE_MAX_VALUE && "invalid Unicode value");
    cp -= 0x10000;
    hermes::encodeUTF8(d, UTF16_HIGH_SURROGATE + ((cp >> 10) & 0x3FF));
    hermes::encodeUTF8(d, UTF16_LOW_SURROGATE + (cp & 0x3FF));
  }
  storage.append(buf, d);
}

void BytecodeModuleGenerator::collectStrings() {
  // If we are in delta optimizing mode, start with the string storage from
  // our base bytecode provider.
  auto strings = baseBCProvider_
      ? stringAccumulatorFromBCProvider(*baseBCProvider_)
      : UniquingStringLiteralAccumulator{};

  for (auto [F, functionID] : functionIDMap_) {
    // Walk functions.
    for (auto &BB : *F) {
      // Walk instruction operands.
      for (auto &I : BB) {
        for (int i = 0, e = I.getNumOperands(); i < e; i++) {
          auto *op = I.getOperand(i);
          if (auto *str = llvh::dyn_cast<LiteralString>(op)) {
            strings.addString(str->getValue().str(), isIdOperand(&I, i));
          }
        }
      }
    }
  }

  if (options_.stripFunctionNames) {
    strings.addString(kStrippedFunctionName, /* isIdentifier */ false);
  }

  /// Add the original function source \p str to the \c strings table.
  /// If it's not ASCII, re-encode it using the string table's string literal
  /// encoding and map from the original source to the newly encoded source in
  /// unicodeFunctionSources,so it can be reused below.
  auto addFunctionSource = [&strings, this](llvh::StringRef str) {
    if (hermes::isAllASCII(str.begin(), str.end())) {
      // Fast path, no re-encoding needed.
      strings.addString(str, /* isIdentifier */ false);
    } else {
      auto &storage = unicodeFunctionSources_[str];
      if (!storage.empty())
        return;
      for (const char *cur = str.begin(), *e = str.end(); cur != e;
           /* increment in body */) {
        if (LLVM_UNLIKELY(isUTF8Start(*cur))) {
          // Decode and re-encode the character and append it to the string
          // storage
          appendUnicodeToStorage(
              hermes::_decodeUTF8SlowPath<false>(
                  cur, [](const llvh::Twine &) {}),
              storage);
        } else {
          storage.push_back(*cur);
          ++cur;
        }
      }
      strings.addString(
          llvh::StringRef{storage.begin(), storage.size()},
          /* isIdentifier */ false);
    }
  };

  // Populate strings table and if the source of a function contains unicode,
  // add an entry to the unicodeFunctionSources.
  for (auto [F, functionID] : functionIDMap_) {
    if (!options_.stripFunctionNames) {
      strings.addString(
          F->getOriginalOrInferredName().str(), /* isIdentifier */ false);
    }
    // The source visibility of the global function indicate the presence of
    // top-level source visibility directives, but we should not preserve the
    // source code of the global function.
    if (!F->isGlobalScope()) {
      // Only add non-default source representation to the string table.
      if (auto source = F->getSourceRepresentationStr()) {
        addFunctionSource(*source);
      }
    }
  }

  if (!M_->getCJSModulesResolved()) {
    for (auto [F, functionID] : functionIDMap_) {
      if (auto *cjsModule = M_->findCJSModule(F)) {
        strings.addString(cjsModule->filename.str(), /* isIdentifier */ false);
      }
    }
  }

  bm_->initializeStringTable(UniquingStringLiteralAccumulator::toTable(
      std::move(strings), options_.optimizationEnabled));
}

std::unique_ptr<BytecodeModule> BytecodeModuleGenerator::generate(
    Function *entryPoint,
    hermes::OptValue<uint32_t> segment) && {
  assert(
      valid_ &&
      "BytecodeModuleGenerator::generate() cannot be called more than once");
  valid_ = false;

  lowerModuleIR(M_, options_);

  if (options_.format == DumpLIR)
    M_->dump();

  if (segment) {
    setSegmentID(*segment);
  }
  // Empty if all functions should be generated (i.e. bundle splitting was not
  // requested).
  llvh::DenseSet<Function *> functionsToGenerate = segment
      ? M_->getFunctionsInSegment(*segment)
      : llvh::DenseSet<Function *>{};

  /// \return true if we should generate function \p f.
  std::function<bool(const Function *)> shouldGenerate;
  if (segment) {
    shouldGenerate = [entryPoint, &functionsToGenerate](const Function *f) {
      return f == entryPoint || functionsToGenerate.count(f) > 0;
    };
  } else {
    shouldGenerate = [](const Function *) { return true; };
  }

  // Add each function to BMGen so that each function has a unique ID.
  for (auto &F : *M_) {
    if (!shouldGenerate(&F)) {
      continue;
    }
    auto index = addFunction(&F);
    if (&F == entryPoint) {
      setEntryPointIndex(index);
    }
  }
  assert(getEntryPointIndex() != -1 && "Entry point not added");

  collectStrings();

  // TODO: Avoid iterating the entire Module here.
  // Possibilities include passing the list of Functions to
  // LiteralBufferBuilder or letting LiteralBufferBuilder operate in an
  // incremental manner on single Functions.
  initializeSerializedLiterals(LiteralBufferBuilder::generate(
      M_,
      shouldGenerate,
      [this](llvh::StringRef str) { return getIdentifierID(str); },
      [this](llvh::StringRef str) { return getStringID(str); },
      options_.optimizationEnabled));

  BytecodeOptions &bytecodeOptions = bm_->getBytecodeOptionsMut();
  bytecodeOptions.cjsModulesStaticallyResolved = M_->getCJSModulesResolved();

  // Allow reusing the debug cache between functions
  FileAndSourceMapIdCache debugCache{};

  const uint32_t strippedFunctionNameId =
      options_.stripFunctionNames ? bm_->getStringID(kStrippedFunctionName) : 0;
  for (auto [F, functionID] : functionIDMap_) {
    if (F->isLazy()) {
      hermes_fatal("lazy compilation not supported");
    }

    auto *cjsModule = M_->findCJSModule(F);
    if (cjsModule) {
      if (M_->getCJSModulesResolved()) {
        addCJSModuleStatic(cjsModule->id, functionID);
      } else {
        addCJSModule(functionID, getStringID(cjsModule->filename.str()));
      }
    }

    // Add entries to function source table for non-default source.
    if (!F->isGlobalScope()) {
      if (auto source = F->getSourceRepresentationStr()) {
        auto it = unicodeFunctionSources_.find(*source);
        // If the original source was mapped to a re-encoded one in
        // unicodeFunctionSources, then use the re-encoded source to lookup the
        // string ID. Otherwise it's ASCII and can be used directly.
        if (it != unicodeFunctionSources_.end()) {
          addFunctionSource(
              functionID,
              getStringID(
                  llvh::StringRef{it->second.begin(), it->second.size()}));
        } else {
          addFunctionSource(functionID, getStringID(*source));
        }
      }
    }

    // Run register allocation.
    HVMRegisterAllocator RA(F);
    if (!options_.optimizationEnabled) {
      RA.setFastPassThreshold(kFastRegisterAllocationThreshold);
      RA.setMemoryLimit(kRegisterAllocationMemoryLimit);
    }
    auto PO = postOrderAnalysis(F);
    /// The order of the blocks is reverse-post-order, which is a simply
    /// topological sort.
    llvh::SmallVector<BasicBlock *, 16> order(PO.rbegin(), PO.rend());
    RA.allocate(order);

    if (options_.format == DumpRA)
      RA.dump();

    lowerAllocatedFunctionIR(F, RA, options_);

    if (options_.format == DumpLRA)
      RA.dump();

    uint32_t functionNameId = options_.stripFunctionNames
        ? strippedFunctionNameId
        : bm_->getStringID(F->getOriginalOrInferredName().str());

    // Use the register allocated IR to make a BytecodeFunctionGenerator and
    // run ISel.
    std::unique_ptr<BytecodeFunction> func =
        BytecodeFunctionGenerator::generateBytecodeFunction(
            F,
            functionID,
            functionNameId,
            *this,
            RA,
            options_,
            debugCache,
            sourceMapGen_,
            debugInfoGenerator_);
    if (!func)
      return nullptr;

    bm_->setFunction(functionID, std::move(func));
  }

  bm_->setDebugInfo(debugInfoGenerator_.serializeWithMove());
  return std::move(bm_);
}

} // namespace hbc
} // namespace hermes
