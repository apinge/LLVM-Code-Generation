#include "llvm/AsmParser/Parser.h" // For parseAssemblyString.
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h" // For LoadInst.
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Use.h"
#include "llvm/IR/User.h"
#include "llvm/Support/Debug.h"     // For errs().
#include "llvm/Support/SourceMgr.h" // For SMDiagnostic.

using namespace llvm;

/*
@global = external global ptr, align 8 # 这里的external代表 @global来自external 不是定义是声明
@other_global = local_unnamed_addr global ptr @global, align 8 # 这句是定义 初始值是 @global 的地址
local_unnamed_addr 这个全局的地址在本编译单元内有意义，但不要求跨模块唯一
unnamed_addr 编译器可以假设：它的地址本身没有语义意义，只关心 指向的内容允许：合并相同常量，地址。折叠去重
e.g. void *other_global __attribute__((unnamed_addr)) = &global;

signext 表示：当 i8 被扩展到更大的寄存器/返回值槽位时，用“符号扩展”而不是零扩展
*/
const char *InputIR =
    "@global = external global ptr, align 8\n"
    "@other_global = local_unnamed_addr global ptr @global, align 8\n"
    "\n"
    "define signext i8 @foo() {\n"
    "bb:\n"
    "  %i = load ptr, ptr @global, align 8\n"
    "  %i1 = load i8, ptr %i, align 1\n"
    "  ret i8 %i1\n"
    "}\n"
    "\n"
    "define signext i8 @bar() {\n"
    "bb:\n"
    "  %i = load ptr, ptr @global, align 8\n"
    "  %i1 = load i8, ptr %i, align 1\n"
    "  ret i8 %i1\n"
    "}\n";

int main() {
  LLVMContext Context;
  SMDiagnostic Err;
  std::unique_ptr<Module> MyModule = parseAssemblyString(InputIR, Err, Context);
  Function *BarFunc = MyModule->getFunction("bar");

  BasicBlock &Entry = *BarFunc->begin();

  auto &BarRes = *cast<LoadInst>(Entry.begin()); // 这个是%i = load ptr, ptr @global

  Value *Global = BarRes.getOperand(0);
    /*Value::users() 返回的是：
  所有“使用了这个 Value 的 User”，不管它在不在 bar、不管是不是 Instruction*/
  for (User *UserOfGlobal : Global->users()) {
    outs()  << "UserOfGlobal" << *UserOfGlobal << "\n";
    /*
    Value::users() 是全程序级别的
    不是“局部视角”的 API
    UserOfGlobal  %i = load ptr, ptr @global, align 8
    UserOfGlobal  %i = load ptr, ptr @global, align 8
    UserOfGlobal@other_global = local_unnamed_addr global ptr @global, align 8
    */
    auto *UserInstr = dyn_cast<Instruction>(UserOfGlobal);
    if (!UserInstr) {
      errs() << "Found a non-instruction use of global: " << *UserOfGlobal
             << '\n';
      continue;
    }
    Function *UserFunc = UserInstr->getParent()->getParent();
    if (UserFunc != BarFunc)
      errs() << "Went from bar to " << UserFunc->getName() << '\n';
  }
  return 0;
}
