//===- PrintKnownValues.cpp - Analyze and print known values --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "PassDetail.h"
#include "circt/Dialect/Comb/CombOps.h"
#include "circt/Dialect/HW/HWInstanceGraph.h"
#include "circt/Dialect/SV/SVOps.h"
#include "circt/Dialect/Seq/SeqOps.h"
#include "circt/Transforms/Passes.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/Threading.h"
#include "mlir/Pass/Pass.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "known-values"

using namespace mlir;
using namespace circt;
using namespace hw;

using llvm::SmallMapVector;

namespace {

struct KnownBits {
  /// A constant value.
  IntegerAttr bits;
  /// A mask of bits in the constant value that are actually known.
  IntegerAttr known;

  KnownBits() {}
  KnownBits(IntegerAttr bits, IntegerAttr known) : bits(bits), known(known) {}
  explicit KnownBits(IntegerAttr fullyKnownValue) : bits(fullyKnownValue) {
    if (fullyKnownValue)
      known = IntegerAttr::get(
          fullyKnownValue.getContext(),
          APSInt(APInt::getAllOnes(fullyKnownValue.getValue().getBitWidth())));
  }

  explicit operator bool() const { return bits && known; }

  bool operator==(const KnownBits &other) const {
    return bits == other.bits && known == other.known;
  }
};

class KnownBitsFoldResult : public std::variant<KnownBits, Value> {
public:
  using std::variant<KnownBits, Value>::variant;

  operator bool() const {
    if (auto *known = std::get_if<KnownBits>(this))
      return bool(*known);
    return bool(std::get<Value>(*this));
  }
};

} // namespace

static KnownBitsFoldResult fold(Operation *op, ArrayRef<KnownBits> operands) {
  return TypeSwitch<Operation *, KnownBitsFoldResult>(op)
      .Case<comb::ConcatOp>([&](auto op) -> KnownBitsFoldResult {
        auto width = op.getType().getIntOrFloatBitWidth();
        APInt bits(width, 0);
        APInt known(width, 0);
        for (auto [input, operand] : llvm::zip(op.getInputs(), operands)) {
          auto operandWidth = input.getType().getIntOrFloatBitWidth();
          bits <<= operandWidth;
          known <<= operandWidth;
          if (operand) {
            bits |= operand.bits.getValue().zext(width);
            known |= operand.known.getValue().zext(width);
          }
        }
        auto *context = op->getContext();
        return KnownBits(IntegerAttr::get(context, APSInt(bits)),
                         IntegerAttr::get(context, APSInt(known)));
      })
      .Default({});
}

static LogicalResult fold(Operation *op, ArrayRef<KnownBits> operands,
                          SmallVectorImpl<KnownBitsFoldResult> &results) {
  if (!llvm::any_of(operands, [](auto operand) { return bool(operand); }))
    return failure();
  if (op->getNumResults() == 0)
    return failure();
  if (op->getNumResults() == 1) {
    auto result = fold(op, operands);
    auto *knownResult = std::get_if<KnownBits>(&result);
    if (!result || (knownResult && knownResult->known.getValue().isZero()))
      return failure();
    results.push_back(result);
    return success();
  }
  return failure();
}

namespace {
struct ModuleContext {
  /// The module for which this context captures value assignments.
  HWModuleOp module;
  /// The instance op that created this context by instantiating the `module`.
  /// Null for the root `ModuleContext`.
  InstanceOp instance;
  /// The parent context that contained the `instance`.
  ModuleContext *parent = nullptr;
  /// Value assignments within this context.
  DenseMap<Value, KnownBits> values;
  /// Contexts for submodules instantiated within this module.
  SmallMapVector<InstanceOp, ModuleContext *, 4> subcontexts;
};

struct HierarchyAnalysis {
  HierarchyAnalysis(InstanceGraph &instanceGraph, HWModuleOp rootModule)
      : instanceGraph(instanceGraph), rootModule(rootModule) {}
  void run();

  InstanceGraph &instanceGraph;
  HWModuleOp rootModule;
  ModuleContext *rootContext = nullptr;
  llvm::SpecificBumpPtrAllocator<ModuleContext> contextAllocator;
  unsigned numContexts = 0;
  unsigned numPrimaryConstants = 0;
  size_t numIterations = 0;
};
} // namespace

