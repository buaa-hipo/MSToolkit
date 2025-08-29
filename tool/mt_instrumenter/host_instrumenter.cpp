#include "clang/AST/Decl.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include <vector>
#include <string>
#include <cstdlib>
#include <sstream>

using namespace clang;
using namespace clang::tooling;
using namespace clang::ast_matchers;

class InsertCallbackHandler : public MatchFinder::MatchCallback {
public:
    InsertCallbackHandler(Rewriter &rewriter) : rewriter(rewriter) {}

    void run(const MatchFinder::MatchResult &Result) override {
        CallExpr *Call = nullptr;
        do {
            if ((Call = const_cast<CallExpr*>(Result.Nodes.getNodeAs<CallExpr>("host_mem_alloc")))) break;
            if ((Call = const_cast<CallExpr*>(Result.Nodes.getNodeAs<CallExpr>("host_mem_free")))) break;
            if ((Call = const_cast<CallExpr*>(Result.Nodes.getNodeAs<CallExpr>("host_group")))) break;
            if ((Call = const_cast<CallExpr*>(Result.Nodes.getNodeAs<CallExpr>("host_driver")))) break;
            return;
        } while (0);
		SourceLocation start_loc = Call->getBeginLoc();
        SourceLocation end_loc = Call->getEndLoc();

        SourceManager &SM = *Result.SourceManager;

        const FunctionDecl *func_decl = Call->getDirectCallee();
        std::string func_name = func_decl->getNameAsString();

        rewriter.RemoveText({start_loc, start_loc.getLocWithOffset(func_name.size() - 1)});

        rewriter.InsertText(start_loc, {"instrumented_" + func_name}, false, true);
    }

private:
    Rewriter &rewriter;
};

class MyFrontendAction : public ASTFrontendAction {
public:
    void EndSourceFileAction() override {
		SourceManager &SM = TheRewriter.getSourceMgr();

        FileID MainFileID = SM.getMainFileID();
        
		// std::string Filename = std::string(SM.getFileEntryForID(SM.getMainFileID())->getName());
        auto file_entry_ref = SM.getFileEntryForID(MainFileID);
        if (!file_entry_ref) {
            llvm::errs() << "SM.getFileEntryForID(MainFileID) failed!\n";
            exit(1);
        }
        std::string Filename = file_entry_ref->tryGetRealPathName().str();

        // size_t last_slash_pos = Filename.find_last_of("/\\");
        // std::string directory = (last_slash_pos == std::string::npos) ? "" : Filename.substr(0, last_slash_pos + 1);
        // std::string base_name = (last_slash_pos == std::string::npos) ? Filename : Filename.substr(last_slash_pos + 1);
        // std::string instrumented_dir = directory + "source_instrumented";
        // std::string output_file = instrumented_dir + "/" + base_name;
        // std::error_code mkdirEC = llvm::sys::fs::create_directories(instrumented_dir, true);
        // if (mkdirEC) {
        //     llvm::errs() << "Failed to create directory: " << instrumented_dir << "\n";
        //     return;
        // }
		std::error_code EC;
		llvm::raw_fd_ostream outFile(Filename, EC, llvm::sys::fs::OF_None);
		TheRewriter.getEditBuffer(MainFileID).write(outFile);
    }

    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                   StringRef file) override {
        TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());

        MatchFinder *Finder = new MatchFinder();
        InsertCallbackHandler *Handler = new InsertCallbackHandler(TheRewriter);

        // 查找所有 hthread 函数的调用
        Finder->addMatcher(callExpr(callee(functionDecl(matchesName("hthread_group*")))).bind("host_group"), Handler);
        Finder->addMatcher(callExpr(callee(functionDecl(matchesName("hthread_malloc")))).bind("host_mem_alloc"), Handler);
        Finder->addMatcher(callExpr(callee(functionDecl(matchesName("hthread_free")))).bind("host_mem_free"), Handler);
        Finder->addMatcher(callExpr(callee(functionDecl(matchesName("hthread_dat*")))).bind("host_driver"), Handler);
        Finder->addMatcher(callExpr(callee(functionDecl(matchesName("hthread_dev*")))).bind("host_driver"), Handler);
        Finder->addMatcher(callExpr(callee(functionDecl(matchesName("hthread_barrier*")))).bind("host_driver"), Handler);
        Finder->addMatcher(callExpr(callee(functionDecl(matchesName("hthread_rwlock*")))).bind("host_driver"), Handler);
        Finder->addMatcher(callExpr(callee(functionDecl(matchesName("hthread_intr_send")))).bind("host_driver"), Handler);
        Finder->addMatcher(callExpr(callee(functionDecl(matchesName("hthread_handler_register")))).bind("host_driver"), Handler);

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

int main(int argc, const char **argv) {
    auto ExpectedParser = CommonOptionsParser::create(argc, argv, MyToolCategory);
    if (!ExpectedParser) {
        llvm::errs() << ExpectedParser.takeError();
        return 1;
    }
    CommonOptionsParser &OptionsParser = *ExpectedParser;

    std::error_code err = llvm::sys::fs::create_directories("source_instrumented", true);
    if (err) {
        llvm::errs() << "Failed to create directory: source_instrumented\n";
            return 1;
    }

    std::vector<std::string> source_paths = OptionsParser.getSourcePathList();
    for (size_t i = 0; i < source_paths.size(); ++i) {
        size_t suffix_pos = source_paths[i].find_last_of('.');
        std::string base_name = llvm::sys::path::filename(source_paths[i]).str();
        std::string expanded = "source_instrumented/" + base_name.substr(0, suffix_pos) + base_name.substr(suffix_pos);
        std::string path_str = getenv("JSI_HOST_INCLUDE_PATH");
        std::vector<std::string> include_paths = split(path_str, ':');
        std::string command = "clang++ -E -DMATRIX -D_GNU_SOURCE " + source_paths[i] + " -o " + expanded;
        for (const auto& path : include_paths) {
            command += " -I" + path;
        }
        llvm::outs() << command + '\n';
        int res = system(command.c_str());
        if (res) return res;
        source_paths[i] = expanded;
    }

    // ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());
    ClangTool Tool(OptionsParser.getCompilations(), source_paths);

    std::vector<std::string> ExtraArgs;
    ExtraArgs.push_back("-ferror-limit=0");
    ExtraArgs.push_back("-w");
    Tool.appendArgumentsAdjuster(getInsertArgumentAdjuster(ExtraArgs, ArgumentInsertPosition::BEGIN));


    return Tool.run(newFrontendActionFactory<MyFrontendAction>().get());
}
