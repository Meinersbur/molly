
### Compile using standard compiler (without WITH_MOLLY being defined)
add_molly_example(jacobi jacobi.cpp)

add_custom_target(run-jacobi
   COMMAND "$<TARGET_FILE:jacobi>" 
   COMMENT "Running ref jacobi..."
)


### Compile using mollycc, link to MollyRT-a
get_filename_component(jacobi_a_path "${LLVM_EXAMPLES_BINARY_DIR}/${CMAKE_EXECUTABLE_PREFIX}jacobi-mollycc-a${CMAKE_EXECUTABLE_SUFFIX}" ABSOLUTE)
mollycc_a_compile("${jacobi_a_path}"
  SOURCES "jacobi.cpp"
  FLAGS -mllvm -shape=2x2 -mllvm -polly-only-marked
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




add_custom_target(jacobi-all)

set(target-all jacobi-all)
set(dimnames X Y)
add_example_config(jacobi.cpp exe1 target1 VOLUME 8x8 PROC 2x2 BLOCK 4x4)

add_custom_target(submit-jacobi1
  COMMAND "${MOLLY_SOURCE_DIR}/examples/submit.py" "${CMAKE_CURRENT_SOURCE_DIR}/jacobi.cpp" exe1=${exe1} --jobfile "${CMAKE_CURRENT_SOURCE_DIR}/jacobi1.ll" 
  DEPENDS ${target1} ${target2}
)
