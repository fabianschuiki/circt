//===- MIRExpressions.td - Moore MIR expression ops --------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This describes the ops for Moore MIR expressions.
//
//===----------------------------------------------------------------------===//

include "circt/Dialect/Moore/MooreTypes.td"
include "mlir/IR/EnumAttr.td"
include "mlir/Interfaces/InferTypeOpInterface.td"

// Base class for binary operators.
class BinOp<string mnemonic, list<Trait> traits = []> :
      MIROp<mnemonic, traits> {
  let arguments = (ins AnyType:$lhs, AnyType:$rhs, UnitAttr:$twoState);
  let results = (outs AnyType:$result);

  let assemblyFormat =
    "$lhs `,` $rhs (`bin` $twoState^)? attr-dict `:` functional-type($args, $results)";
}

// Binary operator with uniform input types.
class UTBinOp<string mnemonic, list<Trait> traits = []> :
      BinOp<mnemonic,
               traits # [SameTypeOperands, SameOperandsAndResultType]> {
  let assemblyFormat = "(`bin` $twoState^)? $lhs `,` $rhs attr-dict `:` qualified(type($result))";
}

// Base class for   variadic operators.
class VariadicOp<string mnemonic, list<Trait> traits = []> :
      MIROp<mnemonic, traits> {
  let arguments = (ins Variadic<AnyType>:$inputs, UnitAttr:$twoState);
  
  let results = (outs AnyType:$result);
}

// VariadicOp with uniform input types.
class UTVariadicOp<string mnemonic, list<Trait> traits = []> :
      VariadicOp<mnemonic,
                    traits # [SameTypeOperands, SameOperandsAndResultType]> {

  let assemblyFormat = "(`bin` $twoState^)? $inputs attr-dict `:` qualified(type($result))";

  let builders = [
    OpBuilder<(ins "Value":$lhs, "Value":$rhs, CArg<"bool", "false">:$twoState), [{
      return build($_builder, $_state, lhs.getType(),
                   ValueRange{lhs, rhs}, twoState);
    }]>
  ];
}

def ConstantOp : MIROp<"constant", [Pure]> {
  let summary = "A constant value";

  let arguments = (ins I32Attr:$value);
  let results = (outs UnpackedType:$result);
  let assemblyFormat = "$value attr-dict `:` qualified(type($result))";
}

def ConcatOp : MIROp<"concat", [
    Pure, DeclareOpInterfaceMethods<InferTypeOpInterface>
]> {
  let summary = "A concatenation of expressions";
  let description = [{
    This operation represents the SystemVerilog concatenation expression
    `{x, y, z}`. See IEEE 1800-2017 §11.4.12 "Concatenation operators".

    All operands must be simple bit vector types.

    The concatenation result is a simple bit vector type. The result is unsigned
    regardless of the sign of the operands (see concatenation-specific rules in
    IEEE 1800-2017 §11.8.1 "Rules for expression types"). The size of the result
    is the sum of the sizes of all operands. If any of the operands is
    four-valued, the result is four-valued; otherwise it is two-valued.
  }];
  let arguments = (ins Variadic<SimpleBitVectorType>:$values);
  let results = (outs SimpleBitVectorType:$result);
  let assemblyFormat = [{
    $values attr-dict `:` functional-type($values, $result)
  }];
}

//===----------------------------------------------------------------------===//
// Unary operations
//===----------------------------------------------------------------------===//

def Plus : I32EnumAttrCase<"Plus", 0, "plus">;
def Minus : I32EnumAttrCase<"Minus", 1, "minus">;
def LogicalNot : I32EnumAttrCase<"LogicalNot", 2, "not">;

def UnaryAttr : I32EnumAttr<"Unary", "Three common unary operators",
                            [Plus, Minus, LogicalNot]>{
  let cppNamespace = "circt::moore";
}

def UnaryOp : MIROp<"unary",[
  Pure,
  TypesMatchWith<"value and result types must match",
                   "value", "result", "$_self">
]> {
  let summary = "Definitions of the unary operations";
  let description = [{
    See IEEE Std 1800-2017 11.4.2 and 11.4.7.
  }];

  let arguments = (ins UnaryAttr:$unary,
                       SimpleBitVectorType:$value);
  let results = (outs SimpleBitVectorType:$result);
  let assemblyFormat = [{
    $unary $value attr-dict `:` type($value)
  }];
}

//===----------------------------------------------------------------------===//
// Reduction operations
//===----------------------------------------------------------------------===//

