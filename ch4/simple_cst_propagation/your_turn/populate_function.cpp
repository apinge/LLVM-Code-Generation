#include "llvm/ADT/PostOrderIterator.h" // For ReversePostOrderTraversal.
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/MachineFrameInfo.h" // For CreateStackObject.
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineMemOperand.h" // For MachinePointerInfo.
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/TargetOpcodes.h"     // For INLINEASM.
#include "llvm/CodeGenTypes/LowLevelType.h" // For LLT.
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h" // For ICMP_EQ.

using namespace llvm;


static llvm::ConstantInt* helper(Instruction & instr, LLVMContext & ctxt, std::optional<APInt>(*compute)(const APInt&, const APInt&) ){
  /* isa<T>(value)
  类似 C++ RTTI 的 dynamic_cast<T*> 检查
  */
  assert(isa<BinaryOperator>(instr) && "binary instruct");
  /*
  dyn_cast<ConstantInt>(value) 返回的类型是 ConstantInt*
  如果转换成功 → 指针指向对象 如果失败 → nullptr
  auto *LHS
  auto 会推导成 ConstantInt（dyn_cast 的模板返回的类型）
  * 表示 这是指针类型 → ConstantInt*
  所以 LHS 的类型是 ConstantInt*
  */
  auto *lhs = dyn_cast<ConstantInt>(instr.getOperand(0));
  auto *rhs = dyn_cast<ConstantInt>(instr.getOperand(1));
  if(!lhs || !rhs) return nullptr;
  std::optional<APInt> res = compute(lhs->getValue(), rhs->getValue());
  if(!res.has_value()) return nullptr;
  return ConstantInt::get(ctxt,*res);
}


// Takes \p Foo and apply a simple constant propagation optimization.
// \returns true if \p Foo was modified (i.e., something had been constant
// propagated), false otherwise.
bool myConstantPropagation(Function &Foo) {
  // TODO: populate this function.
  if (Foo.empty())
  {
    return false;
  }
  // getParent得到Module类型 再getContext，Module才有context
  LLVMContext &ctx =Foo.getParent()->getContext();
  bool change = false;
  ReversePostOrderTraversal<Function *> rpot(&Foo);
 
  for(BasicBlock* bb: rpot){
     /*
      for (Instruction &I : make_early_inc_range(BB)) {
      if (isDead(I))
        I.eraseFromParent();
        make_early_inc_range 是一种“允许边遍历边删当前元素”的 iterator 技巧，
        本质是把 ++it 提前到 *it 时执行
      */
    for (Instruction &instr : make_early_inc_range(*bb)) {
        auto op = instr.getOpcode();
        llvm::ConstantInt* new_const = nullptr;
        //Value* 
        //Value* new_const = nullptr;
        if(op==Instruction::Add){
            new_const = helper(instr,ctx,[](const APInt &A, const APInt &B) -> std::optional<APInt> {
              return A + B;
            });
        }
        else if(op==Instruction::Sub){
            new_const = helper(instr,ctx,[](const APInt &A, const APInt &B) -> std::optional<APInt> {
              return A - B;
            });
        }
        else if(op==Instruction::Mul){
            new_const = helper(instr,ctx,[](const APInt &A, const APInt &B) -> std::optional<APInt> {
              return A * B;
            });
        }
        else if(op==Instruction::SDiv){
            new_const = helper(instr,ctx,[](const APInt &A, const APInt &B) -> std::optional<APInt> {
              if(B.isZero()) return std::nullopt;
              return A.sdiv(B);
            });
        }
        
        else if (op==Instruction::UDiv){
            new_const = helper(instr,ctx,[](const APInt &A, const APInt &B) -> std::optional<APInt> {
              if(B.isZero()) return std::nullopt;
              return A.udiv(B);
            });
        }
        //  Shl: LLVM IR 对左移没有区分逻辑/算术，因为左移对符号位没有特殊处理
        // LShr = 无符号逻辑右移，高位补 0；AShr = 有符号算术右移，高位补符号位
        else if(op==Instruction::LShr){
           new_const = helper(instr,ctx,[](const APInt &A, const APInt &B) -> std::optional<APInt> {
             if (B.uge(A.getBitWidth()))
                return std::nullopt;
              return A.lshr(B);
            });

        }
        else if(op==Instruction::AShr){
           new_const = helper(instr,ctx,[](const APInt &A, const APInt &B) -> std::optional<APInt> {
              return A.ashr(B);
            });

        }
        else if(op==Instruction::Shl){
           new_const = helper(instr,ctx,[](const APInt &A, const APInt &B) -> std::optional<APInt> {
            /*
            https://llvm.org/docs/UndefinedBehavior.html#deferred-ub
            这里是检查 B的位宽不能大于A否则ub了
            比如 a = 1 b = 32
            1<<32 就ub了
            */
             if (B.uge(A.getBitWidth()))
                return std::nullopt;
              return A.shl(B);
            });

        }
        else if(op==Instruction::And){
           new_const = helper(instr,ctx,[](const APInt &A, const APInt &B) -> std::optional<APInt> {
              return A&B;
            });

        }
        else if (op==Instruction::Or){
           new_const = helper(instr,ctx,[](const APInt &A, const APInt &B) -> std::optional<APInt> {
              return A|B;
            });

        }
        else if(op==Instruction::Xor){
           new_const = helper(instr,ctx,[](const APInt &A, const APInt &B) -> std::optional<APInt> {
              return A^B;
            });

        }
        if(new_const){
          instr.replaceAllUsesWith(new_const);
          instr.eraseFromParent(); //和make_early_inc_range配合使用
          change = true;
        }

      }
  }
  
  
  return change;
}
