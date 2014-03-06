
### Compile using standard compiler (Will miscompile since this needs mollycc, but adds it to Visual Studio project)
include_directories("${MOLLY_SOURCE_DIR}/include/mollyrt")
add_llvm_example(lqcd lqcd.cpp)
set_target_properties(lqcd PROPERTIES FOLDER "Molly")
target_link_libraries(lqcd MollyRT ${MPI_CXX_LIBRARIES})




### Compile using mollycc, link to MollyRT that has been compiled using CMAKE_CXX_COMPILER
get_filename_component(lqcd_path "${LLVM_EXAMPLES_BINARY_DIR}/${CMAKE_EXECUTABLE_PREFIX}lqcd-mollycc${CMAKE_EXECUTABLE_SUFFIX}" ABSOLUTE)
#get_target_property(MollyRT_LOCATION MollyRT LOCATION)
mollycc_compile("${lqcd_path}"
  SOURCES "lqcd.cpp"
  CFLAGS -mllvm -shape=2x2x2x2
  COMMENT "mollycc MollyRT lqcd.cpp..."
)

add_custom_target(lqcd-mollycc
  DEPENDS "${gameoflife_path}"
  COMMENT "Built lqcd using mollycc"
  VERBATIM
)
add_dependencies(lqcd-mollycc MollyRT)
set_target_properties(lqcd-mollycc PROPERTIES FOLDER "Molly")





### Compile using mollycc, link to MollyRT-a
get_filename_component(lqcd_path "${LLVM_EXAMPLES_BINARY_DIR}/${CMAKE_EXECUTABLE_PREFIX}lqcd-mollycc-a${CMAKE_EXECUTABLE_SUFFIX}" ABSOLUTE)
mollycc_a_compile("${lqcd_path}"
  SOURCES "lqcd.cpp"
  FLAGS -mllvm -shape=2x2x2x2
  COMMENT "mollycc molly.a lqcd.cpp..."
)

add_custom_target(lqcd-mollycc-a
  DEPENDS "${lqcd_path}"
  COMMENT "Compiling lqcd using mollycc-a"
  VERBATIM
)
set_target_properties(lqcd-mollycc-a PROPERTIES FOLDER "Molly")
add_dependencies(lqcd-mollycc-a MollyRT-a)


