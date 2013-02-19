
#set(MODULE TRUE)
#set(LLVM_NO_RTTI 1)



add_molly_library(LLVMMolly
  RegisterPasses.cpp
  FieldDetection.cpp
  FieldDistribution.cpp
  islpp/cstdiofile.cpp
  islpp/Set.cpp
  islpp/Space.cpp
  islpp/LocalSpace.cpp
  islpp/Ctx.cpp
  islpp/BasicSet.cpp
  islpp/Constraint.cpp
  islpp/Printer.cpp
  islpp/BasicMap.cpp
  islpp/Map.cpp
  islpp/UnionMap.cpp
  islpp/UnionSet.cpp
  islpp/Id.cpp
  islpp/Int.cpp
  islpp/link.cpp
  islpp/Aff.cpp
  islpp/MultiAff.cpp
  islpp/Point.cpp
  islpp/PwQPolynomialFold.cpp
  islpp/Mat.cpp
  islpp/Vertices.cpp
  islpp/SetList.cpp
  islpp/UnionPwMultiAff.cpp
  islpp/QPolynomial.cpp
  islpp/PwQPolynomial.cpp
  islpp/UnionPwQPolynomial.cpp
  islpp/UnionPwQPolynomialFold.cpp
  islpp/Dim.cpp
  islpp/PwAff.cpp
  FieldType.cpp
  FieldVariable.cpp
  Nodegrid.cpp
  MollyContext.cpp
  MollyContextPass.cpp
)
  
target_link_libraries( LLVMMolly ${GMP_LIBRARY} )
target_link_libraries( LLVMMolly ${ISL_LIBRARY} )
target_link_libraries( LLVMMolly ${CLOOG_LIBRARY} )
target_link_libraries( LLVMMolly ${BARVINOK_LIBRARY} )
target_link_libraries( LLVMMolly ${POLYLIB_LIBRARY} )
target_link_libraries( LLVMMolly ${LIBGCC_LIBRARY} )
target_link_libraries( LLVMMolly ${NTL_LIBRARY} )

#set_target_properties(LLVMMolly
#  PROPERTIES
#  LINKER_LANGUAGE CXX
#  PREFIX "")

add_subdirectory(mollyrt)
