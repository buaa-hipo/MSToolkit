#include <gvarp/llvm-pass/GVARPPass.hpp>
#include <gvarp/llvm-pass/GVARPAnalysis.hpp>

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Transforms/Instrumentation.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils.h>

static llvm::cl::opt<bool> RemoveDuplicates("gvarp-remove-duplicates",
                                       llvm::cl::desc("Attempts to merge identical functions."),
                                       llvm::cl::init(false),
                                       llvm::cl::value_desc("boolean flag"));

namespace gvarp {

  GVARPPass::GVARPPass():
    ModulePass(ID)
  {}

  llvm::StringRef GVARPPass::getPassName() const
  {
    return "GVARPTaintPass";
  }

  bool GVARPPass::runOnModule(llvm::Module &m)
  {
    llvm::legacy::PassManager PM;

    // correlated-propagation
    PM.add(llvm::createInstructionNamerPass());
    //PM.add(llvm::createMetaRenamerPass());
    PM.add(llvm::createCorrelatedValuePropagationPass());
    // mem2reg pass
    PM.add(llvm::createPromoteMemoryToRegisterPass());
    // loop-simplify
    PM.add(llvm::createLoopSimplifyPass());
    // merge-functions
    if(RemoveDuplicates)
      PM.add(llvm::createMergeFunctionsPass());
    // our pass 
    PM.add(createGVARPAnalysisPass());

    PM.run(m);

    // Module is always modified by dfsan.
    return true;
  }

}

char gvarp::GVARPPass::ID = 0;
static llvm::RegisterPass<gvarp::GVARPPass> register_pass(
  "gvarp",
  "Apply taint-based critical parameter identification for fixed workload kernels",
  false /* Only looks at CFG */,
  false /* Analysis Pass */
);

// Allow running dynamically through frontend such as Clang
void addGVARP(const llvm::PassManagerBuilder &Builder,
                        llvm::legacy::PassManagerBase &PM) {
  PM.add(gvarp::createGVARPAnalysisPass());
}

// run after optimizations
llvm::RegisterStandardPasses SOpt(llvm::PassManagerBuilder::EP_OptimizerLast,
                            addGVARP);