void HierarchyAnalysis::run() {
  LLVM_DEBUG(llvm::dbgs() << "Analyzing known values in "
                          << rootModule.getModuleName() << "\n");

  // Initialization:
  // - Create context for root module
  // - Push onto worklist
  // - Run through initialization worklist, for each:
  // - For each constant-like op in the module, update lattice value
  // - For each instance op in the module, create context and add to
  //   initialization worklist

  rootContext = new (contextAllocator.Allocate()) ModuleContext;
  rootContext->module = rootModule;
  SetVector<ModuleContext *> initWorklist;
  initWorklist.insert(rootContext);
  ++numContexts;

  SetVector<std::pair<ModuleContext *, Operation *>> worklist;
  while (!initWorklist.empty()) {
    auto *context = initWorklist.pop_back_val();
    for (auto &op : context->module.getBodyBlock()->getOperations()) {
      if (op.hasTrait<OpTrait::ConstantLike>()) {
        worklist.insert({context, &op});
        ++numPrimaryConstants;
      } else if (auto instOp = dyn_cast<InstanceOp>(&op)) {
        if (auto referencedModule = dyn_cast_or_null<HWModuleOp>(
                &*instanceGraph.getReferencedModule(instOp))) {
          auto &subcontext = context->subcontexts[instOp];
          subcontext = new (contextAllocator.Allocate()) ModuleContext;
          subcontext->module = referencedModule;
          subcontext->instance = instOp;
          subcontext->parent = context;
          initWorklist.insert(subcontext);
          ++numContexts;
        }
      }
    }
  }

  LLVM_DEBUG(llvm::dbgs() << "- " << numContexts << " instances total\n");
  LLVM_DEBUG(llvm::dbgs() << "- " << numPrimaryConstants
                          << " initial constants\n");

  auto updateValue = [&](ModuleContext *context, Value value, KnownBits attr) {
    // Check if the folded value is different than what we've recorded before.
    // If it isn't, bail already.
    auto &valueSlot = context->values[value];
    if (valueSlot == attr)
      return;
    valueSlot = attr;

    // Add the users of this value to the worklist.
    for (auto *user : value.getUsers())
      worklist.insert({context, user});
  };

  // Perform basic constant propagation.
  while (!worklist.empty()) {
    auto [context, op] = worklist.pop_back_val();
    ++numIterations;

    // If this is a wire, simply forward its input value.
    if (auto wireOp = dyn_cast<WireOp>(op)) {
      updateValue(context, wireOp, context->values.lookup(wireOp.getInput()));
      continue;
    }

    // If this is an instance, copy its operands into its subcontext.
    if (auto instOp = dyn_cast<InstanceOp>(op)) {
      auto *subcontext = context->subcontexts.lookup(instOp);
      if (!subcontext)
        continue;
      for (auto [instOperand, moduleArg] : llvm::zip(
               instOp.getInputs(), subcontext->module.getBody().getArguments()))
        updateValue(subcontext, moduleArg, context->values.lookup(instOperand));
      continue;
    }

    // If this is the output op in a module, copy its operands into the parent
    // context.
    if (auto outputOp = dyn_cast<OutputOp>(op)) {
      if (context->instance) {
        for (auto [instResult, outputOperand] :
             llvm::zip(context->instance.getResults(), outputOp.getOutputs()))
          updateValue(context->parent, instResult,
                      context->values.lookup(outputOperand));
      }
      continue;
    }

    // If this is a register without reset, or where the reset value matches the
    // constant input value, forward its input value.
    if (auto regOp = dyn_cast<seq::FirRegOp>(op)) {
      auto nextValue = context->values.lookup(regOp.getNext());
      auto resetValue = context->values.lookup(regOp.getResetValue());
      if (nextValue && (!regOp.getReset() || nextValue == resetValue)) {
        updateValue(context, regOp, nextValue);
        continue;
      }
    }

    // If this is a memory with constant write data, forward that value to its
    // read ports.
    if (auto writeOp = dyn_cast<seq::FirMemWriteOp>(op)) {
      auto value = context->values.lookup(writeOp.getData());
      if (value) {
        LLVM_DEBUG(llvm::dbgs() << "  - Constant write of " << value.bits
                                << ": " << writeOp << "\n");
        continue;
      }
    }
    if (auto writeOp = dyn_cast<seq::FirMemReadWriteOp>(op)) {
      auto value = context->values.lookup(writeOp.getWriteData());
      if (value) {
        LLVM_DEBUG(llvm::dbgs() << "  - Constant write of " << value.bits
                                << ": " << writeOp << "\n");
        continue;
      }
    }

    // Collect all of the constant operands feeding into this operation.
    SmallVector<Attribute, 8> completeConstants;
    SmallVector<KnownBits, 8> partialConstants;
    completeConstants.reserve(op->getNumOperands());
    for (Value operand : op->getOperands()) {
      auto attr = context->values.lookup(operand);
      completeConstants.push_back(attr && attr.known.getValue().isAllOnes()
                                      ? attr.bits
                                      : IntegerAttr{});
      partialConstants.push_back(attr);
    }

    // Simulate the result of folding this operation to a constant.
    SmallVector<OpFoldResult, 8> foldCompleteResults;
    SmallVector<KnownBitsFoldResult, 8> foldPartialResults;
    foldCompleteResults.reserve(op->getNumResults());
    foldPartialResults.reserve(op->getNumResults());
    if (failed(op->fold(completeConstants, foldCompleteResults))) {
      if (failed(fold(op, partialConstants, foldPartialResults)))
        continue;
    } else {
      for (auto folded : foldCompleteResults) {
        if (auto attr = dyn_cast<Attribute>(folded))
          foldPartialResults.push_back(
              KnownBits(dyn_cast_or_null<IntegerAttr>(attr)));
        else
          foldPartialResults.push_back(cast<Value>(folded));
      }
    }
    if (foldPartialResults.size() != op->getNumResults())
      continue;

    // Record the updated values.
    for (auto [result, folded] :
         llvm::zip(op->getResults(), foldPartialResults)) {
      KnownBits foldedAttr;
      if (auto *attr = std::get_if<KnownBits>(&folded))
        foldedAttr = *attr;
      else
        foldedAttr = context->values.lookup(std::get<Value>(folded));
      updateValue(context, result, foldedAttr);
    }
  }

  LLVM_DEBUG(llvm::dbgs() << "- " << numIterations << " iterations overall\n");
}

