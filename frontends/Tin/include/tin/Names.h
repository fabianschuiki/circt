//===- Names.h - Name resolution for the Tin language ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once
#include "tin/AST.h"

namespace circt {
namespace tin {

LogicalResult resolveNames(AST &ast);

} // namespace tin
} // namespace circt
