// RUN: circt-translate --import-verilog %s | FileCheck %s
// CHECK-LABEL: moore.module @top
module top();
  // CHECK-NEXT: %0 = moore.mir.vardecl "a" = 12 : !moore.int
int a = 12;
endmodule
