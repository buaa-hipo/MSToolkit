#include <gvarp/llvm-pass/GVARPAnalysis.hpp>

#include <llvm/Analysis/CallGraph.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Analysis/ScalarEvolutionExpressions.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/ModuleSlotTracker.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/CommandLine.h>

#include <algorithm>
#include <iostream>
#include <numeric>
#include <vector>
#include <string>
#include <fstream>
#include <cstdio>
#include <regex>
#include <cxxabi.h>

static llvm::cl::opt<std::string> LogFileName("gvarplog-name",
                                        llvm::cl::desc("Specify filename for output log"),
                                        llvm::cl::init("unknown"),
                                        llvm::cl::value_desc("filename"));

static llvm::cl::opt<std::string> OutFileName("gvarp-out-name",
                                        llvm::cl::desc("Specify filename for output log"),
                                        llvm::cl::init("unknown"),
                                        llvm::cl::value_desc("filename"));

namespace gvarp {
    void GVARPAnalysis::getAnalysisUsage(llvm::AnalysisUsage &AU) const
    {
      ModulePass::getAnalysisUsage(AU);
      // We require loop information
      AU.addRequired<llvm::LoopInfoWrapperPass>();

      AU.addRequiredTransitive<llvm::ScalarEvolutionWrapperPass>();
      // Pass does not modify the input information
      AU.addRequired<llvm::CallGraphWrapperPass>();
      AU.setPreservesAll();
    }

    llvm::StringRef GVARPAnalysis::getPassName() const
    {
      return "GVARPAnalysis";
    }

    bool GVARPAnalysis::runOnModule(llvm::Module &m)
    {
        llvm::errs() << "GVARPAnalysis::runOnModule() target triple: " << m.getTargetTriple() << "\n";
        this->m = &m;
        cgraph = &getAnalysis<llvm::CallGraphWrapperPass>().getCallGraph();
        for(llvm::Function & f : m) {
            runOnFunction(f);
        }
        return false;
    }

    /* For each function, the analysis need to determine whether it is a fixed-workload 
     * probe candidate and discover the critical parameters to its workload. 
     * To accomplish this, we require three analysis stages: 
     * 1) workload-critical variable detection via SCEV pass (sink nodes)
     * 2) Input paramter detection (source parameters)
     * 3) taint analysis: 
     *     - For each input parameter (source node), whether it contribute to the 
     *       value of workload-critical variable (sink nodes).
     *     - All input parameters that can reach the sink nodes are considered as 
     *       critical parameters.
     * Note that when the function contains any loops that fails SCEV analysis, 
     * we will not consider it as a candidate.
     */
    bool GVARPAnalysis::runOnFunction(llvm::Function & f, int override_counter)
    {
        // ignore declarations and non-kernel functions
        if(!is_analyzable(*m, f))
            return false;

        llvm::errs() << "Analyzing function: " << f.getName() << '\n';
        llvm::CallGraphNode * f_node = (*cgraph)[&f];
        /* Step 1: discover all workload-critical variables (a.k.a., sinks) */
        auto ret = analyzeFunction(f, f_node, override_counter);
        bool is_probe_cand   = std::get<0>(ret);
        ValueSet& sinks = *(std::get<1>(ret));
        llvm::errs() << "Function: " << f.getName() << " is GVARP probe candidate: " << is_probe_cand << '\n';
        // early stop when it is not an analyzable probe candidate
        if (!is_probe_cand) return false;
        llvm::errs() << "Sink node set: \n";
        for(auto v : sinks) {
          llvm::errs() << *v << "\n";
        }
        /* Step 2: discover all exciplit/implicit input parameters (a.k.a., sources) */
        ParameterFinder finder(f);
        ValueSet& sources = finder.find_args();
        /* Step 3: Static taint analysis from source to sink. */
        // TaintAnalyzer analyzer(f, f_node);
        // ParameterList& critical_params = func_database[&f].critical_params;
        // for (auto & src : sources) {
        //   bool reachable = analyzer.run(src, sinks);
        //   if (reachable) {
        //       critical_params.push_back(src);
        //   }
        // }
        // llvm::errs() << "Performance-critical parameters:\n" << critical_params << "\n";
        return true;
    }

