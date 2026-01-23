#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h" // For the new PassManager.

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Transforms/IPO/AlwaysInliner.h"       // For inliner pass.
#include "llvm/Transforms/InstCombine/InstCombine.h" // For instcombine pass.
#include "llvm/Transforms/Utils/Mem2Reg.h"           // For mem2reg pass.

#include "llvm/Passes/StandardInstrumentations.h"
using namespace llvm;

void runYourTurnPassPipelineForNewPM(Module &MyModule) {
    LLVMContext &context = MyModule.getContext();

    // Analysis
    FunctionAnalysisManager FAM;
    ModuleAnalysisManager MAM;

    // MAM.registerPass([&] { return FunctionAnalysisManagerModuleProxy(FAM); });
    // FAM.registerPass([&] { return ModuleAnalysisManagerFunctionProxy(MAM); });

    PassBuilder pb; //注册基础analysis
    pb.registerFunctionAnalyses(FAM);
    pb.registerModuleAnalyses(MAM);
    //少了下面两句
    MAM.registerPass([&] { return FunctionAnalysisManagerModuleProxy(FAM); });
    FAM.registerPass([&] { return ModuleAnalysisManagerFunctionProxy(MAM); });

    FunctionPassManager FPM;
    ModulePassManager MPM;

    FPM.addPass(PromotePass());// mem2reg
    FPM.addPass(InstCombinePass()); // instcombine;

    MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
    MPM.addPass(AlwaysInlinerPass());

    MPM.run(MyModule,MAM);
 

}
