


### Compile using mollycc, link to MollyRT
get_filename_component(lqcd_path "${LLVM_EXAMPLES_BINARY_DIR}/lqcd-mollycc${CMAKE_EXECUTABLE_PREFIX}" ABSOLUTE)
get_target_property(MollyRT_LOCATION MollyRT LOCATION)
mollycc_compile(${lqcd_path} 
  SOURCES lqcd.cpp
  FLAGS -mllvm -shape=4
  LIBS ${MollyRT_LOCATION} ${MPI_CXX_LIBRARIES}
  COMMENT "mollycc MollyRT lqcd.cpp...")

add_custom_target(lqcd-mollycc ALL
  DEPENDS ${lqcd_path}
  COMMENT "Built lqcd using mollycc"
  VERBATIM)
add_dependencies(lqcd-mollycc MollyRT)
set_target_properties(lqcd-mollycc PROPERTIES FOLDER "Molly")