    void mergeValueSet(ValueSet& s_dst, ValueSet& s_src) {
        for (auto it : s_src) {
          s_dst.emplace(it);
        }
    }

    bool GVARPAnalysis::is_analyzable(llvm::Module & m, llvm::Function & f)
    {
        llvm::Function * in_module = m.getFunction(f.getName());
        if(f.isDeclaration() || !in_module) {
            unknown << f.getName().str() << '\n';
            llvm::errs() << "Not analyzable: " << f.getName() << " declaration? " << f.isDeclaration() << " in module? " << static_cast<bool>(in_module) << '\n';
            return false;
        }
#ifdef ROCM
        /* For AMD GPU/HyGon DCU, we use the calling convension to identify kernel functions */
        if(f.getCallingConv()!=llvm::CallingConv::AMDGPU_KERNEL) {
            llvm::errs() << "Not a GPU kernel, ignore GVARP static analysis: " << f.getName() << "\n";
            return false;
        }
#endif
        // TODO: CUDA kernel can be detected by checking the nvvm.annotations as described in document.
        llvm::errs() << "Analyzable: " << f.getName() << '\n';
        return true;
    }

    std::tuple<bool, ValueSet*> GVARPAnalysis::analyzeFunction(llvm::Function & f,
            llvm::CallGraphNode * cg_node, int override_counter)
    {
        bool is_probe_cand = true;
        ValueSet* sinks = new ValueSet();
        // analyze calling functions
        for(auto & callsite : *cg_node)
        {
            llvm::Value * call = callsite.first.getValue();
            llvm::CallGraphNode * node = callsite.second;
            llvm::Function * called_f = node->getFunction();
            if(called_f) {
                std::tuple<bool, ValueSet*> called_f_sinks;
                auto it = func_cache.find(called_f);
                if (it==func_cache.end()) {
                    called_f_sinks = analyzeFunction(*called_f, node, override_counter+1);
                } else {
                    called_f_sinks = it->second;
                }
                is_probe_cand &= std::get<0>(called_f_sinks);
                // early stop when called functions detected as unpredictable
                if (!is_probe_cand) {
                  func_cache[&f] = std::make_tuple(false, sinks);
                  return std::make_tuple(false, sinks);
                }
                mergeValueSet(*sinks, *std::get<1>(called_f_sinks));
            }
        }
        // analyze loops within this function scope
        auto linfo = &getAnalysis<llvm::LoopInfoWrapperPass>(f).getLoopInfo();
        assert(linfo);

        llvm::ScalarEvolution & scev = getAnalysis<llvm::ScalarEvolutionWrapperPass>(f).getSE();
        llvm::LoopInfo & analyzed_linfo = getAnalysis<llvm::LoopInfoWrapperPass>(f).getLoopInfo();
        // Process loops
        for(llvm::Loop * l : analyzed_linfo) {
            auto ret = analyzeLoop(l, scev);
            // only considered as probe candidate when the trip counts of all loops are predictable
            is_probe_cand &= std::get<0>(ret);
            // early stop when detected as unpredictable.
            if(!is_probe_cand) break;
            // merge all critical variables into a single sink node set
            mergeValueSet(*sinks, *std::get<1>(ret));
        }
        func_cache[&f] = std::make_tuple(is_probe_cand, sinks);
        return std::make_tuple(is_probe_cand, sinks);
    }

