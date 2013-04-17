


#add_llvm_objects_executable( mollycc driver.cpp cc1as_main.cpp cc1_main.cpp
#  OBJECT_LIBS LLVMMolly PollyAnalysis LLVMPolly clangFrontendTool clangAST clangAnalysis clangBasic clangCodeGen clangDriver clangEdit #clangFrontend clangLex clangParse clangEdit clangARCMigrate clangRewriteCore clangRewriteFrontend clangSema clangSerialization #clangStaticAnalyzerFrontend clangStaticAnalyzerCheckers clangStaticAnalyzerCore LLVMSupport
#)
#set_target_properties(mollycc PROPERTIES FOLDER "Molly")

#IF (MSVC_IDE)
#  foreach(lang C CXX)
#      set(CMAKE_${lang}_FLAGS_DEBUG "${CMAKE_${lang}_FLAGS_DEBUG} /ZI")
#  endforeach(lang)
#  set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /SAFESEH:NO")
#ENDIF(MSVC_IDE)


#add_library(mollccobj OBJECT driver.cpp cc1as_main.cpp cc1_main.cpp)
#set_target_properties(mollyccobj PROPERTIES FOLDER "Molly")

#add_llvm_executable(mollycc2 $<TARGET_OBJECTS:mollccobj>)
#set_target_properties(mollycc2 PROPERTIES FOLDER "Molly")

#add_llvm_executable(mollycc3 $<TARGET_OBJECTS:mollccobj>)
#set_target_properties(mollycc3 PROPERTIES FOLDER "Molly")

add_molly_executable(mollycc
  driver.cpp 
  cc1as_main.cpp 
  cc1_main.cpp
  TARGET_DEPENDS clang-headers intrinsics_gen
  LINK_COMPONENTS ${LLVM_TARGETS_TO_BUILD} asmparser bitreader bitwriter irreader codegen instrumentation ipo linker selectiondag
  LINK_LIBS LLVMMolly PollyAnalysis LLVMPolly clangFrontendTool clangAST clangAnalysis clangBasic clangCodeGen clangDriver clangEdit clangFrontend clangLex clangParse clangEdit clangARCMigrate clangRewriteCore clangRewriteFrontend clangSema clangSerialization clangStaticAnalyzerFrontend clangStaticAnalyzerCheckers clangStaticAnalyzerCore LLVMSupport
)
