
### Compile using standard compiler (Will miscompile since this needs mollycc, but adds it to Visual Studio project)
add_molly_example(jacobi jacobi.cpp)





### Compile using mollycc, link to MollyRT that has been compiled using CMAKE_CXX_COMPILER
get_filename_component(jacobi_path "${LLVM_EXAMPLES_BINARY_DIR}/${CMAKE_EXECUTABLE_PREFIX}jacobi-mollycc${CMAKE_EXECUTABLE_SUFFIX}" ABSOLUTE)
mollycc_compile("${jacobi_path}"
  SOURCES "jacobi.cpp"
  CFLAGS -mllvm -shape=2x2
  COMMENT "mollycc MollyRT jacobi.cpp..."
)

add_custom_target(jacobi-mollycc
  DEPENDS "${jacobi_path}"
  COMMENT "Building jacobi using mollycc"
  VERBATIM
)
set_target_properties(jacobi-mollycc PROPERTIES FOLDER "Molly")





### Compile using mollycc, link to MollyRT-a
get_filename_component(jacobi_a_path "${LLVM_EXAMPLES_BINARY_DIR}/${CMAKE_EXECUTABLE_PREFIX}jacobi-mollycc-a${CMAKE_EXECUTABLE_SUFFIX}" ABSOLUTE)
mollycc_a_compile("${jacobi_a_path}"
  SOURCES "jacobi.cpp"
  FLAGS -mllvm -shape=2x2
  COMMENT "mollycc molly.a jacobi.cpp..."
)

add_custom_target(jacobi-mollycc-a
  DEPENDS "${jacobi_a_path}"
  COMMENT "Compiling jacobi using mollycc-a"
  VERBATIM
)
set_target_properties(jacobi-mollycc-a PROPERTIES FOLDER "Molly")


add_custom_target(run-jacobi-mollycc-a
  COMMAND "${MPIEXEC}" ${MPIEXEC_PREFLAGS} ${MPIEXEC_NUMPROC_FLAG} 4 ${MPIEXEC_POSTFLAGS} "${jacobi_a_path}"
  DEPENDS "${jacobi_a_path}"
  COMMENT "Running jacobi-mollycc-a..."
  VERBATIM
)
set_target_properties(run-jacobi-mollycc-a PROPERTIES FOLDER "Molly")
