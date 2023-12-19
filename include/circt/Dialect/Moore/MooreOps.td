//===- MooreOps.td - Moore dialect operations --------------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef CIRCT_DIALECT_MOORE_MOOREOPS
#define CIRCT_DIALECT_MOORE_MOOREOPS

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
  DeclareOpInterfaceMethods<OpAsmOpInterface, ["getAsmResultNames"]>,
  OptionalTypesMatchWith<"initial value and variable types match",
    "result", "initial", "$_self">,
]> {
  let summary = "A variable declaration";
  let description = [{
    See IEEE 1800-2017 § 6.8 "Variable declarations".
  }];
  let arguments = (ins StrAttr:$name, Optional<UnpackedType>:$initial);
  let results = (outs UnpackedType:$result);
  let assemblyFormat = [{
    `` custom<ImplicitSSAName>($name) ($initial^)? attr-dict
    `:` type($result)
  }];
}

def NetOp : MooreOp<"net", [
  DeclareOpInterfaceMethods<OpAsmOpInterface, ["getAsmResultNames"]>,
  OptionalTypesMatchWith<"assigned value and variable types match",
    "result", "assignment", "$_self">,
]> {
  let summary = "A net declaration";
  let description = [{
    See IEEE 1800-2017 § 6.7 "Net declarations".
  }];
  let arguments = (ins
    StrAttr:$name,
    StrAttr:$kind,
    Optional<UnpackedType>:$assignment
  );
  let results = (outs UnpackedType:$result);
  let assemblyFormat = [{
    `` custom<ImplicitSSAName>($name) $kind ($assignment^)? attr-dict
    `:` type($result)
  }];
}

def Initial: I32EnumAttrCase<"Initial", 0, "initial">;
def Final: I32EnumAttrCase<"Final", 1, "final">;
def Always: I32EnumAttrCase<"Always", 2, "always">;
def AlwaysComb: I32EnumAttrCase<"AlwaysComb", 3, "always_comb">;
def AlwaysLatch: I32EnumAttrCase<"AlwaysLatch", 4, "always_latch">;
def AlwaysFF: I32EnumAttrCase<"AlwaysFF", 5, "always_ff">;

def ProcedureKindAttr: I32EnumAttr<"ProcedureKind", "Procedure kind",
            [Initial, Final, Always, AlwaysComb, AlwaysLatch, AlwaysFF]>{
  let cppNamespace = "circt::moore";
}

def ProcedureOp : MooreOp<"procedure", [
  SingleBlock,
  NoTerminator,
  NoRegionArguments,
  RecursiveMemoryEffects,
  RecursivelySpeculatable
]> {
  let summary = "A procedure executed at specific simulation steps";
  let description = [{
    See IEEE 1800-2017 § 9.2 "Structured procedures".
  }];

  let regions = (region SizedRegion<1>:$body);  
  let arguments = (ins ProcedureKindAttr:$kind);
  let results = (outs);

  let assemblyFormat = [{
    $kind attr-dict-with-keyword $body
  }];

  let extraClassDeclaration = [{
    mlir::Block &getBodyBlock() {
      if (getBody().empty())
        getBody().emplaceBlock();
      return getBody().front();
    }
  }];
}

def In: I32EnumAttrCase<"In", 0>;
def Out: I32EnumAttrCase<"Out", 1>;
def InOut: I32EnumAttrCase<"InOut", 2>;
def Ref: I32EnumAttrCase<"Ref", 3>;

def DirectionAttr: I32EnumAttr<"Direction", "Port direction",
                                  [In, Out, InOut, Ref]>{
  let cppNamespace = "circt::moore";
}

def PortOp : MooreOp<"port", [
  RecursiveMemoryEffects
]> {
  let summary = "Declare a port";
  let arguments = (ins StrAttr:$name, DirectionAttr:$direction);
  let results = (outs);
  let assemblyFormat = [{
    $direction $name attr-dict
  }];
}

def None: I32EnumAttrCase<"None", 0, "none">;
def PosEdge: I32EnumAttrCase<"PosEdge", 1, "posedge">;
def NegEdge: I32EnumAttrCase<"NegEdge", 2, "negedge">;
def BothEdges: I32EnumAttrCase<"BothEdges", 3, "bothedges">;

def EdgeAtrr: I32EnumAttr<"Edge", "Edge kind", 
                          [None, PosEdge, NegEdge, BothEdges]>{
  let cppNamespace = "circt::moore";
}

def EventControlOp : MooreOp<"event", [
  HasParent<"ProcedureOp">
]> {
  let summary = "Detecting posedge and negedge";
  let description = "See SV Spec 9.4.2.";
  let arguments = (ins EdgeAtrr:$edge, StrAttr:$name);
  let results = (outs);
  let assemblyFormat = [{
    $edge $name attr-dict
  }];
}

def ConversionOp : MooreOp<"conversion", [
  Pure
]> {
  let description = [{
    Like logic[31:0] a; int b; assign a = b;
    The type of b is converted from int to logic[31:0].
  }];
  let arguments = (ins AnyType:$src);
  let results = (outs SimpleBitVectorType:$result);
  
  let assemblyFormat = [{
    $src attr-dict `:` functional-type($src, $result)
  }];
}

#endif // CIRCT_DIALECT_MOORE_MOOREOPS
