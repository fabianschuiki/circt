// RUN: circt-translate --import-verilog %s | FileCheck %s


// CHECK-LABEL: moore.module @while_tb
module while_tb ();
    static int i = 0;
	initial begin
	// CHECK: scf.while : () -> () {
	// CHECK:   [[TMP1:%.+]] = moore.constant 2 : !moore.int
	// CHECK:   [[TMP2:%.+]] = moore.ne %i, [[TMP1]] : !moore.int -> !moore.bit
	// CHECK:   [[TMP3:%.+]] = moore.bool_cast [[TMP2]] : !moore.bit -> !moore.bit
	// CHECK:   [[TMP4:%.+]] = moore.conversion [[TMP3]] : !moore.bit -> i1
	// CHECK:   scf.condition([[TMP4]])
	// CHECK: } do {
	// CHECK:   [[TMP1:%.+]] = moore.constant 1 : !moore.int
	// CHECK:   [[TMP2:%.+]] = moore.add %i, [[TMP1]] : !moore.int
	// CHECK:   moore.bpassign %i, [[TMP2]] : !moore.int
	// CHECK:   scf.yield
	// CHECK: }
		while(i != 2)begin
			i++;
		end
	end
endmodule

// CHECK-LABEL: moore.module @dowhile_tb
module dowhile_tb ();
    static int i = 0;
	initial begin
	// CHECK: scf.while : () -> () {
	// CHECK:   [[TMP1:%.+]] = moore.constant 1 : !moore.int
	// CHECK:   [[TMP2:%.+]] = moore.add %i, [[TMP1]] : !moore.int
	// CHECK:   moore.bpassign %i, [[TMP2]] : !moore.int
	// CHECK:   [[TMP3:%.+]] = moore.constant 2 : !moore.int
	// CHECK:   [[TMP4:%.+]] = moore.ne %i, [[TMP3]] : !moore.int -> !moore.bit
	// CHECK:   [[TMP5:%.+]] = moore.bool_cast [[TMP4]] : !moore.bit -> !moore.bit
	// CHECK:   [[TMP6:%.+]] = moore.conversion [[TMP5]] : !moore.bit -> i1
	// CHECK:   scf.condition([[TMP6]])
	// CHECK: } do {
	// CHECK:   scf.yield
	// CHECK: }
		do begin
			i++;
		end while(i != 2); 
	end
endmodule

// CHECK-LABEL: moore.module @for_tb
module for_tb ();
	// CHECK:   [[TMP0:%.+]] = moore.constant 0 : !moore.int
	// CHECK:   %i = moore.variable [[TMP0]] : !moore.int
	// CHECK: scf.while : () -> () {
	// CHECK:   [[TMP1:%.+]] = moore.constant 2 : !moore.int
	// CHECK:   [[TMP2:%.+]] = moore.lt %i, [[TMP1]] : !moore.int -> !moore.bit
	// CHECK:   [[TMP3:%.+]] = moore.bool_cast [[TMP2]] : !moore.bit -> !moore.bit
	// CHECK:   [[TMP4:%.+]] = moore.conversion [[TMP3]] : !moore.bit -> i1
	// CHECK:   scf.condition([[TMP4]])
	// CHECK: } do {
	// CHECK:   [[TMP1:%.+]] = moore.constant 1 : !moore.int
	// CHECK:   [[TMP2:%.+]] = moore.add %i, [[TMP1]] : !moore.int
	// CHECK:   moore.bpassign %i, [[TMP2]] : !moore.int
	// CHECK:   scf.yield
	// CHECK: }
	initial begin
        for(int i=0;i<2;++i)begin
        end
	end
endmodule

// CHECK-LABEL:   moore.module @repeat_tb {
// CHECK: [[TMP1:%.*]] = moore.variable {{%.+}} : !moore.int
// CHECK: moore.procedure initial {
// CHECK:   scf.while ([[TMP0:%.*]] = [[TMP1]]) : (!moore.int) -> !moore.int {
// CHECK:     [[TMP2:%.+]] = moore.bool_cast [[TMP1]] : !moore.int -> !moore.bit
// CHECK:     [[TMP3:%.+]] = moore.conversion [[TMP2]] : !moore.bit -> i1
// CHECK:     scf.condition([[TMP3]]) [[TMP0]] : !moore.int
// CHECK:   } do {
// CHECK:   ^bb0([[TMP4:%.+]]: !moore.int):
// CHECK:     [[TMP2:%.+]] = moore.constant 1 : !moore.int
// CHECK:     [[TMP3:%.+]] = moore.sub [[TMP4]], [[TMP2]] : !moore.int
// CHECK:     scf.yield [[TMP3]] : !moore.int
module repeat_tb ();
	int a = 10;
	initial begin
		repeat(a)begin
		end
	end
endmodule

// CHECK-LABEL: moore.module @TestForeach
// CHECK:  [[t0:%.+]] = moore.constant 1 : !moore.int
// CHECK:  [[t2:%.+]] = moore.constant 2 : !moore.int
// CHECK:  [[t3:%.+]] = moore.constant 0 : !moore.int
// CHECK:  [[t4:%.+]] = scf.while ([[arg0:%.+]] = [[t3]]) : (!moore.int) -> !moore.int {
// CHECK:    [[t7:%.+]] = moore.lt [[t3]], [[t2]] : !moore.int -> !moore.bit
// CHECK:    [[t8:%.+]] = moore.conversion [[t7]] : !moore.bit -> i1
// CHECK:    scf.condition([[t8]]) [[arg0]] : !moore.int
// CHECK:  } do {
// CHECK:  ^bb0([[arg0]]: !moore.int):
// CHECK:    [[t7:%.+]] = moore.constant 0 : !moore.int
// CHECK:    [[t8:%.+]] = moore.constant 3 : !moore.int
// CHECK:    [[t9:%.+]] = moore.constant 0 : !moore.int
// CHECK:    [[t10:%.+]] = scf.while ([[arg1:%.+]] = [[t9]]) : (!moore.int) -> !moore.int {
// CHECK:      [[t12:%.+]] = moore.lt [[t9]], [[t8]] : !moore.int -> !moore.bit
// CHECK:      [[t13:%.+]] = moore.conversion [[t12]] : !moore.bit -> i1
// CHECK:      scf.condition([[t13]]) [[arg1]] : !moore.int
// CHECK:    } do {
// CHECK:    ^bb0([[arg1]]: !moore.int):
// CHECK:      [[t12:%.+]] = moore.constant 1 : !moore.int
// CHECK:      [[t13:%.+]] = moore.add %a, [[t12]] : !moore.int
// CHECK:      moore.bpassign %a, [[t13]] : !moore.int
// CHECK:      [[t14:%.+]] = moore.add [[arg1]], [[t0]] : !moore.int
// CHECK:      scf.yield [[t14]] : !moore.int
// CHECK:    }
// CHECK:    [[t11:%.+]] = moore.add [[arg0]], [[t0]] : !moore.int
// CHECK:    scf.yield [[t11]] : !moore.int
// CHECK:  }
// CHECK:  [[t5:%.+]] = moore.constant 1 : !moore.int
// CHECK:  [[t6:%.+]] = moore.add %a, [[t5]] : !moore.int
// CHECK:  moore.bpassign %a, [[t6]] : !moore.int
// CHECK:}

module TestForeach;
bit array[3][4][4][4];
int a;
initial begin
    foreach (array[i, ,m,]) begin
        a++;
    end
    a++;
end
endmodule
