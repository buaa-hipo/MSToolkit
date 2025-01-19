#ifndef __GVARP_TAINT_PASS_HPP__
#define __GVARP_TAINT_PASS_HPP__

#include <llvm/ADT/Optional.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/Pass.h>

#include <fstream>
#include <unordered_set>
#include <unordered_map>


namespace gvarp {

  struct GVARPPass : public llvm::ModulePass
  {
    static char ID;

    GVARPPass();

    llvm::StringRef getPassName() const override;
    bool runOnModule(llvm::Module & f) override;
  };

}

#endif