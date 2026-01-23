#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h" // For the new PassManager.

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Transforms/IPO/AlwaysInliner.h"       // For inliner pass.
#include "llvm/Transforms/InstCombine/InstCombine.h" // For instcombine pass.
#include "llvm/Transforms/Utils/Mem2Reg.h"           // For mem2reg pass.

#include "llvm/Passes/StandardInstrumentations.h"
using namespace llvm;

#define PRINT_MY_PIPELINE 1
void runYourTurnPassPipelineForNewPM(Module &MyModule) {
    LLVMContext &context = MyModule.getContext();

    // Analysis
    FunctionAnalysisManager FAM;
    ModuleAnalysisManager MAM;

  
    //这两句非常重要
    // analysis不能跨层级访问 function level的analysis需要调用 module level的信息
    MAM.registerPass([&] { return FunctionAnalysisManagerModuleProxy(FAM); });
    FAM.registerPass([&] { return ModuleAnalysisManagerFunctionProxy(MAM); });

#ifdef PRINT_MY_PIPELINE
    PassInstrumentationCallbacks PIC;
    PrintPassOptions PrintPassOpts;
    PrintPassOpts.Verbose = true;
    PrintPassOpts.SkipAnalyses = false;
    PrintPassOpts.Indent = true;
    StandardInstrumentations SI(context, /*DebugLogging=*/true,
                                /*VerifyEachPass=*/false, PrintPassOpts);
    SI.registerCallbacks(PIC, &MAM);

    // Register the passes used implicitly at the start of the pipeline.
    // And enable logging.
    MAM.registerPass([&] { return PassInstrumentationAnalysis(&PIC); });
    FAM.registerPass([&] { return PassInstrumentationAnalysis(&PIC); });
#endif
    // 注意 pb必须在打印的这些hook的后面？
    // 如果在前面注册就打不出来
    PassBuilder pb; //注册基础analysis
    pb.registerFunctionAnalyses(FAM);
    pb.registerModuleAnalyses(MAM);



    FunctionPassManager FPM;
    ModulePassManager MPM;

    FPM.addPass(PromotePass());// mem2reg
    FPM.addPass(InstCombinePass()); // instcombine;

    MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM))); 
    MPM.addPass(AlwaysInlinerPass()); //注意不同的pass属于不同层级

    MPM.run(MyModule,MAM);
 

}