static void printConstPorts(ModuleContext *context,
                            SmallVectorImpl<char> &hierarchyPrefix) {
  if (!hierarchyPrefix.empty())
    hierarchyPrefix.push_back('.');
  size_t baseLen = hierarchyPrefix.size();

  auto printIfConstant = [&](Value value, StringRef name) {
    auto attr = context->values.lookup(value);
    auto intType = dyn_cast<IntegerType>(value.getType());
    if (!attr || !intType)
      return;
    SmallString<16> buffer;

    // Handle the trivial case where the full value is known.
    if (attr.known.getValue().isAllOnes()) {
      attr.bits.getValue().toStringUnsigned(buffer, 16);
      llvm::outs() << "-constant " << hierarchyPrefix << name << " "
                   << intType.getWidth() << "'h" << buffer << "\n";
      return;
    }

    // Handle the case where only part of the value is known.
    APInt bits = attr.bits.getValue();
    APInt known = attr.known.getValue();
    unsigned offset = 0;
    while (!known.isZero()) {
      // Skip unknown bits.
      unsigned skipBits = known.countTrailingZeros();
      offset += skipBits;
      bits.lshrInPlace(skipBits);
      known.lshrInPlace(skipBits);

      // Print known bits.
      unsigned considerBits = known.countTrailingOnes();
      llvm::outs() << "-constant " << hierarchyPrefix << name << "[";
      if (considerBits == 1)
        llvm::outs() << offset;
      else
        llvm::outs() << (offset + considerBits - 1) << ":" << offset;
      bits.trunc(considerBits).toStringUnsigned(buffer, 16);
      llvm::outs() << "] " << considerBits << "'h" << buffer << "\n";
      buffer.clear();
      offset += considerBits;
      bits.lshrInPlace(considerBits);
      known.lshrInPlace(considerBits);
    }
  };

  // Constant inputs.
  auto modType = context->module.getHWModuleType();
  for (auto arg : context->module.getBody().getArguments())
    printIfConstant(arg, modType.getInputName(arg.getArgNumber()));

  // Constant outputs.
  for (auto &operand :
       context->module.getBodyBlock()->getTerminator()->getOpOperands())
    printIfConstant(operand.get(),
                    modType.getOutputName(operand.getOperandNumber()));

  // Constant wires and registers.
  for (auto &op : *context->module.getBodyBlock()) {
    if (!isa<hw::WireOp, sv::WireOp, seq::CompRegOp, seq::FirRegOp>(&op))
      continue;
    if (op.getNumResults() != 1)
      continue;
    StringAttr nameAttr;
    nameAttr = op.getAttrOfType<StringAttr>("name");
    if (!nameAttr)
      nameAttr = op.getAttrOfType<StringAttr>("sv.namehint");
    if (!nameAttr || nameAttr.getValue().empty())
      continue;
    printIfConstant(op.getResult(0), nameAttr.getValue());
  }

  // Descend into submodules.
  for (auto [instOp, subcontext] : context->subcontexts) {
    auto instName = subcontext->instance.getInstanceName();
    hierarchyPrefix.resize(baseLen);
    hierarchyPrefix.append(instName.begin(), instName.end());
    printConstPorts(subcontext, hierarchyPrefix);
  }
}

namespace {
struct PrintKnownValuesPass
    : public PrintKnownValuesBase<PrintKnownValuesPass> {
  void runOnOperation() override;
};
} // namespace

void PrintKnownValuesPass::runOnOperation() {
  auto &instanceGraph = getAnalysis<InstanceGraph>();
  auto &rootNode = *instanceGraph.getTopLevelNode();
  for (auto *rootRecord : rootNode) {
    auto moduleName = rootRecord->getTarget()->getModule().getModuleName();
    if (!topModuleName.empty()) {
      if (moduleName != topModuleName)
        continue;
    } else {
      llvm::outs() << "module " << moduleName << "\n";
    }
    auto module = dyn_cast<HWModuleOp>(
        rootRecord->getTarget()->getModule().getOperation());
    if (!module)
      continue;
    HierarchyAnalysis analysis(instanceGraph, module);
    analysis.run();
    SmallString<128> prefix;
    printConstPorts(analysis.rootContext, prefix);
  }
}

std::unique_ptr<Pass> circt::createPrintKnownValuesPass() {
  return std::make_unique<PrintKnownValuesPass>();
}
