#include <gvarp/llvm-pass/ParameterFinder.hpp>

namespace gvarp {
    ParameterFinder::ParameterFinder(Function* f) {
        _f = f;
    }

    ValueSet& ParameterFinder::find_args(bool find_global, bool find_runtime) {
        _params.clear();
        ValueSet& fparams = find_func_params();
        _params.merge(fparams);
        if (find_global) {
            ValueSet& gparams = find_global_vars();
            _params.merge(gparams);
        }
        if (find_runtime) {
            ValueSet& rparams = find_rt_params();
            _params.merge(rparams);
        }
        return _params;
    }

    ValueSet& ParameterFinder::find_global_vars() {
        _global_vars.clear();
        for (auto &bb : *f) {
            for (auto &i : bb) {
                int n = i.getNumOperands();
                for(int k=0; k<n; ++k) {
                    Value* v = i.getOperand(k);
                    if (llvm::dyn_cast<GlobalVariable>(v);) {
                        _global_vars.insert(v);
                    }
                }
            }
        }
        return _global_vars;
    }

    ValueSet& ParameterFinder::find_func_params() {
        _func_params.clear();
        // ...
        return _func_params;
    }

    ValueSet& ParameterFinder::find_rt_params(){
        _rt_params.clear();
        for (auto &bb : *f) {
            for (auto &i : bb) {
                // ... 
            }
        }
        return _rt_params;
    }
}