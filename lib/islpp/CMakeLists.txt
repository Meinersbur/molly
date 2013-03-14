
if (MSVC_IDE OR XCODE)
  file( GLOB_RECURSE headers
      ${MOLLY_SOURCE_DIR}/include/islpp/*.h
      ${MOLLY_SOURCE_DIR}/lib/islpp/*.h
    )
  set_source_files_properties(${headers} PROPERTIES HEADER_FILE_ONLY ON)
endif (MSVC_IDE OR XCODE)

add_llvm_library(Islpp
  cstdiofile.cpp
  Set.cpp
  Space.cpp
  LocalSpace.cpp
  Ctx.cpp
  BasicSet.cpp
  Constraint.cpp
  Printer.cpp
  BasicMap.cpp
  Map.cpp
  UnionMap.cpp
  UnionSet.cpp
  Id.cpp
  Int.cpp
  link.cpp
  Aff.cpp
  MultiAff.cpp
  Point.cpp
  PwQPolynomialFold.cpp
  Mat.cpp
  Vertices.cpp
  SetList.cpp
  UnionPwMultiAff.cpp
  QPolynomial.cpp
  PwQPolynomial.cpp
  UnionPwQPolynomial.cpp
  UnionPwQPolynomialFold.cpp
  Dim.cpp
  PwAff.cpp
  DimRange.cpp
  PwMultiAff.cpp
  Pw.cpp
  Multi.cpp
  Spacelike.cpp
  ${headers}
)
set_target_properties(Islpp PROPERTIES FOLDER "Molly")

target_link_libraries( Islpp ${GMP_LIBRARY} )
target_link_libraries( Islpp ${ISL_LIBRARY} )
