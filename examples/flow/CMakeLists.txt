
### Compile using standard compiler (without WITH_MOLLY being defined)
add_molly_example(flow flow.cpp)

add_custom_target(run-flow
   COMMAND "$<TARGET_FILE:flow>" 
   COMMENT "Running ref flow..."
)


### Compile using mollycc, link to MollyRT-a
get_filename_component(flow_a_path "${LLVM_EXAMPLES_BINARY_DIR}/${CMAKE_EXECUTABLE_PREFIX}flow-mollycc-a${CMAKE_EXECUTABLE_SUFFIX}" ABSOLUTE)
mollycc_a_compile("${flow_a_path}"
  SOURCES "flow.cpp"
  FLAGS -mllvm -shape=2 -mllvm -polly-only-marked
  COMMENT "mollycc molly.a flow.cpp..."
)

add_custom_target(flow-mollycc-a
  DEPENDS "${flow_a_path}"
  COMMENT "Compiling flow using mollycc-a"
  VERBATIM
)
set_target_properties(flow-mollycc-a PROPERTIES FOLDER "Molly")

add_custom_target(run-flow-mollycc-a
  COMMAND "${MPIEXEC}" ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS} "${flow_a_path}" ${MPIEXEC_POSTFLAGS} ""
  DEPENDS "${flow_a_path}"
  COMMENT "Running flow-mollycc-a..."
  VERBATIM
)
set_target_properties(run-flow-mollycc-a PROPERTIES FOLDER "Molly")





add_custom_target(flow-all)
set(target-all flow-all)
set(dimnames X Y)
add_example_config(flow.cpp exe1 target1 VOLUME 8x8 PROC 2x1 BLOCK 4x8)
add_example_config(flow.cpp exe2 target2 VOLUME 8x8 PROC 2x2 BLOCK 4x4)

add_custom_target(submit-flow1
  COMMAND "${MOLLY_SOURCE_DIR}/examples/submit.py" "${CMAKE_CURRENT_SOURCE_DIR}/flow.cpp" ${exe1} ${exe2} --jobfile "${CMAKE_CURRENT_SOURCE_DIR}/flow1.ll" 
  DEPENDS ${target1} ${target2}
)
