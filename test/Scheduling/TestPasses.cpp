//===- TestPasses.cpp - Test passes for scheduling algorithms -===============//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements test passes for scheduling algorithms.
//
//===----------------------------------------------------------------------===//

#include "circt/Scheduling/Algorithms.h"

#include "mlir/IR/Builders.h"
#include "mlir/Pass/Pass.h"

using namespace mlir;
using namespace circt;
using namespace circt::scheduling;

//===----------------------------------------------------------------------===//
// ASAPScheduler
//===----------------------------------------------------------------------===//

namespace {
struct TestASAPSchedulerPass
    : public PassWrapper<TestASAPSchedulerPass, FunctionPass> {
  void runOnFunction() override;
};
} // anonymous namespace

void TestASAPSchedulerPass::runOnFunction() {
  auto func = getFunction();
  OpBuilder builder(func.getContext());

  Problem prob(func);

  // set up catch-all operator type with unit latency
  auto unitOpr = prob.getOrInsertOperatorType("unit");
  prob.setLatency(unitOpr, 1);

  // parse additional operator type information attached to the test case
  if (auto attr = func->getAttrOfType<ArrayAttr>("operatortypes")) {
    for (auto oprAttr : attr.getAsRange<DictionaryAttr>()) {
      auto name = oprAttr.getAs<StringAttr>("name");
      auto latency = oprAttr.getAs<IntegerAttr>("latency");
      if (!(name && latency))
        continue;

      auto opr = prob.getOrInsertOperatorType(name.getValue());
      prob.setLatency(opr, latency.getInt());
    }
  }

  // construct problem (consider only the first block)
  for (auto &op : func.getBlocks().front().getOperations()) {
    prob.insertOperation(&op);

    if (auto oprRefAttr = op.getAttrOfType<StringAttr>("opr")) {
      auto opr = prob.getOrInsertOperatorType(oprRefAttr.getValue());
      prob.setLinkedOperatorType(&op, opr);
    } else {
      prob.setLinkedOperatorType(&op, unitOpr);
    }
  }

  // parse auxiliary dependences in the testcase, encoded as an array of
  // 2-element arrays of integer attributes (see `test_asap.mlir`)
  if (auto attr = func->getAttrOfType<ArrayAttr>("auxdeps")) {
    auto &ops = prob.getOperations();
    for (auto auxDepAttr : attr.getAsRange<ArrayAttr>()) {
      if (auxDepAttr.size() != 2)
        continue;
      auto fromIdxAttr = auxDepAttr[0].dyn_cast<IntegerAttr>();
      auto toIdxAttr = auxDepAttr[1].dyn_cast<IntegerAttr>();
      if (!fromIdxAttr || !toIdxAttr)
        continue;
      unsigned fromIdx = fromIdxAttr.getInt();
      unsigned toIdx = toIdxAttr.getInt();
      if (fromIdx >= ops.size() || toIdx >= ops.size())
        continue;

      // finally, we have two integer indices in range of the operations list
      if (failed(prob.insertDependence(
              std::make_pair(ops[fromIdx], ops[toIdx])))) {
        func->emitError("inserting aux dependence failed");
        return signalPassFailure();
      }
    }
  }

  if (failed(prob.check())) {
    func->emitError("problem check failed");
    return signalPassFailure();
  }

  if (failed(scheduleASAP(prob))) {
    func->emitError("scheduling failed");
    return signalPassFailure();
  }

  if (failed(prob.verify())) {
    func->emitError("schedule verification failed");
    return signalPassFailure();
  }

  for (auto *op : prob.getOperations()) {
    unsigned startTime = *prob.getStartTime(op);
    op->emitRemark("start time = " + std::to_string(startTime));
  }
}

//===----------------------------------------------------------------------===//
// Pass registration
//===----------------------------------------------------------------------===//

namespace circt {
namespace test {
void registerSchedulingTestPasses() {
  PassRegistration<TestASAPSchedulerPass> asapTester(
      "test-asap-scheduler", "Emit ASAP scheduler's solution as remarks");
}
} // namespace test
} // namespace circt
