// RUN: circt-translate --import-verilog %s | FileCheck %s
// REQUIRES: slang

// CHECK-LABEL: moore.module @Port
// CHECK-NEXT: moore.port In "a"
// CHECK-NEXT: %a = moore.net  "wire" : !moore.packed<range<logic, 3:0>>
// CHECK-NEXT: moore.port Out "b"
// CHECK-NEXT: %b = moore.net  "wire" : !moore.packed<range<logic, 3:0>>
module Port(input [3:0] a, output [3:0] b);
endmodule

// CHECK-LABEL: moore.module @DFF {
module DFF(
    input clk,
    input d,
    input rst,   // synchronous reset
    output reg q);

// CHECK: moore.procedure always {
// CHECK:   moore.event posedge %clk : !moore.logic
// CHECK:   moore.event posedge %rst : !moore.logic
    always @(posedge clk or posedge rst) begin
        if(rst)
            q <= 1'b0;
        else
            q <= d;
    end

endmodule
