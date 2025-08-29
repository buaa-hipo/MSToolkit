#ifndef __GVARP_TAINT_ANALYSIS_HPP__
#define __GVARP_TAINT_ANALYSIS_HPP__

#include <llvm/ADT/Optional.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/Pass.h>

#include <fstream>
#include <unordered_set>
#include <unordered_map>

#include <gvarp/llvm-pass/ParameterFinder.hpp>

namespace llvm {
    class Function;
    class CallGraph;
    class CallGraphNode;
    class LoopInfo;
    class Loop;
    class ScalarEvolution;
}

namespace gvarp {
    struct GVARPAnalysis : public llvm::ModulePass
    {
        static char ID;
        std::fstream log;
        llvm::Module * m;
        llvm::CallGraph * cgraph;
        llvm::LoopInfo * linfo;
        // FunctionParameters parameters;
        // FunctionDatabase database;

        std::vector<llvm::Function *> parent_functions;
        std::unordered_map<llvm::Function*, std::tuple<bool, ValueSet*> > func_cache;
        std::unordered_map<llvm::Loop*, std::tuple<bool, ValueSet*> > loop_cache;
        std::ofstream unknown;

        GVARPAnalysis():
            ModulePass(ID),
            m(nullptr),
            cgraph(nullptr),
            linfo(nullptr)
        {
        }

        ~GVARPAnalysis() {}

        llvm::StringRef getPassName() const override;
        void getAnalysisUsage(llvm::AnalysisUsage & AU) const override;
        bool runOnFunction(llvm::Function & f, int override_counter = -1);
        std::tuple<bool, ValueSet*> analyzeFunction(llvm::Function & f, llvm::CallGraphNode * cg_node,
                int override_counter = -1);
        bool runOnModule(llvm::Module & f) override;
        bool is_analyzable(llvm::Module & m, llvm::Function & f);
        std::tuple<bool, ValueSet*> analyzeLoop(llvm::Loop *l, llvm::ScalarEvolution & scev);
        bool analyzeLoopSCEV(llvm::Loop *l, llvm::ScalarEvolution & scev);
        void removeDuplicatesExperimental();
    };

  llvm::Pass* createGVARPAnalysisPass();
}

#endif
