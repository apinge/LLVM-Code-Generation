#include "llvm/IR/LegacyPassManager.h" // For legacy::PassManager.


#include "llvm/Transforms/IPO/AlwaysInliner.h"       // For inliner pass.
#include "llvm/Transforms/InstCombine/InstCombine.h" // For instcombine pass.
#include "llvm/Transforms/Utils.h"                   // For mem2reg pass.
using namespace llvm;

void runYourTurnPassPipelineForLegacyPM(Module &MyModule) {
    legacy::PassManager pm;
    pm.add(createPromoteMemoryToRegisterPass());
    pm.add(createInstructionCombiningPass());
    pm.add(createAlwaysInlinerLegacyPass());
    pm.run(MyModule);
}
