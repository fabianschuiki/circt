//===- MIRStatements.td - Moore MIR statements ops ---------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This describes the ops for Moore MIR statements.
//
//===----------------------------------------------------------------------===//

include "circt/Dialect/Moore/MooreTypes.td"

//===----------------------------------------------------------------------===//
// Assignment Statements
//===----------------------------------------------------------------------===//

def AssignOp : MIROp<"assign", [SameTypeOperands]> {
  let summary = "Continuous assignment";
  let description = [{
    A SystemVerilog assignment statement 'x = y;'.
    These occur in module scope.  
    See IEEE 1800-2017 § 10.3.2 "The continuous assignment statement".
  }];
  let arguments = (ins UnpackedType:$dest, UnpackedType:$src);
  let results = (outs);
  let assemblyFormat = [{
    $dest `,` $src  attr-dict `:` qualified(type($src))
  }];
}

def BPAssignOp : MIROp<"bpassign", [SameTypeOperands]> {
  let summary = "Blocking procedural assignment";
  let description = [{
    A SystemVerilog blocking procedural assignment statement 'x = y;'.
    These occur in a sequential block.  
    See IEEE 1800-2017 § 10.4.1 "Blocking procedural assignments".
  }];
  let arguments = (ins UnpackedType:$dest, UnpackedType:$src);
  let results = (outs);
  let assemblyFormat = [{
    $dest `,` $src  attr-dict `:` qualified(type($src))
  }];
}

def PAssignOp : MIROp<"passign", [SameTypeOperands]> {
  let summary = "Nonblocking procedural assignment";
  let description = [{
    A SystemVerilog nonblocking procedural assignment statement 'x <= y;'.
    These occur in a sequential block.  
    See IEEE 1800-2017 § 10.4.2 "Nonblocking procedural assignments".
  }];
  let arguments = (ins UnpackedType:$dest, UnpackedType:$src);
  let results = (outs);
  let assemblyFormat = [{
    $dest `,` $src  attr-dict `:` qualified(type($src))
  }];
}

def PCAssignOp : MIROp<"pcassign", [SameTypeOperands]> {
  let summary = "Procedural continuous assignment";
  let description = [{
    A SystemVerilog assignment statement 'assign x = y;'.
    The procedural continuous assignments (using keywords assign and force) 
    are procedural statements that allow expressions to be driven continuously 
    onto variables or nets. These occur in procedural scope.  
    See IEEE 1800-2017 § 10.6 "Procedural continuous assignments".
  }];
  let arguments = (ins UnpackedType:$dest, UnpackedType:$src);
  let results = (outs);
  let assemblyFormat = [{
    $dest `,` $src  attr-dict `:` qualified(type($src))
  }];
}
