//===- MooreOps.cpp - Implement the Moore operations ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the Moore dialect operations.
//
//===----------------------------------------------------------------------===//

#include "circt/Dialect/Moore/MooreOps.h"
#include "circt/Support/CustomDirectiveImpl.h"
#include "mlir/Dialect/CommonFolders.h"
#include "mlir/IR/Builders.h"

using namespace circt;
using namespace circt::moore;

//===----------------------------------------------------------------------===//
// InstanceOp
//===----------------------------------------------------------------------===//

LogicalResult InstanceOp::verifySymbolUses(SymbolTableCollection &symbolTable) {
  auto *module =
      symbolTable.lookupNearestSymbolFrom(*this, getModuleNameAttr());
  if (module == nullptr)
    return emitError("unknown symbol name '") << getModuleName() << "'";

  // It must be some sort of module.
  if (!isa<SVModuleOp>(module))
    return emitError("symbol '")
           << getModuleName()
           << "' must reference a 'moore.module', but got a '"
           << module->getName() << "' instead";

  return success();
}

//===----------------------------------------------------------------------===//
// VariableOp
//===----------------------------------------------------------------------===//

void VariableOp::getAsmResultNames(OpAsmSetValueNameFn setNameFn) {
  setNameFn(getResult(), getName());
}

//===----------------------------------------------------------------------===//
// NetOp
//===----------------------------------------------------------------------===//

void NetOp::getAsmResultNames(OpAsmSetValueNameFn setNameFn) {
  setNameFn(getResult(), getName());
}

//===----------------------------------------------------------------------===//
// ConstantOp
//===----------------------------------------------------------------------===//

void ConstantOp::print(OpAsmPrinter &p) {
  p << " ";
  p.printAttributeWithoutType(getValueAttr());
  p.printOptionalAttrDict((*this)->getAttrs(), /*elidedAttrs=*/{"value"});
  p << " : ";
  p.printType(getType());
}

ParseResult ConstantOp::parse(OpAsmParser &parser, OperationState &result) {
  APInt value;
  UnpackedType type;

  if (parser.parseInteger(value) ||
      parser.parseOptionalAttrDict(result.attributes) || parser.parseColon() ||
      parser.parseType(type))
    return failure();

  auto sbvt = type.getSimpleBitVectorOrNull();
  if (!sbvt)
    return parser.emitError(parser.getCurrentLocation(),
                            "expected simple bit vector type");

  auto attrType = IntegerType::get(parser.getContext(), sbvt.size);
  auto attrValue = IntegerAttr::get(attrType, value);

  result.addAttribute("value", attrValue);
  result.addTypes(type);
  return success();
}

LogicalResult ConstantOp::verify() {
  auto sbvt = getType().getSimpleBitVector();
  auto width = getValue().getBitWidth();
  if (width != sbvt.size)
    return emitError("attribute width ")
           << width << " does not match return type's width " << sbvt.size;
  return success();
}

void ConstantOp::build(OpBuilder &builder, OperationState &result, Type type,
                       const APInt &value) {
  auto sbvt = type.cast<UnpackedType>().getSimpleBitVector();
  assert(sbvt.size == value.getBitWidth() &&
         "APInt width must match simple bit vector's bit width");
  build(builder, result, type,
        builder.getIntegerAttr(builder.getIntegerType(sbvt.size), value));
}

/// This builder allows construction of small signed integers like 0, 1, -1
/// matching a specified MLIR type. This shouldn't be used for general constant
/// folding because it only works with values that can be expressed in an
/// `int64_t`.
void ConstantOp::build(OpBuilder &builder, OperationState &result, Type type,
                       int64_t value) {
  auto sbvt = type.cast<UnpackedType>().getSimpleBitVector();
  build(builder, result, type,
        APInt(sbvt.size, (uint64_t)value, /*isSigned=*/true));
}

mlir::OpFoldResult ConstantOp::fold(ConstantOp::FoldAdaptor adaptor) {
  return adaptor.getValueAttr();
}

mlir::OpFoldResult AddOp::fold(AddOp::FoldAdaptor adaptor) {
  auto type = getType();
  if (type.getDomain() == moore::Domain::FourValued) {
    // TODO: handle fourValue fold
    return nullptr;
  }
  auto lhs = dyn_cast<mlir::IntegerAttr>(adaptor.getOperands()[0]);
  auto rhs = dyn_cast<mlir::IntegerAttr>(adaptor.getOperands()[1]);
  if (!lhs || !rhs) {
    return nullptr;
  }
  // Fold `add(x, 0) -> 0`.
  if (rhs.getValue().isZero()) {
    return getLhs();
  }
  if (lhs.getValue().isZero()) {
    return getRhs();
  }
  return nullptr;
}