def BitwiseNot : I32EnumAttrCase<"BitwiseNot", 0, "not">;
def BitwiseAnd : I32EnumAttrCase<"BitwiseAnd", 1, "and">;
def BitwiseOr : I32EnumAttrCase<"BitwiseOr", 2, "or">;
def BitwiseXor : I32EnumAttrCase<"BitwiseXor", 3, "xor">;
def BitwiseNand : I32EnumAttrCase<"BitwiseNand", 4, "nand">;
def BitwiseNor : I32EnumAttrCase<"BitwiseNor", 5, "nor">;
def BitwiseXnor : I32EnumAttrCase<"BitwiseXnor", 6, "xnor">;


def ReductionAttr : I32EnumAttr<"Reduction", "Reduction predicate",
                      [BitwiseNot, BitwiseAnd, BitwiseOr, BitwiseXor,
                       BitwiseNand, BitwiseNor, BitwiseXnor]>{
  let cppNamespace = "circt::moore";
}

def ReductionOp : MIROp<"reduction",[
  Pure,
  TypesMatchWith<"value and result types must match",
                   "value", "result", "$_self">
]> {
  let summary = "Definitions of the reduction operations";
  let description = [{
    See IEEE Std 1800-2017 11.4.9.
  }];

  let arguments = (ins ReductionAttr:$reduction,
                       SimpleBitVectorType:$value);
  let results = (outs SimpleBitVectorType:$result);
  let assemblyFormat = [{
    $reduction $value attr-dict `:` type($value)
  }];
}

//===----------------------------------------------------------------------===//
// Shift operations
//===----------------------------------------------------------------------===//

class ShiftOp<string name> : MIROp<name, [
    Pure, 
    TypesMatchWith<"value and result types must match",
                   "value", "result", "$_self">
]> {
  let arguments = (ins SimpleBitVectorType:$value,
                       SimpleBitVectorType:$amount,
                       UnitAttr:$arithmetic);
  let results = (outs SimpleBitVectorType:$result);
  let assemblyFormat = [{
    ( `arithmetic` $arithmetic^ )? $value `,` $amount attr-dict
    `:` type($value) `,` type($amount)
  }];
}

def ShlOp : ShiftOp<"shl"> {
  let summary = "A logical or arithmetic left-shift expression";
  let description = [{
    This operation represents the SystemVerilog logical and arithmetic
    left-shift expressions `<<` and `<<<`.
    See IEEE 1800-2017 §11.4.10 "Shift operators".

    The value to be shifted and the amount must be simple bit vector types.
    The shift result is of the same type as the input value.

    The logical and arithmetic shift both insert zeros in place of the shifted
    bits. 
  }];
}

def ShrOp : ShiftOp<"shr"> {
  let summary = "A logical or arithmetic right-shift expression";
  let description = [{
    This operation represents the SystemVerilog logical and arithmetic
    right-shift expressions `>>` and `>>>`.
    See IEEE 1800-2017 §11.4.10 "Shift operators".

    The value to be shifted and the amount must be simple bit vector types.
    The shift result is of the same type as the input value.

    The logical shift always inserts zeros in place of the shifted bits.
    The arithmetic shift inserts zeros if the result type is unsigned or the 
    MSB (sign bit) if the result type is signed.
  }];
}

//===---------------------------------------------------------------------===//
// Equality operations
//===---------------------------------------------------------------------===//

class EqualOp<string name> : MIROp<name, [
  Pure,
  Commutative
]> {
  let arguments = (ins SimpleBitVectorType:$lhs, 
                       SimpleBitVectorType:$rhs, 
                       UnitAttr:$mode);
  let results = (outs I1:$result);
  let assemblyFormat = [{
    ( `case` $mode^ )? $lhs `,` $rhs attr-dict `:` type($lhs) `,` type($rhs)
  }];
} 

def EqualityOp : EqualOp<"eq">{
  let summary = "A logical or case equality expression";
  let description = [{
    This operation represents the SystemVerilog logical and case
    equality expressions `==` and `===`.
    See IEEE1800-2017 11.4.5 "Equality operators".

    a == ba equal to b, result can be unknown.
    a === ba equal to b, including x and z.

    The operators compare operands bit for bit. As with the relational 
    operators, the result shall be 0 if comparison fails and 1 if it succeeds.
  }];
}

def InEqualityOp : EqualOp<"ne">{
  let summary = "A logical or case inequality expression";
  let description = [{
    This operation represents the SystemVerilog logical and case
    inequality expressions `!=` and `!==`.
    See IEEE1800-2017 11.4.5 "Equality operators".

    a != ba not equal to b, result can be unknown.
    a !== ba not equal to b, including x and z.
  }];
}

//===---------------------------------------------------------------------===//
// Relational operations
//===---------------------------------------------------------------------===//

def GreaterThanEqual : I32EnumAttrCase<"GreaterThanEqual", 0, "gte">;
def GreaterThan : I32EnumAttrCase<"GreaterThan", 1, "gt">;
def LessThanEqual : I32EnumAttrCase<"LessThanEqual", 2, "lte">;
def LessThan : I32EnumAttrCase<"LessThan", 3, "lt">;

