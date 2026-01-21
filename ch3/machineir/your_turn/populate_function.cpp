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

// The goal of this function is to build a MachineFunction that
// represents the lowering of the following foo, a C function:
// extern int baz();
// extern void bar(int);
// void foo(int a, int b) {
//   int var = a + b;
//   if (var == 0xFF) {
//     bar(var);
//     var = baz();
//   }
//   bar(var);
// }
//
// The proposed ABI is:
// - 32-bit arguments are passed through registers: w0, w1
// - 32-bit returned values are passed through registers: w0, w1
// w0 and w1 are given as argument of this Function.
//
// The local variable named var is expected to live on the stack.
MachineFunction *populateMachineIR(MachineModuleInfo &MMI, Function &Foo,
                                   Register W0, Register W1) {
  MachineFunction &MF = MMI.getOrCreateMachineFunction(Foo);

  // The type for bool.
  LLT I1 = LLT::scalar(1);
  // The type of var.
  LLT I32 = LLT::scalar(32);

  // To use to create load and store for var.
  MachinePointerInfo PtrInfo;
  Align VarStackAlign(4);

  // The type for the address of var.
  LLT VarAddrLLT = LLT::pointer(/*AddressSpace=*/0, /*SizeInBits=*/64);

  // The stack slot for var.
  int FrameIndex = MF.getFrameInfo().CreateStackObject(32, VarStackAlign,
                                                       /*IsSpillSlot=*/false);

  // TODO: Populate MF.
  llvm::MachineBasicBlock*BB1 = MF.CreateMachineBasicBlock();
  MF.push_back(BB1);
  const TargetInstrInfo *TII = MF.getSubtarget().getInstrInfo();
  llvm::MachineBasicBlock*BB2 = MF.CreateMachineBasicBlock();
  MF.push_back(BB2);
  llvm::MachineBasicBlock*BB3 = MF.CreateMachineBasicBlock();
  MF.push_back(BB3);
  BB1->addSuccessor(BB2);
  BB1->addSuccessor(BB3);
  BB2->addSuccessor(BB3);
  MachineIRBuilder MIBuilder(*BB1, BB1->end());
  Register A = MIBuilder.buildCopy(I32, W0).getReg(0);
  Register B = MIBuilder.buildCopy(I32, W1).getReg(0);
  // Get the stack slot for var.
  Register VarStackAddr = MIBuilder.buildFrameIndex(VarAddrLLT, FrameIndex).getReg(0);
  Register ResAdd = MIBuilder.buildAdd(I32, A, B).getReg(0);
  MIBuilder.buildStore(ResAdd, VarStackAddr, PtrInfo, VarStackAlign);
  Register Cst0xFF = MIBuilder.buildConstant(I32, 0xFF).getReg(0);
  Register ReloadedVar0 =
      MIBuilder.buildLoad(I32, VarStackAddr, PtrInfo, VarStackAlign).getReg(0);
  Register Cmp =
      MIBuilder.buildICmp(CmpInst::ICMP_EQ, I1, ReloadedVar0, Cst0xFF)
          .getReg(0);
   // If true jump to ThenBB.
  MIBuilder.buildBrCond(Cmp, *BB2);
  // Otherwise jump to ExitBB;
  MIBuilder.buildBr(*BB3);

  //BB1
  // Reset MIBuilder to point at the end of BB1.
   MIBuilder.setInsertPt(*BB2, BB2->end());
  // Put var in W0 for the call to bar.
  Register ReloadedVar1 =
      MIBuilder.buildLoad(I32, VarStackAddr, PtrInfo, VarStackAlign).getReg(0);
  MIBuilder.buildCopy(W0, ReloadedVar1);
  // Fake call to bar.
  MIBuilder.buildInstr(TargetOpcode::INLINEASM, {}, {})
      .addExternalSymbol("bl @bar")
      .addImm(0)
      .addReg(W0, RegState::Implicit);
  // Fake call to baz.
  MIBuilder.buildInstr(TargetOpcode::INLINEASM, {}, {})
      .addExternalSymbol("bl @baz")
      .addImm(0)
      .addReg(W0, RegState::Implicit | RegState::Define);
  // Copy the result of baz to var.
  Register ResOfBaz = MIBuilder.buildCopy(I32, W0).getReg(0);
  MIBuilder.buildStore(ResOfBaz, VarStackAddr, PtrInfo, VarStackAlign);


  //BB3
  MIBuilder.setInsertPt(*BB3, BB3->end());
  // Put var in W0 for the call to bar.
  Register ReloadedVar2 =
      MIBuilder.buildLoad(I32, VarStackAddr, PtrInfo, VarStackAlign).getReg(0);
  MIBuilder.buildCopy(W0, ReloadedVar2);
  // Fake call to bar.
  MIBuilder.buildInstr(TargetOpcode::INLINEASM, {}, {})
      .addExternalSymbol("bl @bar")
      .addImm(0)
      .addReg(W0, RegState::Implicit);
  // End of the function, return void;
  MIBuilder.buildInstr(TargetOpcode::INLINEASM, {}, {})
      .addExternalSymbol("ret")
      .addImm(0);
  return &MF;
}
