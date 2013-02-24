
file( GLOB_RECURSE mollyfiles
  ${MOLLY_SOURCE_DIR}/lib/*.cpp
  ${MOLLY_SOURCE_DIR}/lib/islpp/*.cpp
)

file( GLOB_RECURSE mollyheaders
  ${MOLLY_SOURCE_DIR}/include/molly/*.h
  ${MOLLY_SOURCE_DIR}/include/islpp/*.h
  ${MOLLY_SOURCE_DIR}/lib/*.h
  ${MOLLY_SOURCE_DIR}/lib/islpp/*.h
)

file( GLOB_RECURSE pollyfiles
  ${POLLY_SOURCE_DIR}/lib/*.cpp
  ${POLLY_SOURCE_DIR}/lib/Support/*.cpp
  ${POLLY_SOURCE_DIR}/lib/Analysis/*.cpp
  ${POLLY_SOURCE_DIR}/lib/CoddGen/*.cpp
)

file( GLOB_RECURSE pollyheaders
  ${POLLY_SOURCE_DIR}/lib/*.h
  ${POLLY_SOURCE_DIR}/lib/JSON/include/json/*.h
  ${POLLY_SOURCE_DIR}/include/polly/*.h
  ${POLLY_SOURCE_DIR}/include/polly/CodeGen/*.h
  ${POLLY_SOURCE_DIR}/include/polly/Config/*.h
  ${POLLY_SOURCE_DIR}/include/polly/Support/*.h
  ${POLLY_BINARY_DIR}/include/polly/*.h
)

include_directories( ${POLLY_SOURCE_DIR}/include ${POLLY_SOURCE_DIR}/include/polly ${POLLY_BINARY_DIR}/include ${POLLY_BINARY_DIR}/include/polly ${POLLY_SOURCE_DIR}/lib/JSON/include/ )

add_molly_executable(molly-incrlink ../driver/driver.cpp ../driver/cc1as_main.cpp ../driver/cc1_main.cpp ${mollyfiles} ${mollyheaders} ${pollyfiles} ${pollyheaders})

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
)

target_link_libraries( molly-incrlink ${GMP_LIBRARY} )
target_link_libraries( molly-incrlink ${ISL_LIBRARY} )
