// RUN: circt-translate --import-verilog %s

// CHECK-LABEL: moore.module @IntAtoms
module IntAtoms;
  // CHECK-NEXT: %d0 = moore.variable : !moore.logic
  // CHECK-NEXT: %d1 = moore.variable : !moore.bit
  // CHECK-NEXT: %d2 = moore.variable : !moore.reg
  // CHECK-NEXT: %d3 = moore.variable : !moore.int
  // CHECK-NEXT: %d4 = moore.variable : !moore.shortint
  // CHECK-NEXT: %d5 = moore.variable : !moore.longint
  // CHECK-NEXT: %d6 = moore.variable : !moore.integer
  // CHECK-NEXT: %d7 = moore.variable : !moore.byte
  // CHECK-NEXT: %d8 = moore.variable : !moore.time
  logic d0;
  bit d1;
  reg d2;
  int d3;
  shortint d4;
  longint d5;
  integer d6;
  byte d7;
  time d8;

  // CHECK-NEXT: %u0 = moore.variable : !moore.logic
  // CHECK-NEXT: %u1 = moore.variable : !moore.bit
  // CHECK-NEXT: %u2 = moore.variable : !moore.reg
  // CHECK-NEXT: %u3 = moore.variable : !moore.int<unsigned>
  // CHECK-NEXT: %u4 = moore.variable : !moore.shortint<unsigned>
  // CHECK-NEXT: %u5 = moore.variable : !moore.longint<unsigned>
  // CHECK-NEXT: %u6 = moore.variable : !moore.integer<unsigned>
  // CHECK-NEXT: %u7 = moore.variable : !moore.byte<unsigned>
  // CHECK-NEXT: %u8 = moore.variable : !moore.time
  logic unsigned u0;
  bit unsigned u1;
  reg unsigned u2;
  int unsigned u3;
  shortint unsigned u4;
  longint unsigned u5;
  integer unsigned u6;
  byte unsigned u7;
  time unsigned u8;

  // CHECK-NEXT: %s0 = moore.variable : !moore.logic<signed>
  // CHECK-NEXT: %s1 = moore.variable : !moore.bit<signed>
  // CHECK-NEXT: %s2 = moore.variable : !moore.reg<signed>
  // CHECK-NEXT: %s3 = moore.variable : !moore.int
  // CHECK-NEXT: %s4 = moore.variable : !moore.shortint
  // CHECK-NEXT: %s5 = moore.variable : !moore.longint
  // CHECK-NEXT: %s6 = moore.variable : !moore.integer
  // CHECK-NEXT: %s7 = moore.variable : !moore.byte
  // CHECK-NEXT: %s8 = moore.variable : !moore.time<signed>
  logic signed s0;
  bit signed s1;
  reg signed s2;
  int signed s3;
  shortint signed s4;
  longint signed s5;
  integer signed s6;
  byte signed s7;
  time signed s8;
endmodule

// CHECK-LABEL: moore.module @MultiPackedRangeDim
module MultiPackedRangeDim;
  // CHECK-NEXT: %v0 = moore.variable : !moore.packed<range<range<logic, 2:0>, 5:0>>
  // CHECK-NEXT: %v1 = moore.variable : !moore.packed<range<range<logic, 2:0>, 5:0>>
  logic [5:0][2:0] v0;
  logic [0:5][2:0] v1;
endmodule

// CHECK-LABEL: moore.module @MultiPackedRangeDim
module MultiPackedRangeDim;
  // CHECK-NEXT: %v0 = moore.variable : !moore.packed<range<range<logic, 2:0>, 5:0>>
  // CHECK-NEXT: %v1 = moore.variable : !moore.packed<range<range<logic, 2:0>, 0:5>>
  logic [5:0][2:0] v0;
  logic [0:5][2:0] v1;
endmodule

// CHECK-LABEL: moore.module @PackedRangeDim
module PackedRangeDim;
  // CHECK-NEXT: %d0 = moore.variable : !moore.packed<range<logic, 2:0>>
  // CHECK-NEXT: %d1 = moore.variable : !moore.packed<range<logic, 0:2>>
  logic [2:0] d0;
  logic [0:2] d1;
endmodule

// CHECK-LABEL: moore.module @RealType
module RealType;
  // CHECK-NEXT: %d0 = moore.variable : !moore.real
  // CHECK-NEXT: %d1 = moore.variable : !moore.realtime
  // CHECK-NEXT: %d2 = moore.variable : !moore.shortreal
  real d0;
  realtime d1;
  shortreal d2;
endmodule