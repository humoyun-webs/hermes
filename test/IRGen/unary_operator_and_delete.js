/**
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// RUN: %hermes -hermes-parser -dump-ir %s -O0 | %FileCheckOrRegen %s --match-full-lines
// RUN: %hermes -hermes-parser -dump-ir %s -O

function unary_operator_test(x) {
  return +x;
  return -x;
  return ~x;
  return !x;
  return typeof x;
}

function delete_test(o) {
  delete o;
  delete o.f;
  delete o[3];
}

unary_operator_test()
delete_test()

// Auto-generated content below. Please do not modify manually.

// CHECK:function global()
// CHECK-NEXT:frame = [], globals = [unary_operator_test, delete_test]
// CHECK-NEXT:%BB0:
// CHECK-NEXT:  %0 = CreateFunctionInst %unary_operator_test()
// CHECK-NEXT:  %1 = StorePropertyLooseInst %0 : closure, globalObject : object, "unary_operator_test" : string
// CHECK-NEXT:  %2 = CreateFunctionInst %delete_test()
// CHECK-NEXT:  %3 = StorePropertyLooseInst %2 : closure, globalObject : object, "delete_test" : string
// CHECK-NEXT:  %4 = AllocStackInst $?anon_0_ret
// CHECK-NEXT:  %5 = StoreStackInst undefined : undefined, %4
// CHECK-NEXT:  %6 = LoadPropertyInst globalObject : object, "unary_operator_test" : string
// CHECK-NEXT:  %7 = CallInst %6, undefined : undefined
// CHECK-NEXT:  %8 = StoreStackInst %7, %4
// CHECK-NEXT:  %9 = LoadPropertyInst globalObject : object, "delete_test" : string
// CHECK-NEXT:  %10 = CallInst %9, undefined : undefined
// CHECK-NEXT:  %11 = StoreStackInst %10, %4
// CHECK-NEXT:  %12 = LoadStackInst %4
// CHECK-NEXT:  %13 = ReturnInst %12
// CHECK-NEXT:function_end

// CHECK:function unary_operator_test(x)
// CHECK-NEXT:frame = [x]
// CHECK-NEXT:%BB0:
// CHECK-NEXT:  %0 = LoadParamInst %x
// CHECK-NEXT:  %1 = StoreFrameInst %0, [x]
// CHECK-NEXT:  %2 = LoadFrameInst [x]
// CHECK-NEXT:  %3 = AsNumberInst %2
// CHECK-NEXT:  %4 = ReturnInst %3 : number
// CHECK-NEXT:%BB1:
// CHECK-NEXT:  %5 = LoadFrameInst [x]
// CHECK-NEXT:  %6 = UnaryOperatorInst '-', %5
// CHECK-NEXT:  %7 = ReturnInst %6
// CHECK-NEXT:%BB2:
// CHECK-NEXT:  %8 = LoadFrameInst [x]
// CHECK-NEXT:  %9 = UnaryOperatorInst '~', %8
// CHECK-NEXT:  %10 = ReturnInst %9
// CHECK-NEXT:%BB3:
// CHECK-NEXT:  %11 = LoadFrameInst [x]
// CHECK-NEXT:  %12 = UnaryOperatorInst '!', %11
// CHECK-NEXT:  %13 = ReturnInst %12
// CHECK-NEXT:%BB4:
// CHECK-NEXT:  %14 = LoadFrameInst [x]
// CHECK-NEXT:  %15 = UnaryOperatorInst 'typeof', %14
// CHECK-NEXT:  %16 = ReturnInst %15
// CHECK-NEXT:%BB5:
// CHECK-NEXT:  %17 = ReturnInst undefined : undefined
// CHECK-NEXT:function_end

// CHECK:function delete_test(o)
// CHECK-NEXT:frame = [o]
// CHECK-NEXT:%BB0:
// CHECK-NEXT:  %0 = LoadParamInst %o
// CHECK-NEXT:  %1 = StoreFrameInst %0, [o]
// CHECK-NEXT:  %2 = LoadFrameInst [o]
// CHECK-NEXT:  %3 = DeletePropertyLooseInst %2, "f" : string
// CHECK-NEXT:  %4 = LoadFrameInst [o]
// CHECK-NEXT:  %5 = DeletePropertyLooseInst %4, 3 : number
// CHECK-NEXT:  %6 = ReturnInst undefined : undefined
// CHECK-NEXT:function_end
