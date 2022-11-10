// RUN: circt-opt %s -verify-diagnostics | circt-opt -verify-diagnostics | FileCheck %s

// CHECK-LABEL: moore.module @Foo
moore.module @Foo {
  // CHECK: moore.instance "foo" @Foo
  moore.instance "foo" @Foo
  // CHECK: %myVar = moore.variable : !moore.bit
  %myVar = moore.variable : !moore.bit
  // CHECK: [[TMP:%.+]] = moore.variable name "myVar" : !moore.bit
  moore.variable name "myVar" : !moore.bit
}

// CHECK-LABEL: moore.module @Bar
moore.module @Bar {
}

// CHECK-LABEL: func @Expressions
func.func @Expressions(%a: !moore.bit, %b: !moore.logic, %c: !moore.packed<range<bit, 4:0>>) {
  // CHECK: moore.concat
  // CHECK: moore.concat
  moore.concat %a, %a : (!moore.bit, !moore.bit) -> !moore.packed<range<bit, 1:0>>
  moore.concat %b, %b : (!moore.logic, !moore.logic) -> !moore.packed<range<logic, 1:0>>

  // CHECK: moore.shl %
  moore.shl %b, %a : !moore.logic, !moore.bit

  // CHECK: moore.shr
  // CHECK: moore.ashr
  moore.shr %b, %a : !moore.logic, !moore.bit
  moore.ashr %c, %a : !moore.packed<range<bit, 4:0>>, !moore.bit

  return
}