mlir::OpFoldResult SubOp::fold(SubOp::FoldAdaptor adaptor) {
  auto type = getType();
  if (type.getDomain() == moore::Domain::FourValued) {
    // TODO: handle fourValue fold
    return nullptr;
  }
  auto lhs = dyn_cast<mlir::IntegerAttr>(adaptor.getOperands()[0]);
  auto rhs = dyn_cast<mlir::IntegerAttr>(adaptor.getOperands()[1]);
  if (!lhs || !rhs) {
    return nullptr;
  }
  // Fold `sub(x, 0) -> x`.
  if (rhs.getValue().isZero()) {
    return getLhs();
  }

  return nullptr;
}

mlir::OpFoldResult AndOp::fold(AndOp::FoldAdaptor adaptor) {
  auto type = getType();
  if (type.getDomain() == moore::Domain::FourValued) {
    // TODO: handle fourValue fold
    return nullptr;
  }
  auto lhs = dyn_cast<mlir::IntegerAttr>(adaptor.getOperands()[0]);
  auto rhs = dyn_cast<mlir::IntegerAttr>(adaptor.getOperands()[1]);
  if (!lhs || !rhs) {
    return nullptr;
  }
  // Fold `and(x, 0) -> 0`.
  if (rhs.getValue().isZero()) {
    return getRhs();
  }
  if (lhs.getValue().isZero()) {
    return getLhs();
  }

  // Fold `and(x,'1) -> x`
  if (rhs.getValue().isMaxValue()) {
    return getLhs();
  }
  if (lhs.getValue().isMaxValue()) {
    return getRhs();
  }

  // Fold `and(x,x) -> x`
  if (rhs.getValue() == lhs.getValue()) {
    return getLhs();
  }
  return nullptr;
}

mlir::OpFoldResult OrOp::fold(OrOp::FoldAdaptor adaptor) {
  auto type = getType();
  if (type.getDomain() == moore::Domain::FourValued) {
    // TODO: handle fourValue fold
    return nullptr;
  }
  auto lhs = dyn_cast<mlir::IntegerAttr>(adaptor.getOperands()[0]);
  auto rhs = dyn_cast<mlir::IntegerAttr>(adaptor.getOperands()[1]);
  if (!lhs || !rhs) {
    return nullptr;
  }

  // Fold `or(x, '1) -> '1`
  if (lhs.getValue().isMaxValue()) {
    return getLhs();
  }
  if (rhs.getValue().isMaxValue()) {
    return getRhs();
  }

  // Fold `or(x,0) -> x`
  if (rhs.getValue().isZero()) {
    return getLhs();
  }
  if (lhs.getValue().isZero()) {
    return getRhs();
  }

  return nullptr;
}

mlir::OpFoldResult XorOp::fold(XorOp::FoldAdaptor adaptor) {
  auto type = getType();
  if (type.getDomain() == moore::Domain::FourValued) {
    // TODO: handle fourValue fold
    return nullptr;
  }
  auto lhs = dyn_cast<mlir::IntegerAttr>(adaptor.getOperands()[0]);
  auto rhs = dyn_cast<mlir::IntegerAttr>(adaptor.getOperands()[1]);
  if (!lhs || !rhs) {
    return nullptr;
  }

  // Fold `xor(x, 0) -> x`
  if (rhs.getValue().isZero()) {
    return getLhs();
  }
  if (lhs.getValue().isZero()) {
    return getRhs();
  }

  return nullptr;
}

//===----------------------------------------------------------------------===//
// ConcatOp
//===----------------------------------------------------------------------===//

LogicalResult ConcatOp::inferReturnTypes(
    MLIRContext *context, std::optional<Location> loc, ValueRange operands,
    DictionaryAttr attrs, mlir::OpaqueProperties properties,
    mlir::RegionRange regions, SmallVectorImpl<Type> &results) {
  Domain domain = Domain::TwoValued;
  unsigned size = 0;
  for (auto operand : operands) {
    auto type = operand.getType().cast<UnpackedType>().getSimpleBitVector();
    if (type.domain == Domain::FourValued)
      domain = Domain::FourValued;
    size += type.size;
  }
  results.push_back(
      SimpleBitVectorType(domain, Sign::Unsigned, size).getType(context));
  return success();
}

//===----------------------------------------------------------------------===//
// BoolCastOp
//===----------------------------------------------------------------------===//

OpFoldResult BoolCastOp::fold(FoldAdaptor adaptor) {
  // Fold away no-op casts.
  if (getInput().getType() == getResult().getType())
    return getInput();
  return {};
}

//===----------------------------------------------------------------------===//
// TableGen generated logic.
//===----------------------------------------------------------------------===//

// Provide the autogenerated implementation guts for the Op classes.
#define GET_OP_CLASSES
#include "circt/Dialect/Moore/Moore.cpp.inc"
#include "circt/Dialect/Moore/MooreEnums.cpp.inc"
