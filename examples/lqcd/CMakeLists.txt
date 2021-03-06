
### Compile using standard compiler (Will miscompile since this needs mollycc, but adds it to Visual Studio project)
add_molly_example(lqcd lqcd.cpp)

add_custom_target(run-lqcd
   COMMAND "$<TARGET_FILE:lqcd>" 
   COMMENT "Running ref LQCD..."
)
set_target_properties(run-lqcd PROPERTIES FOLDER "Molly")




### Compile using mollycc, link to MollyRT-a
get_filename_component(lqcd_a_path "${LLVM_EXAMPLES_BINARY_DIR}/${CMAKE_EXECUTABLE_PREFIX}lqcd-mollycc-a${CMAKE_EXECUTABLE_SUFFIX}" ABSOLUTE)
mollycc_a_compile("${lqcd_a_path}"
  SOURCES "lqcd.cpp"
  FLAGS -mllvm -shape=2x2x2x2 -mllvm -polly-only-marked
  COMMENT "mollycc molly.a lqcd.cpp..."
) 

add_custom_target(lqcd-mollycc-a
  DEPENDS "${lqcd_a_path}"
  COMMENT "Compiling lqcd using mollycc-a"
  VERBATIM
)
set_target_properties(lqcd-mollycc-a PROPERTIES FOLDER "Molly")


add_custom_target(run-lqcd-mollycc-a
  COMMAND "${MPIEXEC}" ${MPIEXEC_PREFLAGS} ${MPIEXEC_NUMPROC_FLAG} 16 ${MPIEXEC_POSTFLAGS} "${lqcd_a_path}"
  DEPENDS "${lqcd_a_path}"
  COMMENT "Running lqcd-mollycc-a..."
  VERBATIM
)
set_target_properties(run-lqcd-mollycc-a PROPERTIES FOLDER "Molly")
