
add_molly_executable( mollycc driver.cpp cc1as_main.cpp cc1_main.cpp)

target_link_libraries( mollycc 
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
  PollyAnalysis
  LLVMPolly
  LLVMMolly
)
