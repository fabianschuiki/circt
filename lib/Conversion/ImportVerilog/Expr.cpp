
#include "ImportVerilogInternals.h"
#include "circt/Dialect/Moore/MooreTypes.h"
#include "mlir/IR/Value.h"
#include "slang/ast/ASTVisitor.h"
#include "slang/syntax/SyntaxVisitor.h"
#include <cstddef>
#include <slang/ast/ASTContext.h>
#include "slang/ast/types/AllTypes.h"
#include <slang/ast/EvalContext.h>
#include <slang/ast/Expression.h>
#include <slang/ast/expressions/LiteralExpressions.h>
#include <slang/ast/types/DeclaredType.h>

using namespace circt;
using namespace ImportVerilog;


namespace {
struct ExprVisitor {
  slang::ast::EvalContext &evalctx;
  Location loc;
  ExprVisitor(slang::ast::EvalContext &evalctx, Location loc) : evalctx(evalctx), loc(loc) {}

  slang::SVInt visit(const slang::ast::IntegerLiteral &lit){
    return lit.eval(evalctx).integer();
  }
  
  

  /// Emit an error for all other types.
  template <typename T>
  slang::SVInt visitInvalid(T &&node) {
    mlir::emitError(loc, "unsupported expr: ");
    return NULL;
  }
  /// Emit an error for all other types.
  template <typename T>
  slang::SVInt visit(T &&node) {
    mlir::emitError(loc, "unsupported expr: ");
    return NULL;
  }
};
} // namespace

slang::SVInt Context::convertExpr(const slang::ast::Expression &expr, LocationAttr loc) {
  if (!loc)
    loc = convertLocation(expr.sourceRange.start());
  slang::ast::EvalContext evalctx(compilation);
  return expr.visit(ExprVisitor(evalctx,loc));
}
