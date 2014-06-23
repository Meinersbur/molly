
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
  FLAGS -mllvm -shape=2 -mllvm -polly-only-marked
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



add_custom_target(gemm-all)

set(target-all gemm-all)
set(dimnames Y)
add_example_config(gemm.cpp exe1 target1 VOLUME 16 PROC 2 BLOCK 8 DEFINES "LX=16" "LZ=16")

add_custom_target(submit-gemm1
  COMMAND "${MOLLY_SOURCE_DIR}/examples/submit.py" "${CMAKE_CURRENT_SOURCE_DIR}/gemm.cpp" exe1=${exe1} --jobfile "${CMAKE_CURRENT_SOURCE_DIR}/gemm1.ll" 
  DEPENDS ${target1}
)

