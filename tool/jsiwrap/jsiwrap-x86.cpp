#include <iostream>
#include <string>

#include "BPatch.h"
#include "BPatch_binaryEdit.h"
#include "BPatch_addressSpace.h"
#include "BPatch_function.h"
#include "BPatch_image.h"
#include "BPatch_point.h"
#include "BPatch_process.h"
#include "BPatch_thread.h"
#include "BPatch_type.h"

BPatch bpatch;

BPatch_addressSpace *appThread;
BPatch_image *appImage;

BPatch_function *traceEntryFunc;
BPatch_function *traceExitFunc;
// BPatch_function *traceInitFunc;
// BPatch_function *traceFinalizeFunc;

void showModsFuncs(BPatch_addressSpace *app);

BPatch_function *findFunc(char const *str) {
  BPatch_Vector<BPatch_function *> *funcBuf =
      new BPatch_Vector<BPatch_function *>;
  appImage->findFunction(str, *funcBuf);
  if ((*funcBuf).size() < 1) {
    std::cerr << "couldn't find the function " << str << "\n";
    exit(1);
  }

  return (*funcBuf)[0]; // grab an arbitrary one
}

void initTraceLibrary() {
  const char *tracetool_lib = getenv("TRACETOOL_LIB");
  if (tracetool_lib == nullptr) {
    std::cerr << "Need to set environment variable TRACETOOL_LIB to use "
              << "tracetool\n";
    exit(1);
  }

  if (!appThread->loadLibrary(tracetool_lib)) {
    std::cerr << "failed when attempting to load library " << tracetool_lib
              << std::endl;
    exit(1);
  }

  traceEntryFunc = findFunc("trace_entry_func");
  traceExitFunc = findFunc("trace_exit_func");
  // traceInitFunc = findFunc("trace_init_func");
  // traceFinalizeFunc = findFunc("trace_finalize_func");
}

void instrument_entry(BPatch_function *func, char *funcname) {
  BPatch_Vector<BPatch_snippet *> traceFuncArgs;

  BPatch_Vector<BPatch_point *> *entryPointBuf = func->findPoint(BPatch_entry);
  if ((*entryPointBuf).size() != 1) {
    std::cerr << "couldn't find entry point for func " << funcname << std::endl;
    exit(1);
  }
  BPatch_point *entryPoint = (*entryPointBuf)[0];

  BPatch_funcCallExpr traceEntryCall(*traceEntryFunc, traceFuncArgs);
  appThread->insertSnippet(traceEntryCall, *entryPoint, BPatch_callBefore,
                           BPatch_firstSnippet);
}

void instrument_exit(BPatch_function *func, char *funcname) {
  BPatch_Vector<BPatch_snippet *> traceFuncArgs;

  BPatch_Vector<BPatch_point *> *exitPointBuf = func->findPoint(BPatch_exit);

  // dyninst might not be able to find exit point
  if ((*exitPointBuf).size() == 0) {
    std::cerr << "   couldn't find exit point, so returning\n";
    return;
  }

  for (int i = 0; i < (*exitPointBuf).size(); i++) {
    // std::cerr << "   inserting an exit pt instrumentation\n";
    BPatch_point *curExitPt = (*exitPointBuf)[i];

    BPatch_funcCallExpr traceExitCall(*traceExitFunc, traceFuncArgs);
    appThread->insertSnippet(traceExitCall, *curExitPt, BPatch_callAfter,
                             BPatch_firstSnippet);
  }
}

/*
void instrument_MPI_Init_exit(BPatch_function *func) {
  BPatch_Vector<BPatch_snippet *> traceFuncArgs;

  BPatch_Vector<BPatch_point *> *exitPointBuf = func->findPoint(BPatch_exit);

  // dyninst might not be able to find exit point
  if ((*exitPointBuf).size() == 0) {
    std::cerr << "   couldn't find exit point, so returning\n";
    return;
  }

  for (int i = 0; i < (*exitPointBuf).size(); i++) {
    // std::cerr << "   inserting an exit pt instrumentation\n";
    BPatch_point *curExitPt = (*exitPointBuf)[i];

    BPatch_funcCallExpr traceMPIInitCall(*traceMPIInitFunc, traceFuncArgs);
    appThread->insertSnippet(traceMPIInitCall, *curExitPt, BPatch_callAfter,
                             BPatch_firstSnippet);
  }
}


void instrument_finalize(BPatch_thread *thread, BPatch_exitType type) {
  BPatch_process *app = thread->getProcess();
  BPatch_image *appImage = app->getImage();
  
  BPatch_Vector<BPatch_snippet *> traceFuncArgs;
  BPatch_funcCallExpr traceFinalizeCall(*traceFinalizeFunc, traceFuncArgs);

  void * ret = app->oneTimeCode(traceFinalizeCall);
  fprintf(stderr, "Exit callback called for process...--exitType=%d, ret = %d\n", type, (int)(long)ret);

  app->continueExecution();
}

void instrument_finalize(BPatch_function *func, char *funcname) {
  BPatch_Vector<BPatch_snippet *> traceFuncArgs;

  BPatch_Vector<BPatch_point *> *entryPointBuf = func->findPoint(BPatch_entry);
  if ((*entryPointBuf).size() != 1) {
    std::cerr << "couldn't find entry point for func " << funcname << std::endl;
    exit(1);
  }
  BPatch_point *entryPoint = (*entryPointBuf)[0];

  BPatch_funcCallExpr traceFinalizeCall(*traceFinalizeFunc, traceFuncArgs);
  appThread->insertSnippet(traceFinalizeCall, *entryPoint, BPatch_callBefore,
                           BPatch_firstSnippet);

}
*/
void instrument_funcs_in_module(BPatch_module *mod, char const *funcname) {
  BPatch_Vector<BPatch_function *> *allprocs = mod->getProcedures();

  char name[100];


  for (unsigned i = 0; i < (*allprocs).size(); i++) {
    BPatch_function *func = (*allprocs)[i];
    func->getName(name, 99);
	
    if(strcmp(name, funcname)) continue;

    instrument_entry(func, name);
    instrument_exit(func, name);
    break;
  }
}

