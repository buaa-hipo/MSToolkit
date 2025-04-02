#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Decl.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include <cstdlib>
#include <sstream>

using namespace clang;
using namespace clang::tooling;
using namespace clang::ast_matchers;

static bool inst_kernel = true;
static bool inst_func = true;
static bool inst_malloc = true;
static bool inst_memcpy = true;
static bool inst_barrier = true;

void printSourceLocation(const SourceLocation &Loc, const SourceManager &SM) {
    if (Loc.isInvalid()) {
        llvm::outs() << "Invalid location\n";
        return;
    }

    PresumedLoc PLoc = SM.getPresumedLoc(Loc);
    if (!PLoc.isValid()) {
        llvm::outs() << "Unable to get presumed location\n";
        return;
    }

    llvm::outs() << "File: " << PLoc.getFilename() << "\n";
    llvm::outs() << "Line: " << PLoc.getLine() << "\n";
    llvm::outs() << "Column: " << PLoc.getColumn() << "\n";
}

class InsertCallbackHandler : public MatchFinder::MatchCallback {
public:
    InsertCallbackHandler(Rewriter &rewriter) : rewriter(rewriter) {}

    void run(const MatchFinder::MatchResult &Result) override {
        FunctionDecl *func = nullptr;
        CallExpr *Call = nullptr;
        do {
            // if ((func = const_cast<FunctionDecl*>(Result.Nodes.getNodeAs<FunctionDecl>("dev_kernel")))) break;
            if ((func = const_cast<FunctionDecl*>(Result.Nodes.getNodeAs<FunctionDecl>("dev_user_func")))) break;
            if ((Call = const_cast<CallExpr*>(Result.Nodes.getNodeAs<CallExpr>("dev_mem_alloc")))) break;
            if ((Call = const_cast<CallExpr*>(Result.Nodes.getNodeAs<CallExpr>("dev_mem_free")))) break;
            if ((Call = const_cast<CallExpr*>(Result.Nodes.getNodeAs<CallExpr>("dev_memcpy")))) break;
            if ((Call = const_cast<CallExpr*>(Result.Nodes.getNodeAs<CallExpr>("dev_barrier")))) break;
            return;
        } while (0);

        if (Call) {

            const FunctionDecl *func_decl = Call->getDirectCallee();
            std::string func_name = func_decl->getNameAsString();

            SourceLocation start_loc = Call->getBeginLoc();
            SourceLocation end_loc = Call->getEndLoc();
            SourceManager &SM = *Result.SourceManager;
            // printSourceLocation(start_loc, SM);

            // remove the old function call.
            rewriter.RemoveText({start_loc, start_loc.getLocWithOffset(func_name.size() - 1)});

            // add the new function name.
            rewriter.InsertText(start_loc, {"instrumented_" + func_name}, false, true);
        } else if (func) {
            if (!func->hasBody()) return;
            const SectionAttr *Attr = func->getAttr<SectionAttr>();
            SourceLocation start_loc = func->getBeginLoc();
            SourceLocation end_loc = func->getEndLoc();
            SourceManager &SM = *Result.SourceManager;

            const std::string func_name = func->getNameAsString();
            llvm::outs() << "HAHAHA " << func_name << "\n";
            
            if (inst_kernel && Attr != nullptr) {
                
                llvm::outs() << Attr->getName() << "\n";
                if (Attr->getName() == ".global") {
                    Stmt *KernelBody = func->getBody();
                    const std::string kernel_name = func->getNameAsString();
                    if (kernel_name == "double_buffer_init" || kernel_name == "double_buffer_fin") return;  // exclude control kernels

                    if (func->param_empty()) {
                        llvm::outs() << "Kernel with no parameter!\n";
                    }
                    if (func->param_size() > 0) {
                        SourceLocation param_start = func->getParamDecl(0)->getBeginLoc();
                        rewriter.InsertText(param_start, "unsigned int host_pid, unsigned long long kernel_cid, unsigned int kernel_index, ", true, true);
                    } else {
                        LangOptions LangOpts;
                        llvm::outs() << "Kernel with no parameter!" << kernel_name << "\n";
                        SourceLocation LParenLoc = func->getLocation().getLocWithOffset(func->getNameInfo().getAsString().length());
                        SourceLocation RParenLoc = Lexer::findLocationAfterToken(LParenLoc, tok::r_paren, SM, LangOpts, true).getLocWithOffset(-1);
                        std::string newParams = "(unsigned int host_pid, unsigned long long kernel_cid, unsigned int kernel_index)";
                        rewriter.ReplaceText(SourceRange(LParenLoc, RParenLoc), newParams);
                    }
                    
                    rewriter.InsertText(KernelBody->getBeginLoc().getLocWithOffset(1), "\ninstrumented_kernel(\"" + kernel_name + "\", host_pid, kernel_cid, kernel_index, ACCL_API_ENTER);", true, true);

                    rewriter.InsertTextAfterToken(KernelBody->getEndLoc().getLocWithOffset(-1), "\ninstrumented_kernel(\"" + kernel_name + "\", host_pid, kernel_cid, kernel_index, ACCL_API_EXIT);");
                } else {
                    Stmt *UserFuncBody = func->getBody();
                    const std::string user_func_name = func->getNameAsString();
                    llvm::outs() << user_func_name << "\n";
                }
            } else if (inst_func) {
                llvm::outs() << "Instrumenting user func!" << "\n";
                Stmt *FuncBody = func->getBody();
                
                rewriter.InsertText(FuncBody->getBeginLoc().getLocWithOffset(1), "\nunsigned long local_cid = correlation_id[get_core_id()];\ninstrumented_func(" + func_name + ", \"" + func_name + "\"" + ", local_cid, ACCL_API_ENTER);", true, true);
                rewriter.InsertTextAfterToken(FuncBody->getEndLoc().getLocWithOffset(-1), "\ninstrumented_func(" + func_name + ", \"" + func_name + "\"" + ", local_cid, ACCL_API_EXIT);");

                llvm::outs() << func_name << " Instrumented!" << "\n";
            }
            printSourceLocation(start_loc, SM);
        }
    }

private:
    Rewriter &rewriter;
};

