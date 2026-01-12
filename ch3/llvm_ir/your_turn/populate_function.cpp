#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"    // For ConstantInt.
#include "llvm/IR/DerivedTypes.h" // For PointerType, FunctionType.
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/Debug.h" // For errs().

#include<memory>

#include <memory> // For unique_ptr

using namespace llvm;

// The goal of this function is to build a Module that
// represents the lowering of the following foo, a C function:
// extern int baz();
// extern void bar(int);
//  {
//   int var = a + b;
//   if (var == 0xFF) {
//     bar(var);
//     var = baz();
//   }
//   bar(var);
// }
//
// The IR for this snippet (at O0) is:
// define void @foo(i32 %arg, i32 %arg1) {
// bb:
//   %i = alloca i32
//   %i2 = alloca i32
//   %i3 = alloca i32
//   store i32 %arg, ptr %i
//   store i32 %arg1, ptr %i2
//   %i4 = load i32, ptr %i
//   %i5 = load i32, ptr %i2
//   %i6 = add i32 %i4, %i5
//   store i32 %i6, ptr %i3
//   %i7 = load i32, ptr %i3
//   %i8 = icmp eq i32 %i7, 255
//   br i1 %i8, label %bb9, label %bb12
//
// bb9:
//   %i10 = load i32, ptr %i3
//   call void @bar(i32 %i10)
//   %i11 = call i32 @baz()
//   store i32 %i11, ptr %i3
//   br label %bb12
//
// bb12:
//   %i13 = load i32, ptr %i3
//   call void @bar(i32 %i13)
//   ret void
// }
//
// declare void @bar(i32)
// declare i32 @baz(...)
std::unique_ptr<Module> myBuildModule(LLVMContext &Ctxt) { 
   // SMDiagnostic Err;
    std::unique_ptr<Module> MyModule(new Module("MyModule", Ctxt));
    /* 错误写法 FunctionCallee baz = MyModule->getOrInsertFunction("baz",Type::getInt32Ty(Ctxt),Type::getVoidTy(Ctxt));
    Function arguments must have first-class types!
    void %0
    Incorrect number of arguments passed to called function!
    %11 = call i32 @baz()

    */
    FunctionType *BazTy =
      FunctionType::get(/*RetTy=*/Type::getInt32Ty(Ctxt), /*isVarArg=*/false);
     FunctionCallee baz =MyModule->getOrInsertFunction("baz", BazTy);
    FunctionCallee bar = MyModule->getOrInsertFunction("bar",Type::getVoidTy(Ctxt),Type::getInt32Ty(Ctxt));
     // 1. 定义参数类型
    Type *ArgTypes[] = {Type::getInt32Ty(Ctxt), Type::getInt32Ty(Ctxt)};

    // 2. 创建函数签名: void foo(int, int)
    FunctionType *FT = FunctionType::get(Type::getVoidTy(Ctxt), ArgTypes, false);

    // Argument *V = new Argument(Type::getInt32Ty(Ctx));
    Function *foo = Function::Create(FT, Function::ExternalLinkage, "foo", MyModule.get());
    //先创建 Block -> 创建 Builder -> 写逻辑 -> 最后写跳转
    BasicBlock *BB1 = BasicBlock::Create(Ctxt, "bb1", foo);
    IRBuilder<> Builder1(BB1);
    BasicBlock* BB2 = BasicBlock::Create(Ctxt, "bb2", foo); // foo 作为父对象必须传
    IRBuilder<> Builder2(BB2);
   
    BasicBlock* BB3 = BasicBlock::Create(Ctxt, "bb3", foo);
    IRBuilder<> Builder3(BB3);
    
    //BB1
    AllocaInst *var_i = Builder1.CreateAlloca(Builder1.getInt32Ty());
    AllocaInst *var_i2 = Builder1.CreateAlloca(Builder1.getInt32Ty());
    AllocaInst *var_i3 = Builder1.CreateAlloca(Builder1.getInt32Ty());
    Value *StoreInst0 = Builder1.CreateStore(foo->getArg(0), var_i);
    Value *StoreInst1 = Builder1.CreateStore(foo->getArg(1), var_i2);
    Value* load4 = Builder1.CreateLoad(Builder1.getInt32Ty() ,var_i);
    Value* load5 = Builder1.CreateLoad(Builder1.getInt32Ty() ,var_i2);
    Value* add = Builder1.CreateAdd(load4,load5);
    Value *StoreInst3 = Builder1.CreateStore(add, var_i3);
    Value* load6 = Builder1.CreateLoad(Builder1.getInt32Ty() ,var_i3);
    Value* cmp = Builder1.CreateCmp(ICmpInst::ICMP_EQ,load6,ConstantInt::get(IntegerType::get(Ctxt, 32),0xFF));
    //Builder1.CreateBr(BB2); // BB1跳转到BB2
    //Builder1.CreateBr(BB3);//BB1跳转到BB3
    // 参数顺序：(条件, 为真时的目标, 为假时的目标)
    Builder1.CreateCondBr(cmp, BB2, BB3);
  
    //BB2
    Value* load7 = Builder2.CreateLoad(Builder2.getInt32Ty() ,var_i3);
    
    Builder2.CreateCall(bar, load7);
    Value* var_i11 = Builder2.CreateCall(baz);
    Value *StoreInst4 = Builder2.CreateStore(var_i11, var_i3);
    Builder2.CreateBr(BB3);//BB跳转到BB3
    //BB3
    Value* var_i12 = Builder3.CreateLoad(Builder3.getInt32Ty() ,var_i3);
    Builder3.CreateCall(bar, var_i12);
    Builder3.CreateRetVoid();
    return MyModule ;
 }