// clang-format off
void usage() {
  fprintf(stderr, "Usage: jsiwrap -- program [program args] -f funcname\n");
  fprintf(stderr, "       (need to set environment variable TRACETOOL_LIB to "
                  "\n        path of trace library)\n");
}
// clang-format on

void postForkFunc(BPatch_thread *parent, BPatch_thread *child) {
  std::cerr
      << "###############################################################\n";
  std::cerr << "tool:  a fork occurred, parent pid: "
            << parent->getProcess()->getPid()
            << ", child pid: " << child->getProcess()->getPid() << std::endl;
  std::cerr
      << "###############################################################\n";
  parent->getProcess()->continueExecution();
  child->getProcess()->continueExecution();
  std::cerr << "done with postForkFunc\n";
}

// End the sequence with a nullptr
// should_instrument_module is expecting these to all be in lowercase
char const *excluded_modules[] = {"default_module", "libstdc++",
                                  "libm",           "libc",
                                  "ld-linux",       "libdyninstapi_rt",
                                  "libdl",          "tracelib",
                                  "kernel",         ".so.",
                                  "global_linkage", // AIX modules
                                  nullptr};

void mystrlwr(char *str) {
  for (char *c = str; (*c) != 0; c++) {
    *c = tolower(*c);
  }
}

bool should_instrument_module(char const *mod_input) {
  int i = 0;
  char modname[100];
  strcpy(modname, mod_input);
  mystrlwr(modname);
  while (i < 1000) {
    char const *cur_mod = excluded_modules[i];
    if (cur_mod == nullptr)
      break;
    if (strstr(modname, cur_mod))
      return false;
    i++;
  }
  return true;
}

void handleArguments(int argc, char *argv[], bool *show_usage, char **program, char **thefunc, char const *prog_args[]) {
  *show_usage = false;
  // start at argument 1, that is skip the program name, because we don't
  // care about the program name
  if (argc <= 1) {
    *show_usage = true;
    return;
  }

  int parg_index = 0;
  bool have_prog = false;
  bool have_func = false;
  for (int i = 1; i < argc; i++) {
    char *curArg = argv[i];

    if (curArg[0] == '-' && curArg[1] == 'h') {
      *show_usage = true;
      return;
    } else if (curArg[0] == '-' && curArg[1] == 'f') {
      *thefunc = argv[++i];
      have_func = true;
    } else if(curArg[0] == '-' && curArg[1] == '-') {
      if (have_prog == false) {
        *program = argv[++i];
        have_prog = true;
      }
      prog_args[parg_index] = curArg;
      parg_index++;
    }
  }
  if(!have_func) *show_usage = true;
  prog_args[parg_index] = nullptr;
}

void finishInstrumenting(BPatch_addressSpace *app, const char *newName) {
  BPatch_process *appProc = dynamic_cast<BPatch_process*>(app);
  BPatch_binaryEdit *appBin = dynamic_cast<BPatch_binaryEdit*>(app);

  if (appProc) {
    if (!appProc->continueExecution()) {
      fprintf(stderr, "continueExecution failed\n");
    }
    while (!appProc->isTerminated()) {
      bpatch.waitForStatusChange();
    }
  } else if (appBin) {
      if (!appBin->writeFile(newName)) {
        fprintf(stderr, "writeFile failed\n");
      }
  }
}

int main(int argc, char *argv[]) {
  char *program_path, *thefunc;
  char const *prog_args[50]; // maximum of 50 program arguments
  bool show_usage = false;

  handleArguments(argc, argv, &show_usage, &program_path, &thefunc, prog_args);
  if(show_usage) {
      usage();
      return 0;
  }

  bpatch.registerPostForkCallback(postForkFunc);

  // create
  // appThread = bpatch.processCreate(program_path, prog_args);
  // if (!appThread) { fprintf(stderr, "create proc failed\n"); }

  //  binary edit
  appThread = bpatch.openBinary(program_path, false);
  if (!appThread) { fprintf(stderr, "open binary failed\n"); }

  appImage = appThread->getImage();

  initTraceLibrary();

/*
  BPatch_Vector<BPatch_snippet *> traceFuncArgs;
  BPatch_funcCallExpr traceInitCall(*traceInitFunc, traceFuncArgs);
  appThread->oneTimeCode(traceInitCall);
*/
  const BPatch_Vector<BPatch_module *> *mbuf = appImage->getModules();

  for (unsigned n = 0; n < (*mbuf).size(); n++) {
    BPatch_module *mod = (*mbuf)[n];
    char modname[100];
    mod->getName(modname, 99);
    if (/*strstr(modname, "mpi") || */should_instrument_module(modname)) {
      instrument_funcs_in_module(mod, thefunc);
    }
  }

  // bpatch.registerExitCallback(instrument_finalize);
  // appThread->continueExecution();

  // Finish instrumentation
  std::string progNameNew = program_path + std::string("-rewritten");
  finishInstrumenting(appThread, progNameNew.c_str());

  return 0;
}
