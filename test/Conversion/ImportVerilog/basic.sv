// RUN: circt-translate --import-verilog %s | FileCheck %s


// CHECK-LABEL: moore.module @Variables
module Variables();
  // CHECK: %var1 = moore.variable : !moore.int
  // CHECK: %var2 = moore.variable %var1 : !moore.int
  int var1;
  int var2 = var1;
endmodule


// CHECK-LABEL: moore.module @Procedures
module Procedures();
  // CHECK: moore.procedure initial {
  initial;
  // CHECK: moore.procedure final {
  final begin end;
  // CHECK: moore.procedure always {
  always begin end;
  // CHECK: moore.procedure always_comb {
  always_comb begin end;
  // CHECK: moore.procedure always_latch {
  always_latch begin end;
  // CHECK: moore.procedure always_ff {
  always_ff @* begin end;
endmodule


// CHECK-LABEL: moore.module @Expressions {
module Expressions();
  // CHECK: %a = moore.variable : !moore.int
  // CHECK: %b = moore.variable : !moore.int
  // CHECK: %c = moore.variable : !moore.int
  int a, b, c;
  bit [1:0][3:0] v;
  integer d, e, f;
  bit x;
  logic y;

  initial begin
    // CHECK: moore.constant 42 : !moore.int
    c = 42;

    // Unary operators

    // CHECK: moore.mir.bpassign %c, %a : !moore.int
    c = a;
    // CHECK: moore.neg %a : !moore.int
    c = -a;
    // CHECK: [[TMP1:%.+]] = moore.conversion %v : !moore.packed<range<range<bit, 3:0>, 1:0>> -> !moore.packed<range<bit, 31:0>>
    // CHECK: [[TMP2:%.+]] = moore.neg [[TMP1]] : !moore.packed<range<bit, 31:0>>
    // CHECK: [[TMP3:%.+]] = moore.conversion [[TMP2]] : !moore.packed<range<bit, 31:0>> -> !moore.int
    c = -v;
    // CHECK: moore.not %a : !moore.int
    c = ~a;

    // CHECK: moore.reduce_and %a : !moore.int -> !moore.bit
    x = &a;
    // CHECK: moore.reduce_and %d : !moore.integer -> !moore.logic
    y = &d;
    // CHECK: moore.reduce_or %a : !moore.int -> !moore.bit
    x = |a;
    // CHECK: moore.reduce_xor %a : !moore.int -> !moore.bit
    x = ^a;
    // CHECK: [[TMP:%.+]] = moore.reduce_and %a : !moore.int -> !moore.bit
    // CHECK: moore.not [[TMP]] : !moore.bit
    x = ~&a;
    // CHECK: [[TMP:%.+]] = moore.reduce_or %a : !moore.int -> !moore.bit
    // CHECK: moore.not [[TMP]] : !moore.bit
    x = ~|a;
    // CHECK: [[TMP:%.+]] = moore.reduce_xor %a : !moore.int -> !moore.bit
    // CHECK: moore.not [[TMP]] : !moore.bit
    x = ~^a;
    // CHECK: [[TMP:%.+]] = moore.reduce_xor %a : !moore.int -> !moore.bit
    // CHECK: moore.not [[TMP]] : !moore.bit
    x = ^~a;
    // CHECK: [[TMP:%.+]] = moore.bool_cast %a : !moore.int -> !moore.bit
    // CHECK: moore.not [[TMP]] : !moore.bit
    x = !a;

    // CHECK: [[TMP1:%.+]] = moore.constant 1 : !moore.int
    // CHECK: [[TMP2:%.+]] = moore.add %a, [[TMP1]] : !moore.int
    // CHECK: moore.mir.bpassign %a, [[TMP2]]
    // CHECK: moore.mir.bpassign %c, %a
    c = a++;
    // CHECK: [[TMP1:%.+]] = moore.constant 1 : !moore.int
    // CHECK: [[TMP2:%.+]] = moore.sub %a, [[TMP1]] : !moore.int
    // CHECK: moore.mir.bpassign %a, [[TMP2]]
    // CHECK: moore.mir.bpassign %c, %a
    c = a--;
    // CHECK: [[TMP1:%.+]] = moore.constant 1 : !moore.int
    // CHECK: [[TMP2:%.+]] = moore.add %a, [[TMP1]] : !moore.int
    // CHECK: moore.mir.bpassign %a, [[TMP2]]
    // CHECK: moore.mir.bpassign %c, [[TMP2]]
    c = ++a;
    // CHECK: [[TMP1:%.+]] = moore.constant 1 : !moore.int
    // CHECK: [[TMP2:%.+]] = moore.sub %a, [[TMP1]] : !moore.int
    // CHECK: moore.mir.bpassign %a, [[TMP2]]
    // CHECK: moore.mir.bpassign %c, [[TMP2]]
    c = --a;

    // Binary operators

    // CHECK: moore.add %a, %b : !moore.int
    c = a + b;
    // CHECK: [[TMP1:%.+]] = moore.conversion %a : !moore.int -> !moore.packed<range<bit, 31:0>>
    // CHECK: [[TMP2:%.+]] = moore.conversion %v : !moore.packed<range<range<bit, 3:0>, 1:0>> -> !moore.packed<range<bit, 31:0>>
    // CHECK: [[TMP3:%.+]] = moore.add [[TMP1]], [[TMP2]] : !moore.packed<range<bit, 31:0>>
    // CHECK: [[TMP4:%.+]] = moore.conversion [[TMP3]] : !moore.packed<range<bit, 31:0>> -> !moore.int
    c = a + v;
    // CHECK: moore.sub %a, %b : !moore.int
    c = a - b;
    // CHECK: moore.mul %a, %b : !moore.int
    c = a * b;
    // CHECK: moore.div %d, %e : !moore.integer
    f = d / e;
    // CHECK: moore.mod %d, %e : !moore.integer
    f = d % e;

    // CHECK: moore.and %a, %b : !moore.int
    c = a & b;
    // CHECK: moore.or %a, %b : !moore.int
    c = a | b;
    // CHECK: moore.xor %a, %b : !moore.int
    c = a ^ b;
    // CHECK: [[TMP:%.+]] = moore.xor %a, %b : !moore.int
    // CHECK: moore.not [[TMP]] : !moore.int
    c = a ~^ b;
    // CHECK: [[TMP:%.+]] = moore.xor %a, %b : !moore.int
    // CHECK: moore.not [[TMP]] : !moore.int
    c = a ^~ b;

    // CHECK: moore.eq %a, %b : !moore.int -> !moore.bit
    x = a == b;
    // CHECK: moore.eq %d, %e : !moore.integer -> !moore.logic
    y = d == e;
    // CHECK: moore.ne %a, %b : !moore.int -> !moore.bit
    x = a != b ;
    // CHECK: moore.case_eq %a, %b : !moore.int
    x = a === b;
    // CHECK: moore.case_ne %a, %b : !moore.int
    x = a !== b;
    // CHECK: moore.wildcard_eq %a, %b : !moore.int -> !moore.bit
    x = a ==? b;
    // CHECK: [[TMP:%.+]] = moore.conversion %a : !moore.int -> !moore.integer
    // CHECK: moore.wildcard_eq [[TMP]], %d : !moore.integer -> !moore.logic
    y = a ==? d;
    // CHECK: [[TMP:%.+]] = moore.conversion %b : !moore.int -> !moore.integer
    // CHECK: moore.wildcard_eq %d, [[TMP]] : !moore.integer -> !moore.logic
    y = d ==? b;
    // CHECK: moore.wildcard_eq %d, %e : !moore.integer -> !moore.logic
    y = d ==? e;
    // CHECK: moore.wildcard_ne %a, %b : !moore.int -> !moore.bit
    x = a !=? b;

    // CHECK: moore.ge %a, %b : !moore.int -> !moore.bit
    c = a >= b;
    // CHECK: moore.gt %a, %b : !moore.int -> !moore.bit
    c = a > b;
    // CHECK: moore.le %a, %b : !moore.int -> !moore.bit
    c = a <= b;
    // CHECK: moore.lt %a, %b : !moore.int -> !moore.bit
    c = a < b;

    // CHECK: moore.mir.logic and %a, %b : !moore.int, !moore.int
    c = a && b;
    // CHECK: moore.mir.logic equiv %a, %b : !moore.int, !moore.int
    c = a <-> b;
    // CHECK: moore.mir.logic impl %a, %b : !moore.int, !moore.int
    c = a -> b;
    // CHECK: moore.mir.logic or %a, %b : !moore.int, !moore.int
    c = a || b;

    // CHECK: moore.mir.shl %a, %b : !moore.int, !moore.int
    c = a << b;
    // CHECK: moore.mir.shr %a, %b : !moore.int, !moore.int
    c = a >> b;
    // CHECK: moore.mir.shl arithmetic %a, %b : !moore.int, !moore.int
    c = a <<< b;
    // CHECK: moore.mir.shr arithmetic %a, %b : !moore.int, !moore.int
    c = a >>> b;

    // Assign operators

    // CHECK: moore.asadd %a, %b : !moore.int
    a += b;
    
    // CHECK: [[TMP0:%.+]] = moore.asadd %a, %b : !moore.int
    // CHECK: moore.asadd %a, [[TMP0]] : !moore.int
    a += (a+=b);
    // CHECK: moore.assub %a, %b : !moore.int
    a -= b;
    // CHECK: moore.asmul %a, %b : !moore.int
    a *= b;
    // CHECK: moore.asdiv %f, %d : !moore.integer
    f /= d;
    // CHECK: moore.asmod %f, %d : !moore.integer
    f %= d;

    // CHECK: moore.asand %a, %b : !moore.int
    a &= b;
    // CHECK: moore.asor %a, %b : !moore.int
    a |= b;
    // CHECK: moore.asxor %a, %b : !moore.int
    a ^= b;
  end
