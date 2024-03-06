// RUN: circt-verilog %s | FileCheck %s
// CHECK-LABEL:   moore.module @fold {
module fold();
bit a;
// CHECK:  %[[VAL_0:.*]] = moore.constant false : !moore.packed<range<bit, 0:0>>
// CHECK:  %[[VAL_1:.*]] = moore.constant true : !moore.packed<range<bit, 0:0>>
// CHECK:  %[[VAL_2:.*]] = moore.constant 2 : !moore.int
// CHECK:  %[[VAL_3:.*]] = moore.variable : !moore.bit

initial begin
// CHECK:    %[[VAL_4:.*]] = moore.conversion %[[VAL_2]] : !moore.int -> !moore.bit
// CHECK-COUNT-2:    moore.blocking_assign %[[VAL_3]], %[[VAL_4]] : !moore.bit
    a = 0+2;
    a = 2-0;

// CHECK:    %[[VAL_5:.*]] = moore.conversion %[[VAL_0]] : !moore.packed<range<bit, 0:0>> -> !moore.bit
// CHECK-COUNT-3:    moore.blocking_assign %[[VAL_3]], %[[VAL_5]] : !moore.bit

    a = 1'b1&1'b0;
    a = 1'b0&1'b1;
    a = 1'b0&1'b0;

// CHECK:    %[[VAL_6:.*]] = moore.conversion %[[VAL_1]] : !moore.packed<range<bit, 0:0>> -> !moore.bit
// CHECK-COUNT-4:    moore.blocking_assign %[[VAL_3]], %[[VAL_6]] : !moore.bit
    a = 1'b1|1'b0;
    a = 1'b0|1'b1;
    a = 1'b1|1'b1;

    a = 1'b1^1'b0;
end

endmodule
