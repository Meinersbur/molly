
add_molly_executable(molly molly.cpp)

set(LLVM_LINK_COMPONENTS ${LLVM_TARGETS_TO_BUILD} polly clang LLVMMolly)

target_link_libraries( molly 
  clangFrontendTool
  clangAST
  clangAnalysis
  clangBasic
  clangCodeGen
  clangDriver
  clangEdit
  clangFrontend
  clangLex
  clangParse
  clangEdit
  clangARCMigrate
  clangRewriteCore
  clangRewriteFrontend
  clangSema
  clangSerialization
  clangStaticAnalyzerFrontend
  clangStaticAnalyzerCheckers
  clangStaticAnalyzerCore
  LLVMPolly
  LLVMMolly
)


