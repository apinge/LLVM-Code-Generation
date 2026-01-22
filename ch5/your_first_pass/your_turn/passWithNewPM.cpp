#include "llvm/IR/Function.h"
#include "llvm/Support/Debug.h" // For errs().

#include "passWithNewPM.h"

using namespace llvm;



extern bool solutionConstantPropagation(llvm::Function &);

 llvm::PreservedAnalyses YourTurnConstantPropagationNewPass::run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM){
    outs() << "Solution New PM on " << F.getName() << "\n";
    bool changes = solutionConstantPropagation(F);
    if(!changes){
        return PreservedAnalyses::all();
    }
    PreservedAnalyses PA;
    PA.preserveSet<CFGAnalyses>();
    return PA;
                              }
/*
之所以const progagation不改变CFG
因为
CFG 只关心
有哪些 block
block 之间有没有边

        bb
       /  \
      v    v
    bb2   bb4
      \    /
       v  v
        bb6

*/
                              
// TODO: Fill in the blanks.
