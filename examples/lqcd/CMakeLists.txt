
### Compile using standard compiler (Will miscompile since this needs mollycc, but adds it to Visual Studio project)
add_molly_example(lqcd lqcd.cpp)





### Compile using mollycc, link to MollyRT that has been compiled using CMAKE_CXX_COMPILER
get_filename_component(lqcd_path "${LLVM_EXAMPLES_BINARY_DIR}/${CMAKE_EXECUTABLE_PREFIX}lqcd-mollycc${CMAKE_EXECUTABLE_SUFFIX}" ABSOLUTE)
mollycc_compile("${lqcd_path}"
  SOURCES "lqcd.cpp"
  CFLAGS -mllvm -shape=2x2x2x2 -mllvm -polly-only-func=HoppingMatrix
  COMMENT "mollycc MollyRT lqcd.cpp..."
)

add_custom_target(lqcd-mollycc
  DEPENDS "${lqcd_path}"
  COMMENT "Building lqcd using mollycc"
  VERBATIM
)
set_target_properties(lqcd-mollycc PROPERTIES FOLDER "Molly")





### Compile using mollycc, link to MollyRT-a
get_filename_component(lqcd_path "${LLVM_EXAMPLES_BINARY_DIR}/${CMAKE_EXECUTABLE_PREFIX}lqcd-mollycc-a${CMAKE_EXECUTABLE_SUFFIX}" ABSOLUTE)
mollycc_a_compile("${lqcd_path}"
  SOURCES "lqcd.cpp"
  FLAGS -mllvm -shape=2 -mllvm -polly-only-func=HoppingMatrix
  COMMENT "mollycc molly.a lqcd.cpp..."
)

add_custom_target(lqcd-mollycc-a
  DEPENDS "${lqcd_path}"
  COMMENT "Compiling lqcd using mollycc-a"
  VERBATIM
)
set_target_properties(lqcd-mollycc-a PROPERTIES FOLDER "Molly")
#add_dependencies(lqcd-mollycc-a MollyRT-a)


add_custom_target(run-lqcd-mollycc-a
  COMMAND "${MPIEXEC}" ${MPIEXEC_PREFLAGS} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_POSTFLAGS} "${lqcd_path}"
  DEPENDS "${lqcd_path}"
  COMMENT "Running lqcd-mollycc-a..."
  VERBATIM
)
set_target_properties(run-lqcd-mollycc-a PROPERTIES FOLDER "Molly")
