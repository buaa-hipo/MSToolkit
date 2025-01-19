#ifndef __PARAMTER_FINDER_HPP__
#define __PARAMTER_FINDER_HPP__


#include <vector>
#include <utility>
#include <string>
#include <unordered_map>

#include <llvm/ADT/SmallSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Operator.h>

namespace llvm {
    class GlobalVariable;
    class Module;
    class Value;
    class Function;
    class StructType;
}

namespace gvarp {
    typedef std::unordered_set<llvm::Value*> ValueSet;

    class ParameterFinder {
      public:
        ParameterFinder(Function* f);
        // by default, we do not find runtime parameters as it is always collected.
        ValueSet& find_args(bool find_global=true, bool find_runtime=false);
        ValueSet& find_global_vars();
        ValueSet& find_func_params();
        ValueSet& find_rt_params();
      private:
        Function* _f;
        // result of find_args
        ValueSet _params;
        // global variables that may be referred as function inputs
        ValueSet _global_vars;
        // input function parameters
        ValueSet _func_params;
        // runtime paramters, e.g., threadIdx, blockIdx, blockNum, GridNum, ...
        ValueSet _rt_params;
    };
}

#endif