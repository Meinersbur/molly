
#IF (MSVC_IDE)
#  foreach(lang C CXX)
#      set(CMAKE_${lang}_FLAGS_DEBUG "${CMAKE_${lang}_FLAGS_DEBUG} /ZI")
#  endforeach(lang)
#  set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /SAFESEH:NO")
#ENDIF(MSVC_IDE)

set(LLVM_LINK_COMPONENTS
  ${LLVM_TARGETS_TO_BUILD}
  asmparser
  bitreader
  bitwriter
  irreader
  codegen
  instrumentation
  ipo
  linker
  selectiondag
  )
  
set(LLVM_LINK_LIBS
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
  clangSema
  clangSerialization
)
  
if(CLANG_ENABLE_STATIC_ANALYZER)
  list(APPEND LLVM_LINK_LIBS
    clangStaticAnalyzerFrontend
    clangStaticAnalyzerCheckers
    clangStaticAnalyzerCore
    )
endif()

if(CLANG_ENABLE_ARCMT)
  list(APPEND LLVM_LINK_LIBS
    clangARCMigrate
    )
endif()

if(CLANG_ENABLE_REWRITER)
  list(APPEND LLVM_LINK_LIBS
    clangRewriteCore
    clangRewriteFrontend
    )
endif()

add_molly_executable(mollycc
    driver.cpp 
    cc1as_main.cpp 
    cc1_main.cpp
#  TARGET_DEPENDS 
#    clang-headers 
#    intrinsics_gen
#  LINK_COMPONENTS 
#    ${LLVM_TARGETS_TO_BUILD}
#    asmparser
#    bitreader
#    bitwriter
#    irreader
#    codegen
#    instrumentation
#    ipo
#    linker
#    selectiondag
  LINK_LIBS 
    LLVMMolly
    Polly
)

#add_dependencies(mollycc     clang-headers     intrinsics_gen)
target_link_libraries(mollycc ${LLVM_LINK_LIBS})
