#ifndef MOLLY_LLVMFWD_H
#define MOLLY_LLVMFWD_H

namespace llvm {

  // #include <llvm/Support/raw_ostream.h>
  class raw_ostream;
  class raw_string_ostream;
  class raw_svector_ostream;
  class raw_null_ostream;
  class raw_fd_ostream;

  // #include <llvm/ADT/StringRef.h>
  class StringRef;

  // #include <llvm/ADT/Twine.h>
  class Twine;

  // #include <llvm/ADT/SmallVector.h>
  template<typename> class SmallVectorImpl;

  // #include <llvm/ADT/ArrayRef.h>
  template<typename> class ArrayRef;

  // #include <llvm/IR/Instruction.h>
  class Instruction;

  // #include <llvm/IR/Instructions.h>
  class CallInst;

  // #include <llvm/IR/BasicBlock.h>
  class BasicBlock;

  // #include <llvm/IR/Function.h>
  class Function;

  // #include <llvm/IR/Module.h>
  class Module;

  // #include <llvm/IR/Value.h>
  class Value;

  // #include <llvm/IR/Constant.h>
  class Constant;

  // #include <llvm/IR/Constants.h>
  class ConstantInt;

  // #include <llvm/IR/GlobalVariable.h>
  class GlobalVariable;

  // #include <llvm/IR/GlobalValue.h>
  class GlobalValue;

  // #include <llvm/Analysis/RegionInfo.h>
  class RegionInfo;
  class Region;

  // #include <llvm/Pass.h>
  class Pass;
  class ModulePass;
  class FunctionPass;
  class BasicBlockPass;

  // #include <llvm/IR/IRBuilder.h>
  // TODO: Do not pass IRBuilder<> as argument to codegen functions
  class ConstantFolder;
  template<bool preserveNames = true> class IRBuilderDefaultInserter;
  template<bool preserveNames = true, typename T = ConstantFolder, typename Inserter = IRBuilderDefaultInserter<preserveNames>> class IRBuilder;

  // #include <llvm/IR/LLVMContext.h>
  class LLVMContext;

  // #include <llvm/IR/DataLayout.h>
  class DataLayout;

  // #include <llvm/IR/Metadata.h>
  class MDNode;

  // #include <llvm/IR/Type.h>
  class Type;

  // #include <llvm/IR/DerivedTypes.h>
  class StructType;
  class PointerType;

  // #include <llvm/IR/InstrTypes.h>
  class TerminatorInst;

  namespace CodeGen {
     // #include <llvm/CodeGen/FieldTypeMetadata.h>
    class FieldTypeMetadata;

    // #include <llvm/CodeGen/MollyRuntimeMetadata.h>
    class MollyRuntimeMetadata;
  } // namespace CodeGen

} // namespace llvm
#endif /* MOLLY_LLVMFWD_H */
