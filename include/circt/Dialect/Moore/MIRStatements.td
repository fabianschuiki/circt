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

def VariableDeclOp : MIROp<"vardecl", []> {
  let summary = "A variable declaration";

  let arguments = (ins StrAttr:$name, I32Attr:$init);
  let results = (outs MooreLValueType:$result);
  let assemblyFormat = [{
    $name `=` $init attr-dict `:` custom<LValueType>(type($result))
  }];
}

//===----------------------------------------------------------------------===//
// Assignment Statements
//===----------------------------------------------------------------------===//

def AssignOp : MIROp<"assign",
    [TypesMatchWith<"src and dest types have to match", "src", "dest",
     "LValueType::get($_self)">]> {

  let summary = "Continuous assignment";
  let description = [{
    A SystemVerilog assignment statement 'x = y;'.
    These occur in module scope.  See SV Spec 10.3.2.
  }];
  let arguments = (ins MooreLValueType:$dest, MooreRValueType:$src);
  let results = (outs);
  let assemblyFormat = [{
    $dest `,` $src  attr-dict `:` qualified(type($src))
  }];
}

//===----------------------------------------------------------------------===//
// Conditional Statements
//===----------------------------------------------------------------------===//

def IfOp : MIROp<"if", [
  SingleBlock,
  NoTerminator,
  NoRegionArguments
]>{
  let summary = "if-else statement";
  let description = [{
    See SV Spec 12.4
  }];

  let regions = (region SizedRegion<1>:$thenRegion, AnyRegion:$elseRegion);
  let arguments = (ins UnpackedType:$cond);
  let results = (outs);
  let assemblyFormat = [{
   $cond $thenRegion (`else` $elseRegion^)? attr-dict `:` qualified(type($cond))
  }];

  let skipDefaultBuilders = 1;
  let builders = [
    OpBuilder<(ins "Value":$cond,
                      CArg<"std::function<void()>", "{}">:$thenCtor,
                      CArg<"std::function<void()>", "{}">:$elseCtor)>
  ];

  let extraClassDeclaration = [{
    bool hasElse() { return !getElseRegion().empty(); }
    
    Block *getThenBlock() {
      assert(!getThenRegion().empty() && "'then' region does not exist.");
      return &getThenRegion().front();
    }

    Block *getElseBlock() {
      assert(hasElse() && "'else' region does not exist.");
      return &getElseRegion().front();
    }
  }];
}
