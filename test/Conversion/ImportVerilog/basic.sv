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

  initial begin
    // CHECK: moore.mir.unary plus %a : !moore.int
    c = +a;
    // CHECK: moore.mir.unary minus %a : !moore.int
    c = -a;
    // CHECK: moore.mir.unary not %a : !moore.int
    c = !a;

    // CHECK: moore.mir.reduction xnor %a : !moore.int
    c = ~^a;

    // CHECK: moore.mir.add %a, %b : !moore.int
    c = a + b;
    // CHECK: moore.mir.mul %a, %b : !moore.int
    c = a * b;

    // CHECK: moore.mir.logic and %a, %b : !moore.int, !moore.int
    c = a && b;
    // CHECK: moore.mir.logic equiv %a, %b : !moore.int, !moore.int
    c = a <-> b;
    // CHECK: moore.mir.logic impl %a, %b : !moore.int, !moore.int
    c = a -> b;
    // CHECK: moore.mir.logic or %a, %b : !moore.int, !moore.int
    c = a || b;

    // CHECK: moore.mir.binBitwise and %a, %b : !moore.int, !moore.int
    c = a & b;
    // CHECK: moore.mir.binBitwise or %a, %b : !moore.int, !moore.int
    c = a | b;
    // CHECK: moore.mir.binBitwise xor %a, %b : !moore.int, !moore.int
    c = a ^ b;
    // CHECK: moore.mir.binBitwise xnor %a, %b : !moore.int, !moore.int
    c = a ~^ b;

    // CHECK: moore.mir.eq case %a, %b : !moore.int, !moore.int
    c = a === b;
    // CHECK: moore.mir.ne case %a, %b : !moore.int, !moore.int
    c = a !== b;
    // CHECK: moore.mir.eq %a, %b : !moore.int, !moore.int
    c = a == b;
    // CHECK: moore.mir.ne %a, %b : !moore.int, !moore.int
    c = a != b ;
    // CHECK: moore.mir.icmp gte %a, %b : !moore.int, !moore.int
    c = a >= b;
    // CHECK: moore.mir.icmp gt %a, %b : !moore.int, !moore.int
    c = a > b;
    // CHECK: moore.mir.icmp lte %a, %b : !moore.int, !moore.int
    c = a <= b;
    // CHECK: moore.mir.icmp lt %a, %b : !moore.int, !moore.int
    c = a < b;

    // CHECK: moore.mir.shl %a, %b : !moore.int, !moore.int
    c = a << b;
    // CHECK: moore.mir.shr %a, %b : !moore.int, !moore.int
    c = a >> b;
    // CHECK: moore.mir.shl arithmetic %a, %b : !moore.int, !moore.int
    c = a <<< b;
    // CHECK: moore.mir.shr arithmetic %a, %b : !moore.int, !moore.int
    c = a >>> b;
  end
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
    // CHECK: [[ZERO:%.+]] = moore.mir.constant 0 : !moore.int
    // CHECK: [[COND:%.+]] = moore.mir.ne %a, [[ZERO]] : !moore.int, !moore.int
    // CHECK: scf.if [[COND]]
    if (a)
      ;
  end
endmodule