def RelationalAttr : I32EnumAttr<"Relation", "Relational predicate", 
            [GreaterThanEqual, GreaterThan, LessThanEqual, LessThan]>{
    let cppNamespace = "circt::moore";
}

def RelationalOp : MIROp<"icmp", [
  Pure
]> {
  let summary = "Definitions of relational operators";
  let description = [{
    The result shall be the scalar value 0 if the specified relation is false
    or the value 1 if it is true. A 1-bit unknown value (x) will be yield 
    if the `x` or `z` occur in operand.
    See IEEE Std 1800-2017 11.4.4.
  }];
  let arguments = (ins RelationalAttr:$relation,
                       SimpleBitVectorType:$lhs, 
                       SimpleBitVectorType:$rhs);
  let results = (outs I1:$result);
  
  let assemblyFormat = [{
    $relation $lhs `,` $rhs attr-dict `:` type($lhs) `,` type($rhs)
  }];
}

//===---------------------------------------------------------------------===//
// Logical operations
//===---------------------------------------------------------------------===//

def LogicalAnd : I32EnumAttrCase<"LogicalAnd", 0, "and">;
def LogicalOr : I32EnumAttrCase<"LogicalOr", 1, "or">;
def LogicalImplication : I32EnumAttrCase<"LogicalImplication", 2, "impl">;
def LogicalEquivalence : I32EnumAttrCase<"LogicalEquivalence", 3, "equiv">;

def LogicalAttr : I32EnumAttr<"Logic", "Logical predicate", 
       [LogicalAnd, LogicalOr, LogicalImplication, LogicalEquivalence]>{
    let cppNamespace = "circt::moore";
}

def LogicalOp : MIROp<"logic", [
  Pure,
  TypesMatchWith<"lhs and result types must match",
                   "lhs", "result", "$_self">
]> {
  let summary = "Definitions of logical operators";
  let description = [{
    The result of the evaluation of a logical operation shall be `1`, `0`, 
    or, if the result is ambiguous, the unkonw value `x`.
    See IEEE Std 1800-2017 11.4.7.
  }];
  let arguments = (ins LogicalAttr:$logic,
                       SimpleBitVectorType:$lhs,
                       SimpleBitVectorType:$rhs);
  let results = (outs SimpleBitVectorType:$result);

  let assemblyFormat = [{
    $logic $lhs `,` $rhs attr-dict `:` type($lhs) `,` type($rhs)
  }];
}

//===---------------------------------------------------------------------===//
// Bitwise operations
//===---------------------------------------------------------------------===//

def BinaryAnd : I32EnumAttrCase<"BinaryAnd", 0, "and">;
def BinaryOr : I32EnumAttrCase<"BinaryOr", 1, "or">;
def BinaryXor : I32EnumAttrCase<"BinaryXor", 2, "xor">;
def BinaryXnor : I32EnumAttrCase<"BinaryXnor", 3, "xnor">;

def BitwiseAttr : I32EnumAttr<"Bitwise", "Binary bitwise predicate",
                       [BinaryAnd, BinaryOr, BinaryXor, BinaryXnor]>{
  let cppNamespace = "circt::moore";
}

def BitwiseOp : MIROp<"binBitwise", [
  Pure,
  TypesMatchWith<"lhs and result types must match",
                   "lhs", "result", "$_self">
]> {
  let summary = "Definitions of the binary bitwise operators";
  let description = [{
    This operator shall combine a bit in one operand with its corresponding
    bit in the other operand to calculate 1 bit for the result.
    See IEEE Std 1800-2017 11.4.8.
  }];
  let arguments = (ins BitwiseAttr:$bitwise,
                       SimpleBitVectorType:$lhs,
                       SimpleBitVectorType:$rhs);
  let results = (outs SimpleBitVectorType:$result);

  // let skipDefaultBuilders = 1;
  // let builders = [
  //   OpBuilder<(ins "Bitwise":$bitwise, "Value":$lhs, "Value":$rhs),
  //   [{
  //     $_state.addOperands({lhs,rhs});
  //     $_state.addAttribute("bitwise", 
  //       $_builder.getI32IntegerAttr(static_cast<int32_t>(bitwise)));
  //     $_state.addTypes(lhs.getType());
  //   }]>
  // ];

  let assemblyFormat = [{
    $bitwise $lhs `,` $rhs attr-dict `:` type($lhs) `,` type($rhs)
  }];
}


//===---------------------------------------------------------------------===//
// Arithmetic Operations 
//===---------------------------------------------------------------------===//

// Arithmetic and Logical Operations.
def AddOp : UTVariadicOp<"add", [Commutative]>;
def MulOp : UTVariadicOp<"mul", [Commutative]>;
