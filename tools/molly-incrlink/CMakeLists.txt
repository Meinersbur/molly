
file( GLOB_RECURSE mollyfiles
  ${MOLLY_SOURCE_DIR}/lib/*.cpp
  ${MOLLY_SOURCE_DIR}/lib/islpp/*.cpp
)

file( GLOB_RECURSE headers
  ${MOLLY_SOURCE_DIR}/include/molly/*.h
  ${MOLLY_SOURCE_DIR}/include/islpp/*.h
  ${MOLLY_SOURCE_DIR}/lib/*.h
  ${MOLLY_SOURCE_DIR}/lib/islpp/*.h
)

add_molly_executable(molly-incrlink ../driver/driver.cpp ../driver/cc1as_main.cpp ../driver/cc1_main.cpp ${mollyfiles} ${headers})

target_link_libraries( molly-incrlink
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
)
