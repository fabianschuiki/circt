//===- Names.cpp - Name resolution for the Tin language -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "tin/Names.h"

#include "mlir/IR/Diagnostics.h"
#include "llvm/ADT/ScopeExit.h"

using namespace circt;
using namespace tin;

static Location getLoc(ast::Binding binding) {
  return TypeSwitch<ast::Binding, Location>(binding).Case<ast::ModPort>(
      [](auto node) { return node->loc; });
}

namespace {
struct Scope {
  SmallDenseMap<StringAttr, ast::Binding> names;
};

struct Resolver : public ast::Visitor<Resolver> {
  using ast::Visitor<Resolver>::visit;
  using ast::Visitor<Resolver>::visitDefault;
  SmallVector<Scope> scopes;
  bool anyErrors = false;

  /// Declare a name in the current scope. Reports an error if the name has
  /// already been defined.
  void declareName(StringAttr name, ast::Binding node) {
    auto &slot = scopes.back().names[name];
    if (slot) {
      auto nodeLoc = getLoc(node);
      auto slotLoc = getLoc(slot);
      auto d = mlir::emitError(nodeLoc)
               << "name `" << name.getValue() << "` already defined";
      d.attachNote(slotLoc)
          << "previous definition of `" << name.getValue() << "` was here";
      anyErrors = true;
      return;
    }
    slot = node;
  }

  /// Resolve a name in the current scope or one of its parents. Reports an
  /// error for the given location if the name cannot be found.
  ast::Binding resolveName(StringAttr name, Location loc) {
    for (auto &scope : llvm::reverse(scopes))
      if (auto binding = scope.names.lookup(name))
        return binding;
    auto d = mlir::emitError(loc) << "unknown name `" << name.getValue() << "`";
    anyErrors = true;

    // Try to suggest a name that the user might have intended to type.
    for (auto &scope : llvm::reverse(scopes)) {
      StringRef bestName;
      unsigned bestDistance = 3;
      LocationAttr bestLoc;
      for (auto &[candidate, binding] : scope.names) {
        unsigned distance = name.getValue().edit_distance(candidate.getValue(),
                                                          true, bestDistance);
        if (distance > bestDistance)
          continue;
        if (distance == bestDistance)
          if (candidate.getValue() > bestName)
            continue;
        bestName = candidate;
        bestDistance = distance;
        bestLoc = getLoc(binding);
      }
      if (bestLoc) {
        d.attachNote(bestLoc)
            << "did you mean `" << bestName << "` defined here?";
        break;
      }
    }
    return {};
  }

  struct Resolve {};
  struct Declare {};

  LogicalResult resolve(AST &ast) {
    for (auto *root : ast.roots) {
      visit(*root, Resolve{});
    }
    return failure(anyErrors);
  }

  void visit(ast::Root &root, Resolve) {
    // Create a scope for this file.
    scopes.emplace_back();
    auto guard = llvm::make_scope_exit([this] { scopes.pop_back(); });

    // Resolve everything in the file.
    root.walk(*this, Resolve{});
  }

  void visit(ast::ModItem &item, Resolve) {
    // Create a scope for the module.
    scopes.emplace_back();
    auto guard = llvm::make_scope_exit([this] { scopes.pop_back(); });

    // Collect all name declarations in the module first.
    item.walk(*this, Declare{});
    if (anyErrors)
      return;

    // Resolve all names in child nodes.
    item.walk(*this, Resolve{});
  }

  void visit(ast::IdentExpr &expr, Resolve) {
    expr.binding = resolveName(expr.name, expr.loc);
  }

  // When resolving names in AST nodes, descend into child nodes by default.
  template <typename T>
  void visitDefault(T &node, Resolve) {
    node.walk(*this, Resolve{});
  }

  void visit(ast::ModPort &port, Declare) { declareName(port.name, &port); }
};
} // namespace

LogicalResult tin::resolveNames(AST &ast) { return Resolver().resolve(ast); }