class MyFrontendAction : public ASTFrontendAction {
public:
    void EndSourceFileAction() override {
        // SourceManager &SM = TheRewriter.getSourceMgr();
        // llvm::errs() << "** EndSourceFileAction for: "
        //              << SM.getFileEntryForID(SM.getMainFileID())->getName() << "\n";
        // TheRewriter.getEditBuffer(SM.getMainFileID()).write(llvm::outs());
		SourceManager &SM = TheRewriter.getSourceMgr();
        // FileID MainFileID = SM.getMainFileID();
        // SourceLocation StartLoc = SM.getLocForStartOfFile(MainFileID);
        // TheRewriter.InsertText(StartLoc, "#include \"instrument/instrumented_func_dev.h\"\n", true, true);

		std::string Filename = std::string(SM.getFileEntryForID(SM.getMainFileID())->getName());
		std::error_code EC;
		llvm::raw_fd_ostream outFile(Filename + ".out", EC, llvm::sys::fs::OF_None);
		TheRewriter.getEditBuffer(SM.getMainFileID()).write(outFile);
    }

    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                   StringRef file) override {
        TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());

        MatchFinder *Finder = new MatchFinder();
        InsertCallbackHandler *Handler = new InsertCallbackHandler(TheRewriter);

        Finder->addMatcher(functionDecl(isDefinition(), hasAttr(attr::Section)).bind("dev_kernel"), Handler);

        Finder->addMatcher(functionDecl(isDefinition()).bind("dev_user_func"), Handler);

        if (inst_malloc) {
            Finder->addMatcher(callExpr(callee(functionDecl(matchesName("vector_malloc*")))).bind("dev_mem_alloc"), Handler);
            Finder->addMatcher(callExpr(callee(functionDecl(matchesName("vector_free")))).bind("dev_mem_free"), Handler);
            Finder->addMatcher(callExpr(callee(functionDecl(matchesName("scalar_malloc")))).bind("dev_mem_alloc"), Handler);
            Finder->addMatcher(callExpr(callee(functionDecl(matchesName("scalar_free")))).bind("dev_mem_free"), Handler);
            Finder->addMatcher(callExpr(callee(functionDecl(matchesName("hbm_malloc")))).bind("dev_mem_alloc"), Handler);
            Finder->addMatcher(callExpr(callee(functionDecl(matchesName("hbm_free")))).bind("dev_mem_free"), Handler);
        }
        
        if (inst_memcpy) {
            Finder->addMatcher(callExpr(callee(functionDecl(matchesName("dma_p2p")))).bind("dev_memcpy"), Handler);
            Finder->addMatcher(callExpr(callee(functionDecl(matchesName("dma_broadcast")))).bind("dev_memcpy"), Handler);
            Finder->addMatcher(callExpr(callee(functionDecl(matchesName("dma_segment")))).bind("dev_memcpy"), Handler);
            Finder->addMatcher(callExpr(callee(functionDecl(matchesName("dma_sg")))).bind("dev_memcpy"), Handler);
            Finder->addMatcher(callExpr(callee(functionDecl(matchesName("dma_wait*")))).bind("dev_memcpy"), Handler);
            Finder->addMatcher(callExpr(callee(functionDecl(matchesName("vector_load*")))).bind("dev_memcpy"), Handler);
            Finder->addMatcher(callExpr(callee(functionDecl(matchesName("vector_store*")))).bind("dev_memcpy"), Handler);
            Finder->addMatcher(callExpr(callee(functionDecl(matchesName("scalar_load*")))).bind("dev_memcpy"), Handler);
            Finder->addMatcher(callExpr(callee(functionDecl(matchesName("scalar_store*")))).bind("dev_memcpy"), Handler);
        }

        if (inst_barrier) {
            Finder->addMatcher(callExpr(callee(functionDecl(matchesName("group_barrier")))).bind("dev_barrier"), Handler);
            Finder->addMatcher(callExpr(callee(functionDecl(matchesName("core_barrier")))).bind("dev_barrier"), Handler);
            Finder->addMatcher(callExpr(callee(functionDecl(matchesName("core_barrier_wait")))).bind("dev_barrier"), Handler);
            Finder->addMatcher(callExpr(callee(functionDecl(matchesName("rwlock_try_rdlock")))).bind("dev_barrier"), Handler);
            Finder->addMatcher(callExpr(callee(functionDecl(matchesName("rwlock_try_wrlock")))).bind("dev_barrier"), Handler);
            Finder->addMatcher(callExpr(callee(functionDecl(matchesName("rwlock_rdlock")))).bind("dev_barrier"), Handler);
            Finder->addMatcher(callExpr(callee(functionDecl(matchesName("rwlock_wrlock")))).bind("dev_barrier"), Handler);
            Finder->addMatcher(callExpr(callee(functionDecl(matchesName("rwlock_unlock")))).bind("dev_barrier"), Handler);
        }
        

        return Finder->newASTConsumer();
    }

