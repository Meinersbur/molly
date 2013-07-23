#ifndef MOLLY_LLVMFWD_H
#define MOLLY_LLVMFWD_H

namespace llvm {

  // llvm/Support/raw_ostream.h
  class raw_ostream;

  // llvm/ADT/SmallVector.h
  template<typename T> class SmallVectorImpl;

  // llvm/IR/Instruction.h
  class Instruction;

  // llvm/IR/Instructions.h
  class CallInst;

  // llvm/IR/BasicBlock.h
  class BasicBlock;

  // llvm/IR/Function.h
  class Function;

  // llvm/IR/Module.h
  class Module;

  // llvm/Analysis/RegionInfo.h
  class RegionInfo;
  class Region;

  // llvm/Pass.h
  class Pass;

} // namespace llvm
#endif /* MOLLY_LLVMFWD_H */
