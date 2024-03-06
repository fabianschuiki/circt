// RUN: circt-opt %s -canonicalize -cse | FileCheck %s

// CHECK-LABEL:   moore.module @fold {
moore.module @fold {
// CHECK-NEXT:           %[[VAL_0:.*]] = moore.constant false : !moore.packed<range<bit, 0:0>>
// CHECK-NEXT:           %[[VAL_1:.*]] = moore.constant true : !moore.packed<range<bit, 0:0>>
// CHECK-NEXT:           %[[VAL_2:.*]] = moore.variable : !moore.bit
// CHECK-NEXT:           moore.procedure initial {
    %a = moore.variable : !moore.bit
    moore.procedure initial {
// CHECK-NEXT:             %[[VAL_3:.*]] = moore.conversion %[[VAL_0]] : !moore.packed<range<bit, 0:0>> -> !moore.bit
// CHECK-NEXT:             moore.blocking_assign %[[VAL_2]], %[[VAL_3]] : !moore.bit
// CHECK-NEXT:             moore.blocking_assign %[[VAL_2]], %[[VAL_3]] : !moore.bit
// CHECK-NEXT:             moore.blocking_assign %[[VAL_2]], %[[VAL_3]] : !moore.bit
        %0 = moore.constant true : !moore.packed<range<bit, 0:0>>
        %1 = moore.constant false : !moore.packed<range<bit, 0:0>>
        %2 = moore.and %0, %1 : !moore.packed<range<bit, 0:0>>
        %3 = moore.conversion %2 : !moore.packed<range<bit, 0:0>> -> !moore.bit

        moore.blocking_assign %a, %3 : !moore.bit
        %4 = moore.constant false : !moore.packed<range<bit, 0:0>>
        %5 = moore.constant true : !moore.packed<range<bit, 0:0>>
        %6 = moore.and %4, %5 : !moore.packed<range<bit, 0:0>>
        %7 = moore.conversion %6 : !moore.packed<range<bit, 0:0>> -> !moore.bit
        moore.blocking_assign %a, %7 : !moore.bit

        %8 = moore.constant false : !moore.packed<range<bit, 0:0>>
        %9 = moore.constant false : !moore.packed<range<bit, 0:0>>
        %10 = moore.and %8, %9 : !moore.packed<range<bit, 0:0>>
        %11 = moore.conversion %10 : !moore.packed<range<bit, 0:0>> -> !moore.bit
// CHECK-NEXT:             %[[VAL_4:.*]] = moore.conversion %[[VAL_1]] : !moore.packed<range<bit, 0:0>> -> !moore.bit
// CHECK-NEXT:             moore.blocking_assign %[[VAL_2]], %[[VAL_4]] : !moore.bit
// CHECK-NEXT:             moore.blocking_assign %[[VAL_2]], %[[VAL_4]] : !moore.bit
// CHECK-NEXT:             moore.blocking_assign %[[VAL_2]], %[[VAL_4]] : !moore.bit
// CHECK-NEXT:             moore.blocking_assign %[[VAL_2]], %[[VAL_4]] : !moore.bit
        moore.blocking_assign %a, %11 : !moore.bit
        %12 = moore.constant true : !moore.packed<range<bit, 0:0>>
        %13 = moore.constant false : !moore.packed<range<bit, 0:0>>
        %14 = moore.or %12, %13 : !moore.packed<range<bit, 0:0>>
        %15 = moore.conversion %14 : !moore.packed<range<bit, 0:0>> -> !moore.bit

        moore.blocking_assign %a, %15 : !moore.bit
        %16 = moore.constant false : !moore.packed<range<bit, 0:0>>
        %17 = moore.constant true : !moore.packed<range<bit, 0:0>>
        %18 = moore.or %16, %17 : !moore.packed<range<bit, 0:0>>
        %19 = moore.conversion %18 : !moore.packed<range<bit, 0:0>> -> !moore.bit

        moore.blocking_assign %a, %19 : !moore.bit
        %20 = moore.constant true : !moore.packed<range<bit, 0:0>>
        %21 = moore.constant true : !moore.packed<range<bit, 0:0>>
        %22 = moore.or %20, %21 : !moore.packed<range<bit, 0:0>>
        %23 = moore.conversion %22 : !moore.packed<range<bit, 0:0>> -> !moore.bit
        
        moore.blocking_assign %a, %23 : !moore.bit
        %24 = moore.constant true : !moore.packed<range<bit, 0:0>>
        %25 = moore.constant false : !moore.packed<range<bit, 0:0>>
        %26 = moore.xor %24, %25 : !moore.packed<range<bit, 0:0>>
        %27 = moore.conversion %26 : !moore.packed<range<bit, 0:0>> -> !moore.bit
        moore.blocking_assign %a, %27 : !moore.bit
    }
}
