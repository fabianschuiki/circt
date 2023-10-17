//===- MooreOps.td - Moore dialect operations --------------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

include "mlir/IR/RegionKindInterface.td"
include "circt/Dialect/Moore/MooreTypes.td"

def SVModuleOp : MooreOp<"module", [
  IsolatedFromAbove,
  Symbol,
  RegionKindInterface,
  NoTerminator
]> {
  let summary = "A module definition";

  let arguments = (ins SymbolNameAttr:$sym_name);
  let regions = (region SizedRegion<1>:$body);
  let assemblyFormat = [{
    $sym_name attr-dict-with-keyword $body
  }];

  let extraClassDeclaration = [{
    static mlir::RegionKind getRegionKind(unsigned index) {
      return mlir::RegionKind::SSACFG;
    }

    mlir::Block &getBodyBlock() { return getBody().front(); }
  }];
}

def InstanceOp : MooreOp<"instance", [
  DeclareOpInterfaceMethods<SymbolUserOpInterface>
]> {
  let summary = "Create an instance of a module";

  let arguments = (ins StrAttr:$instanceName,
                       FlatSymbolRefAttr:$moduleName);
  let assemblyFormat = [{
    $instanceName $moduleName attr-dict
  }];
}

def VariableOp : MooreOp<"variable", [
  DeclareOpInterfaceMethods<OpAsmOpInterface, ["getAsmResultNames"]>
]> {
  let summary = "Declare a variable";
  let arguments = (ins StrAttr:$name);
  let results = (outs UnpackedType:$result);
  let assemblyFormat = [{
    `` custom<ImplicitSSAName>($name) attr-dict `:` type($result)
  }];
}

def AlwaysCombOp : MooreOp<"alwayscomb", [
  SingleBlock,
  NoTerminator,
  NoRegionArguments,
  RecursiveMemoryEffects,
  RecursivelySpeculatable
]> {
  let summary = "'alwayscomb block";
  let description = "See SV Spec 9.2, and 9.2.2.2.";

  let regions = (region SizedRegion<1>:$body);
  let arguments = (ins);
  let results = (outs);

  let assemblyFormat = [{
    $body attr-dict
  }];

  let skipDefaultBuilders = 1;
  let builders = [
    OpBuilder<(ins CArg<"std::function<void()>", "{}">:$bodyCtor)>
  ];

  let extraClassDeclaration = [{
    static mlir::RegionKind getRegionKind(unsigned index) {
      return mlir::RegionKind::SSACFG;
    }

    mlir::Block &getBodyBlock() { return getBody().front(); }
  }];
}

def InitialOp : MooreOp<"initial", [
  SingleBlock,
  NoTerminator,
  NoRegionArguments,
  RecursiveMemoryEffects,
  RecursivelySpeculatable
]> {
  let summary = "'initial block";
  let description = "See SV Spec 9.2.1.";

  let regions = (region SizedRegion<1>:$body);
  let arguments = (ins);
  let results = (outs);

  let assemblyFormat = [{
    $body attr-dict
  }];

  let skipDefaultBuilders = 1;
  let builders = [
    OpBuilder<(ins CArg<"std::function<void()>", "{}">:$bodyCtor)>
  ];

  let extraClassDeclaration = [{
    static mlir::RegionKind getRegionKind(unsigned index) {
      return mlir::RegionKind::SSACFG;
    }

    mlir::Block &getBodyBlock() { return getBody().front(); }
  }];
}
