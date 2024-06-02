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

#include "tin/Codegen.h"
#include "tin/Lexer.h"
#include "tin/Parser.h"

#include "circt/Support/LLVM.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Support/FileUtilities.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/ToolOutputFile.h"
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

  cl::opt<std::string> outputFilename{
      "o", cl::desc("Output filename (`-` for stdout)"),
      cl::value_desc("filename"), cl::init("-")};
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

  // Create a parser and parse the input into an AST.
  AST ast;
  Parser parser(lexer, ast);
  auto *root = parser.parseRoot();
  if (!root)
    return failure();
  ast.roots.push_back(root);

  // Convert the AST to MLIR.
  auto module = convertToMLIR(context, ast);
  if (!module)
    return failure();

  // Open the output file.
  std::string errorMessage;
  auto outputFile = mlir::openOutputFile(opt.outputFilename, &errorMessage);
  if (!outputFile) {
    mlir::emitError(UnknownLoc::get(context)) << errorMessage << "\n";
    return failure();
  }

  // Print the final MLIR.
  module->print(outputFile->os());
  outputFile->keep();
  return success();
}

int main(int argc, char **argv) {
  mlir::registerMLIRContextCLOptions();
  cl::ParseCommandLineOptions(argc, argv, "Tin HDL compiler\n");
  MLIRContext context;
  auto result = executeCompiler(&context);
  exit(failed(result));
}
