//===- TimingControl.cpp - Slang timing control conversion ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "slang/ast/TimingControl.h"
#include "ImportVerilogInternals.h"

using namespace circt;
using namespace ImportVerilog;

namespace {
struct TimingCtrlVisitor {
  Context &context;
  Location loc;
  OpBuilder &builder;

  LogicalResult visit(const slang::ast::SignalEventControl &ctrl) {
    auto loc = context.convertLocation(ctrl.sourceRange.start());
    auto input = context.convertExpression(ctrl.expr);
    builder.create<moore::EventControlOp>(
        loc, static_cast<moore::Edge>(ctrl.edge), input);
    return success();
  }

  LogicalResult visit(const slang::ast::ImplicitEventControl &ctrl) {
    return success();
  }

  LogicalResult visit(const slang::ast::EventListControl &ctrl) {
    for (auto *event : ctrl.as<slang::ast::EventListControl>().events) {
      if (failed(context.convertTimingControl(*event)))
        return failure();
    }
    return success();
  }

  /// Emit an error for all other timing controls.
  template <typename T>
  LogicalResult visit(T &&node) {
    mlir::emitError(loc, "unspported timing control: ")
        << slang::ast::toString(node.kind);
    return failure();
  }

  LogicalResult visitInvalid(const slang::ast::TimingControl &ctrl) {
    mlir::emitError(loc, "invalid timing control");
    return failure();
  }
};
} // namespace

LogicalResult
Context::convertTimingControl(const slang::ast::TimingControl &timingControl) {
  auto loc = convertLocation(timingControl.sourceRange.start());
  TimingCtrlVisitor visitor{*this, loc, builder};
  return timingControl.visit(visitor);
}