endmodule


// CHECK-LABEL: moore.module @Conversion {
module Conversion();
  // Implicit conversion.
  // CHECK: %a = moore.variable
  // CHECK: [[TMP:%.+]] = moore.conversion %a : !moore.shortint -> !moore.int
  // CHECK: %b = moore.variable [[TMP]]
  shortint a;
  int b = a;

  // Explicit conversion.
  // CHECK: [[TMP1:%.+]] = moore.conversion %a : !moore.shortint -> !moore.byte
  // CHECK: [[TMP2:%.+]] = moore.conversion [[TMP1]] : !moore.byte -> !moore.int
  // CHECK: %c = moore.variable [[TMP2]]
  int c = byte'(a);

  // Sign conversion.
  // CHECK: [[TMP:%.+]] = moore.conversion %b : !moore.int -> !moore.packed<range<bit<signed>, 31:0>>
  // CHECK: %d1 = moore.variable [[TMP]]
  // CHECK: [[TMP:%.+]] = moore.conversion %b : !moore.int -> !moore.packed<range<bit, 31:0>>
  // CHECK: %d2 = moore.variable [[TMP]]
  bit signed [31:0] d1 = signed'(b);
  bit [31:0] d2 = unsigned'(b);

  // Width conversion.
  // CHECK: [[TMP:%.+]] = moore.conversion %b : !moore.int -> !moore.packed<range<bit<signed>, 18:0>>
  // CHECK: %e = moore.variable [[TMP]]
  bit signed [18:0] e = 19'(b);
endmodule


// CHECK-LABEL: moore.module @Assignments {
module Assignments();
  // CHECK: %a = moore.variable : !moore.int
  // CHECK: %b = moore.variable : !moore.int
  int a, b;

  initial begin
    // CHECK: moore.mir.bpassign %a, %b : !moore.int
    a = b;
    // CHECK: moore.mir.passign %a, %b : !moore.int
    a <= b;
    // CHECK: moore.mir.pcassign %a, %b : !moore.int
    assign a = b;
  end
endmodule


// CHECK-LABEL: moore.module @Statements {
module Statements();
  // CHECK: %a = moore.variable : !moore.int
  // CHECK: %b = moore.variable : !moore.int
  int a, b;

  initial begin
    // CHECK: [[ZERO:%.+]] = moore.constant 0 : !moore.int
    // CHECK: [[COND:%.+]] = moore.mir.ne %a, [[ZERO]] : !moore.int, !moore.int
    // CHECK: scf.if [[COND]]
    if (a)
      ;
  end
endmodule
