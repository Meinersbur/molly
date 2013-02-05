
add_molly_executable(molly driver.cpp cc1as_main.cpp cc1_main.cpp)

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


