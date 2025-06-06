#include "triton/Dialect/Triton/IR/Dialect.h"
#include "triton/Dialect/Triton/IR/Interfaces.h"
#include "triton/Dialect/Triton/IR/Types.h"

#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/UB/IR/UBOps.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/ADT/TypeSwitch.h"

#include "triton/Dialect/Triton/IR/AttrInterfaces.cpp.inc"
#include "triton/Dialect/Triton/IR/Dialect.cpp.inc"
#include "triton/Dialect/Triton/IR/OpInterfaces.cpp.inc"

using namespace mlir;
using namespace mlir::triton;

//===----------------------------------------------------------------------===//
// TritonDialect Dialect Interfaces
//===----------------------------------------------------------------------===//

bool TritonInlinerInterface::isLegalToInline(Operation *call,
                                             Operation *callable,
                                             bool wouldBeCloned) const {
  auto funcOp = dyn_cast<triton::FuncOp>(callable);
  if (!funcOp)
    return true;
  if (funcOp->hasAttr("noinline"))
    return !funcOp->getAttrOfType<BoolAttr>("noinline").getValue();
  return true;
}

/// Handle the given inlined terminator by replacing it with a new operation
/// as necessary.
void TritonInlinerInterface::handleTerminator(Operation *op,
                                              Block *newDest) const {
  // Only return needs to be handled here.
  auto returnOp = dyn_cast<triton::ReturnOp>(op);
  if (!returnOp)
    return;

  // Replace the return with a branch to the dest.
  OpBuilder builder(op);
  builder.create<mlir::cf::BranchOp>(op->getLoc(), newDest,
                                     returnOp.getOperands());
  op->erase();
}

/// Handle the given inlined terminator by replacing it with a new operation
/// as necessary.
void TritonInlinerInterface::handleTerminator(Operation *op,
                                              ValueRange valuesToRepl) const {
  // Only return needs to be handled here.
  auto returnOp = cast<triton::ReturnOp>(op);

  // Replace the values directly with the return operands.
  assert(returnOp.getNumOperands() == valuesToRepl.size());
  for (const auto &it : llvm::enumerate(returnOp.getOperands()))
    valuesToRepl[it.index()].replaceAllUsesWith(it.value());
}

void TritonDialect::initialize() {
  registerTypes();

  addOperations<
#define GET_OP_LIST
#include "triton/Dialect/Triton/IR/Ops.cpp.inc"
      >();

  // We can also add interface here.
  addInterfaces<TritonInlinerInterface>();
}

Operation *TritonDialect::materializeConstant(OpBuilder &builder,
                                              Attribute value, Type type,
                                              Location loc) {
  return arith::ConstantOp::materialize(builder, value, type, loc);
}