    std::tuple<bool, ValueSet*> GVARPAnalysis::analyzeLoop(llvm::Loop *l, llvm::ScalarEvolution & scev) {
        ValueSet* sinks = new ValueSet();
        // check all subloops first
        auto & subloops = l->getSubLoops();
        for(llvm::Loop * subl : subloops) {
            auto ret = analyzeLoop(subl, scev);
            if (!std::get<0>(ret)) {
                loop_cache[l] = std::make_tuple(false, sinks);
                return std::make_tuple(false, sinks);
            }
            mergeValueSet(*sinks, *std::get<1>(ret));
        }
        // try to reuse well-formed SCEV analysis to discover constant trip counts.
        if (analyzeLoopSCEV(l, scev)) {
          loop_cache[l] = std::make_tuple(true, sinks);
          return std::make_tuple(true, sinks);
        }
        // if SCEV is failed, fall back to found loop count associated variables as sink nodes.
        llvm::SmallVector<llvm::BasicBlock*, 10> exit_blocks;
        l->getExitingBlocks(exit_blocks);
        for(llvm::BasicBlock * bb : exit_blocks) {
            llvm::Instruction * inst = bb->getTerminator();
            llvm::errs() << "Terminating instruction of exiting block:\n" << *inst << '\n';
            const llvm::BranchInst * br = llvm::dyn_cast<llvm::BranchInst>(inst);
            if(br && br->isConditional()) {
                llvm::Instruction * inst = llvm::dyn_cast<llvm::Instruction>(br->getCondition());
                // For branches with actual conditions
                if(inst) {
                  // llvm::errs() << "condition instruction:\n" << *inst << '\n';
                  unsigned n = inst->getNumOperands();
                  for(unsigned i=0; i<n; ++i) {
                    // llvm::errs() << "Condition operand " << i << ":" << *inst->getOperand(i) << "\n";
                    sinks->emplace(inst->getOperand(i));
                  }
                }
            } else if(const llvm::SwitchInst * _switch = llvm::dyn_cast<llvm::SwitchInst>(inst)) {
                llvm::Instruction * inst =
                    llvm::dyn_cast<llvm::Instruction>(_switch->getCondition());
                if(inst) {
                  // llvm::errs() << "condition instruction:\n" << *inst << '\n';
                  unsigned n = inst->getNumOperands();
                  for(unsigned i=0; i<n; ++i) {
                    // llvm::errs() << "Condition operand " << i << ":" << *inst->getOperand(i) << "\n";
                    sinks->emplace(inst->getOperand(i));
                  }
                }
            } else {
                // llvm::errs() << "Unknown branch: " << *inst << '\n';
            }
        }
        // if none sink nodes are found, the analysis must be failed.
        bool is_probe_cand = !sinks->empty();
        // cache & return
        loop_cache[l] = std::make_tuple(is_probe_cand, sinks);
        return std::make_tuple(is_probe_cand, sinks);
    }

    bool GVARPAnalysis::analyzeLoopSCEV(llvm::Loop *l, llvm::ScalarEvolution & scev)
    {
        if(scev.hasLoopInvariantBackedgeTakenCount(l)) {
          const llvm::SCEV * backedge_count = scev.getBackedgeTakenCount(l);
          // Unknown count? SCEV failed, function is instrumented
          if(llvm::isa<llvm::SCEVCouldNotCompute>(backedge_count)) {
            llvm::errs() << "SCEV failed: unknown count\n";
          // Non-constant count? SCEV succeeded, function is instrumented
          } else if(llvm::isa<llvm::SCEVConstant>(backedge_count)) {
            llvm::errs() << "Constant loop number: " << *backedge_count << "\n";
            return true;
          // Constant count? SCEV succeeded, function maybe not instrumented
          } else {
            llvm::errs() << "Extracted SCEV: " << *backedge_count << "\n";
          }
        // Not known? SCEV failed, function is instrumented
        } else {
          llvm::errs() << "SCEV failed: loop invariant backage taken count is invalid!\n";
        }
        return false;
    }

    llvm::Pass* createGVARPAnalysisPass()
    {
        return new GVARPAnalysis();
    } 
}

char gvarp::GVARPAnalysis::ID = 0;
