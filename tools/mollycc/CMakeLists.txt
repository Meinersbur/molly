
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

#IF (MSVC_IDE)
#  foreach(lang C CXX)
#      set(CMAKE_${lang}_FLAGS_DEBUG "${CMAKE_${lang}_FLAGS_DEBUG} /ZI")
#  endforeach(lang)
#  set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /SAFESEH:NO")
#ENDIF(MSVC_IDE)
