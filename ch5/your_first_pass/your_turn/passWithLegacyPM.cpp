#include "llvm/IR/Function.h"
#include "llvm/Pass.h"          // For FunctionPass & INITIALIZE_PASS.
#include "llvm/Support/Debug.h" // For errs().

using namespace llvm;

extern bool solutionConstantPropagation(llvm::Function &);

// The implementation of this function is generated at the end of this file. See
// INITIALIZE_PASS.
namespace llvm {
void initializeYourTurnConstantPropagationPass(PassRegistry &);
};

namespace {
class YourTurnConstantPropagation: public llvm::FunctionPass {
public:
  static char ID;
  YourTurnConstantPropagation():llvm::FunctionPass(ID) {
    initializeYourTurnConstantPropagationPass(*PassRegistry::getPassRegistry());
  }

  // TODO: Fill in the blanks.
  bool runOnFunction(llvm::Function& F) override {
    llvm::outs() << " My Turn Legacy called on"<< F.getName() << "\n" ;
    bool change = solutionConstantPropagation(F);
    return change;

  }
};
char YourTurnConstantPropagation::ID = 0;
} // End anonymous namespace.

// TODO: Remove and add proper implementation
//void llvm::initializeYourTurnConstantPropagationPass(PassRegistry &) {}

INITIALIZE_PASS(/*passImplementationName=*/YourTurnConstantPropagation,
                /*commandLineArgName=*/"My legacy-solution",
                /*name=*/"My Legacy Solution", /*isCFGOnly=*/false,
                /*isAnalysis=*/false);

Pass *createYourTurnPassForLegacyPM() {
  return new YourTurnConstantPropagation(); // TODO: Fill in the blanks.
}
