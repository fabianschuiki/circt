//===- tinc.cpp - The Tin HDL Compiler ------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the entry point for the Tin compiler.
//
//===----------------------------------------------------------------------===//

#include "tin/Lexer.h"
#include "tin/Parser.h"

#include "circt/Support/LLVM.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/MLIRContext.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include <memory>
#include <system_error>

using namespace circt;
using namespace tin;
namespace cl = llvm::cl;

//===----------------------------------------------------------------------===//
// Command Line Options
//===----------------------------------------------------------------------===//

struct Opt {
  cl::opt<std::string> inputFilename{cl::Positional, cl::desc("<input file>"),
                                     cl::init("-"), cl::value_desc("filename")};
};
Opt opt;

//===----------------------------------------------------------------------===//
// Driver
//===----------------------------------------------------------------------===//

LogicalResult executeCompiler(MLIRContext *context) {
  llvm::SourceMgr sourceMgr;
  mlir::SourceMgrDiagnosticHandler sourceMgrHandler(sourceMgr, context);

  // Open the source file.
  auto fileOrError = llvm::MemoryBuffer::getFileOrSTDIN(opt.inputFilename);
  if (auto error = fileOrError.getError()) {
    llvm::errs() << "error: unable to open input file: " << error.message()
                 << "\n";
    return failure();
  }

  // Create a lexer.
  Lexer lexer(context, fileOrError.get()->getBuffer(),
              StringAttr::get(context, opt.inputFilename));

  // Create a parser.
  Parser parser(lexer);
  if (failed(parser.parseRoot()))
    return failure();

  // while (auto token = lexer.next()) {
  //   if (token.kind == TokenKind::error)
  //     return failure();
  //   llvm::errs() << "- " << token << "\n";
  // }

  // auto moduleAST = parseInputFile(opt.inputFilename);
  // if (failed(moduleAST))
  //   return failure();
  return success();
}

int main(int argc, char **argv) {
  mlir::registerMLIRContextCLOptions();
  cl::ParseCommandLineOptions(argc, argv, "Tin HDL compiler\n");
  MLIRContext context;
  auto result = executeCompiler(&context);
  exit(failed(result));
}
