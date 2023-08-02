//===- Types.cpp - Slang type conversion ----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ImportVerilogInternals.h"
#include "slang/ast/ASTVisitor.h"
#include "slang/ast/Symbol.h"
#include "slang/ast/symbols/CompilationUnitSymbols.h"
#include "slang/ast/symbols/InstanceSymbols.h"
#include "slang/ast/symbols/VariableSymbols.h"
#include "slang/ast/types/AllTypes.h"
#include "slang/ast/types/Type.h"
#include "slang/syntax/SyntaxVisitor.h"

using namespace circt;
using namespace ImportVerilog;

namespace {
struct TypeVisitor {
  Context &context;
  Location loc;
  TypeVisitor(Context &context, Location loc) : context(context), loc(loc) {}

  Type visit(const slang::ast::ScalarType &type) {
    moore::IntType::Kind kind;
    switch (type.scalarKind) {
    case slang::ast::ScalarType::Bit:
      kind = moore::IntType::Bit;
      break;
    case slang::ast::ScalarType::Logic:
      kind = moore::IntType::Logic;
      break;
    case slang::ast::ScalarType::Reg:
      kind = moore::IntType::Reg;
      break;
    }

    std::optional<moore::Sign> sign =
        type.isSigned ? moore::Sign::Signed : moore::Sign::Unsigned;
    if (sign == moore::IntType::getDefaultSign(kind))
      sign = {};

    return moore::IntType::get(context.getContext(), kind, sign);
  }

  Type visit(const slang::ast::FloatingType &type) {
    moore::RealType::Kind kind;
    switch (type.floatKind) {
    case slang::ast::FloatingType::Real:
      kind = moore::RealType::Real;
      break;
    case slang::ast::FloatingType::ShortReal:
      kind = moore::RealType::ShortReal;
      break;
    case slang::ast::FloatingType::RealTime:
      kind = moore::RealType::RealTime;
      break;
    }

    return moore::RealType::get(context.getContext(), kind);
  }

  Type visit(const slang::ast::PredefinedIntegerType &type) {
    moore::IntType::Kind kind;
    switch (type.integerKind) {
    case slang::ast::PredefinedIntegerType::Int:
      kind = moore::IntType::Int;
      break;
    case slang::ast::PredefinedIntegerType::ShortInt:
      kind = moore::IntType::ShortInt;
      break;
    case slang::ast::PredefinedIntegerType::LongInt:
      kind = moore::IntType::LongInt;
      break;
    case slang::ast::PredefinedIntegerType::Integer:
      kind = moore::IntType::Integer;
      break;
    case slang::ast::PredefinedIntegerType::Byte:
      kind = moore::IntType::Byte;
      break;
    case slang::ast::PredefinedIntegerType::Time:
      kind = moore::IntType::Time;
      break;
    }

    std::optional<moore::Sign> sign =
        type.isSigned ? moore::Sign::Signed : moore::Sign::Unsigned;
    if (sign == moore::IntType::getDefaultSign(kind))
      sign = {};

    return moore::IntType::get(context.getContext(), kind, sign);
  }

  Type visit(const slang::ast::PackedArrayType &type) {
    auto innerType = type.elementType.visit(*this);
    if (!innerType)
      return {};
    auto packedInnerType = dyn_cast<moore::PackedType>(innerType);
    if (!packedInnerType) {
      mlir::emitError(loc, "packed array with unpacked elements; ")
          << type.elementType.toString() << " is unpacked";
      return {};
    }
    return moore::PackedRangeDim::get(
        packedInnerType, moore::Range(type.range.left, type.range.right));
  }

  Type visit(const slang::ast::AssociativeArrayType &type) {
    auto innerType = type.elementType.visit(*this);
    if (!innerType)
      return {};
    auto unpackedInnerType = dyn_cast<moore::UnpackedType>(innerType);
    if (!unpackedInnerType) {
      mlir::emitError(loc, "dynamic unpacked array; ")
          << type.elementType.toString() << "is associative unpacked";
      return {};
    }
    auto indexType = type.indexType->visit(*this);
    if (!indexType)
      return {};
    auto unpackedIndexType = dyn_cast<moore::UnpackedType>(indexType);
    if (!unpackedIndexType)
      return {};
    return moore::UnpackedAssocDim::get(unpackedInnerType, unpackedIndexType);
  }

  Type visit(const slang::ast::FixedSizeUnpackedArrayType &type) {
    auto innerType = type.elementType.visit(*this);
    if (!innerType)
      return {};
    auto unpackedInnerType = dyn_cast<moore::UnpackedType>(innerType);
    if (!unpackedInnerType) {
      mlir::emitError(loc, "dynamic unpacked array; ")
          << type.elementType.toString() << " is fixed size unpacked";
      return {};
    }
    return moore::UnpackedRangeDim::get(
        unpackedInnerType, moore::Range(type.range.left, type.range.right));
  }

  Type visit(const slang::ast::DynamicArrayType &type) {
    auto innerType = type.elementType.visit(*this);
    if (!innerType)
      return {};
    auto unpackedInnerType = dyn_cast<moore::UnpackedType>(innerType);
    if (!unpackedInnerType) {
      mlir::emitError(loc, "fixed size unpacked array; ")
          << type.elementType.toString() << " is dynamic size unpacked";
      return {};
    }
    return moore::UnpackedUnsizedDim::get(unpackedInnerType);
  }

  /// Emit an error for all other types.
  template <typename T>
  Type visit(T &&node) {
    mlir::emitError(loc, "unsupported type: ")
        << node.template as<slang::ast::Type>().toString();
    return {};
  }
};
} // namespace

Type Context::convertType(const slang::ast::Type &type, LocationAttr loc) {
  if (!loc)
    loc = convertLocation(type.location);
  return type.visit(TypeVisitor(*this, loc));
}

Type Context::convertType(const slang::ast::DeclaredType &type) {
  LocationAttr loc;
  if (auto *ts = type.getTypeSyntax())
    loc = convertLocation(ts->sourceRange().start());
  return convertType(type.getType(), loc);
}
