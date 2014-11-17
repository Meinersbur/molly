
### Compile using standard compiler (without WITH_MOLLY being defined)
add_molly_example(gemm gemm.cpp)

add_custom_target(run-gemm
   COMMAND "$<TARGET_FILE:gemm>" 
   COMMENT "Running ref gemm..."
)


### Compile using mollycc, link to MollyRT-a
get_filename_component(gemm_a_path "${LLVM_EXAMPLES_BINARY_DIR}/${CMAKE_EXECUTABLE_PREFIX}gemm-mollycc-a${CMAKE_EXECUTABLE_SUFFIX}" ABSOLUTE)
mollycc_a_compile("${gemm_a_path}"
  SOURCES "gemm.cpp"
  FLAGS -mllvm -shape=4 -mllvm -polly-only-marked -DLY=32  -DPY=4 -DPY=4 -DBY=8
  COMMENT "mollycc molly.a gemm.cpp..."
)
add_custom_target(gemm-mollycc-a
  DEPENDS "${gemm_a_path}"
  COMMENT "Compiling gemm using mollycc-a"
  VERBATIM
)
set_target_properties(gemm-mollycc-a PROPERTIES FOLDER "Molly")

add_custom_target(run-gemm-mollycc-a
  COMMAND "${MPIEXEC}" ${MPIEXEC_PREFLAGS} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_POSTFLAGS} "${gemm_a_path}"
  DEPENDS "${gemm_a_path}"
  COMMENT "Running gemm-mollycc-a..."
  VERBATIM
)
set_target_properties(run-gemm-mollycc-a PROPERTIES FOLDER "Molly")
