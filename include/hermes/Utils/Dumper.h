/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef HERMES_UTILS_DUMPER_H
#define HERMES_UTILS_DUMPER_H

#include <map>

#include "hermes/IR/IRVisitor.h"
#include "llvh/ADT/StringRef.h"

namespace llvh {
class raw_ostream;
} // namespace llvh

namespace hermes {
class Value;
class Argument;
class Instruction;
class BasicBlock;
class Function;
class Module;

class CondBranchInst;
class AllocaInst;
class ReturnInst;
class JSDynamicParam;
class BranchInst;

namespace irdumper {

/// Display a nice dotty graph that depicts the function.
void viewGraph(Function *F);

/// A utility class for naming IR values. This should only be used for
/// pretty-printing instructions and basic blocks.
class ValueNamer {
  llvh::DenseMap<Value *, unsigned> map_{};
  unsigned counter_ = 0;

 public:
  ValueNamer() = default;
  void clear();
  unsigned getNumber(Value *);
};

/// Utility class to print unique variable name within a function.
class VariableNamer {
  /// Map from a function+name to number of occurrences.
  llvh::DenseMap<std::pair<Function *, Identifier>, unsigned> namesCounts_{};
  /// Map from variable to suffix.
  llvh::DenseMap<Variable *, unsigned> varMap_{};

 public:
  struct Name {
    Identifier name;
    unsigned suffix;
  };

  Name getName(Variable *var);
};

/// This class holds all state necessary for naming things in IR dumps.
class Namer {
  ValueNamer instNamer;
  ValueNamer bbNamer;
  VariableNamer varNamer{};

 public:
  /// Reset the state before a new function is dumped.
  void newFunction(const Function *) {
    instNamer.clear();
    bbNamer.clear();
  }

  /// Return the number associated with \p inst.
  unsigned getInstNumber(Instruction *inst) {
    return instNamer.getNumber(inst);
  }
  /// Return the number associated with \p bb.
  unsigned getBBNumber(BasicBlock *bb) {
    return bbNamer.getNumber(bb);
  }
  /// Return the unique printable name associated with \p var.
  VariableNamer::Name getVarName(Variable *var) {
    return varNamer.getName(var);
  }
};

llvh::raw_ostream &operator<<(
    llvh::raw_ostream &os,
    const VariableNamer::Name &n);

class IRPrinter : public IRVisitor<IRPrinter, void> {
 protected:
  /// Indentation level.
  unsigned indent_;

  SourceErrorManager &sm_;
  /// Output stream.
  llvh::raw_ostream &os_;
  /// Whether to show colors.
  bool colors_;
  /// If set to true then we need to escape the quote mark because the output of
  /// this printer may be printed as a quoted label.
  bool needEscape_;

 public:
  /// Indexes in a pallette of colors for IR dumps.
  enum class Color : uint8_t {
    // Default color.
    None,
    // Color of an instruction.
    Inst,
    // Color of type annotation like :number.
    Type,
    // Color of a name like %10.
    Name,
    // Color of a register name.
    Register,
    _last
  };

  /// State for naming values and variables.
  Namer namer_{};

  explicit IRPrinter(Context &ctx, llvh::raw_ostream &ost, bool escape = false);

  /// Force colors to off.
  void disableColors() {
    colors_ = false;
  }

  virtual ~IRPrinter() = default;

  virtual void printFunctionHeader(Function *F);
  virtual void printFunctionVariables(Function *F);
  virtual void printValueLabel(Instruction *I, Value *V, unsigned opIndex);
  virtual void printTypeLabel(Value *v);
  virtual void printInstruction(Instruction *I);
  /// Return true if the destination is non-empty.
  virtual bool printInstructionDestination(Instruction *I);
  virtual void printSourceLocation(SMLoc loc);
  virtual void printSourceLocation(SMRange rng);

  std::string getQuoteSign() {
    return needEscape_ ? R"(\")" : R"(")";
  }

  /// Quote the string if it has spaces.
  std::string quoteStr(llvh::StringRef name);

  /// Escapes the string if it has non-printable characters.
  std::string escapeStr(llvh::StringRef name);

  /// Declare the functions we are going to reimplement.
  void visitInstruction(const Instruction &I);
  void visitBasicBlock(const BasicBlock &BB);
  void visitFunction(const Function &F);
  void visitFunction(const Function &F, llvh::ArrayRef<BasicBlock *> order);
  void visitModule(const Module &M);

  /// Set the output color to \p Color. Do nothing if colors are disabled.
  void setColor(Color color);
  /// Set the output color to the default color. Do nothing if colors are
  /// disabled.
  void resetColor();

  /// Invoke llvh::raw_ostream::changeColor() if colors are enabled, otherwise
  /// do nothing.
  void _changeColor(
      llvh::raw_ostream::Colors Color,
      bool Bold = false,
      bool BG = false);
};

} // namespace irdumper

} // namespace hermes

#endif
