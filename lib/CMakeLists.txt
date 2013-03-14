
#set(MODULE TRUE)
#set(LLVM_NO_RTTI 1)



add_molly_library(LLVMMolly
  RegisterPasses.cpp
  FieldDetection.cpp
  FieldDistribution.cpp
  FieldType.cpp
  FieldVariable.cpp
  Nodegrid.cpp
  MollyContext.cpp
  MollyContextPass.cpp
  FieldCodeGen.cpp
  ValueSourceAnalysis.cpp
  ScopDistribution.cpp
  ScopStmtSplit.cpp
  MollyUtils.cpp
  MollyInlinePrepa.cpp
  MollyFieldAccess.cpp
)

#target_link_libraries( LLVMMolly ${GMP_LIBRARY} )
#target_link_libraries( LLVMMolly ${ISL_LIBRARY} )
#target_link_libraries( LLVMMolly ${CLOOG_LIBRARY} )
#target_link_libraries( LLVMMolly ${BARVINOK_LIBRARY} )
#target_link_libraries( LLVMMolly ${POLYLIB_LIBRARY} )
#target_link_libraries( LLVMMolly ${LIBGCC_LIBRARY} )
#target_link_libraries( LLVMMolly ${NTL_LIBRARY} )

#set_target_properties(LLVMMolly
#  PROPERTIES
#  LINKER_LANGUAGE CXX
#  PREFIX "")

add_subdirectory(mollyrt)
add_subdirectory(islpp)

target_link_libraries( LLVMMolly Islpp )