private:
    Rewriter TheRewriter;
};

static llvm::cl::OptionCategory MyToolCategory("my-tool options");

std::vector<std::string> split(const std::string &str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

// static llvm::cl::list<std::string> IncludeFunctions("+",
//                                                      llvm::cl::desc("Functions to instrument"),
//                                                      llvm::cl::ZeroOrMore,
//                                                      llvm::cl::cat(MyToolCategory));

static llvm::cl::opt<bool> noFunc("no-func",
        llvm::cl::desc("Do not instrument user func"),
        llvm::cl::cat(MyToolCategory));

static llvm::cl::opt<bool> noMalloc("no-malloc",
        llvm::cl::desc("Do not instrument malloc"),
        llvm::cl::cat(MyToolCategory));

static llvm::cl::opt<bool> noMemcpy("no-memcpy",
        llvm::cl::desc("Do not instrument memcpy"),
        llvm::cl::cat(MyToolCategory));

static llvm::cl::opt<bool> noBarrier("no-barrier",
        llvm::cl::desc("Do not instrument barrier"),
        llvm::cl::cat(MyToolCategory));
        

int main(int argc, const char **argv) {
    auto ExpectedParser = CommonOptionsParser::create(argc, argv, MyToolCategory);
    if (!ExpectedParser) {
        llvm::errs() << ExpectedParser.takeError();
        return 1;
    }
    CommonOptionsParser &OptionsParser = *ExpectedParser;

    std::vector<std::string> source_paths = OptionsParser.getSourcePathList();
    for (size_t i = 0; i < source_paths.size(); ++i) {
        size_t suffix_pos = source_paths[i].find_last_of('.');
        std::string expanded = source_paths[i].substr(0, suffix_pos) + "_expand" + source_paths[i].substr(suffix_pos);
        std::string path_str = getenv("JSI_DEV_INCLUDE_PATH");
        std::vector<std::string> include_paths = split(path_str, ':');
        std::string command = "gcc -E " + source_paths[i] + " -o " + expanded;
        for (const auto& path : include_paths) {
            command += " -I" + path;
        }
        int res = system(command.c_str());
        if (res) return res;
        source_paths[i] = expanded;
    }

    if (noMalloc) {
        llvm::outs() << "Do not instrument malloc\n";
        inst_malloc = false;
    }
    if (noMemcpy) {
        llvm::outs() << "Do not instrument memcpy\n";
        inst_memcpy = false;
    }
    if (noFunc) {
        llvm::outs() << "Do not instrument user func\n";
        inst_func = false;
    }
    if (noBarrier) {
        llvm::outs() << "Do not instrument dev barriers\n";
        inst_barrier = false;
    }
    // if (argc != 2) {
    //     for (int i = 2; i < argc; ++i) {
    //         if (std::string(argv[i]) == "+malloc") inst_malloc = true;
    //         else if (std::string(argv[i]) == "-malloc") inst_malloc = false;
    //         else if (std::string(argv[i]) == "+memcpy") inst_memcpy = true;
    //         else if (std::string(argv[i]) == "-memcpy") inst_memcpy = false;
    //         else if (std::string(argv[i]) == "+kernel") inst_kernel = true;
    //         else if (std::string(argv[i]) == "-kernel") inst_kernel = false;
    //         else if (std::string(argv[i]) == "+func") inst_func = true;
    //         else if (std::string(argv[i]) == "-func") inst_func = false;
    //     }
    // }

    // ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());
    ClangTool Tool(OptionsParser.getCompilations(), source_paths);
    return Tool.run(newFrontendActionFactory<MyFrontendAction>().get());
}
