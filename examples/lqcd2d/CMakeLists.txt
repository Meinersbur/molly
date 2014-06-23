
### Compile using standard compiler (Will miscompile since this needs mollycc, but adds it to Visual Studio project)
add_molly_example(lqcd2d lqcd2d.cpp)

add_custom_target(run-lqcd2d
   COMMAND "$<TARGET_FILE:lqcd2d>" 
   COMMENT "Running ref LQCD 2D..."
)
set_target_properties(run-lqcd2d PROPERTIES FOLDER "Molly")




### Compile using mollycc, link to MollyRT-a
get_filename_component(lqcd2d_a_path "${LLVM_EXAMPLES_BINARY_DIR}/${CMAKE_EXECUTABLE_PREFIX}lqcd2d-mollycc-a${CMAKE_EXECUTABLE_SUFFIX}" ABSOLUTE)
mollycc_a_compile("${lqcd2d_a_path}"
  SOURCES "lqcd2d.cpp"
  FLAGS -mllvm -shape=2x2 -mllvm -polly-only-marked
  COMMENT "mollycc molly.a lqcd.cpp..."
) 

add_custom_target(lqcd2d-mollycc-a
  DEPENDS "${lqcd2d_a_path}"
  COMMENT "Compiling lqcd2d using mollycc-a"
  VERBATIM
)
set_target_properties(lqcd2d-mollycc-a PROPERTIES FOLDER "Molly")


add_custom_target(run-lqcd2d-mollycc-a
  COMMAND "${MPIEXEC}" ${MPIEXEC_PREFLAGS} ${MPIEXEC_NUMPROC_FLAG} 4 ${MPIEXEC_POSTFLAGS} "${lqcd2d_a_path}"
  DEPENDS "${lqcd2d_a_path}"
  COMMENT "Running lqcd2d-mollycc-a..."
  VERBATIM
)
set_target_properties(run-lqcd2d-mollycc-a PROPERTIES FOLDER "Molly")



add_custom_target(lqcd2d-all)
#add_dependencies(lqcd-all lqcd)

set(target-all lqcd2d-all)
set(dimnames T X)
add_example_config(lqcd2d.cpp exe1 target1 VOLUME 8x8 PROC 2x2 BLOCK 4x4 DEFINES "WITH_KAMUL=1")
add_example_config(lqcd2d.cpp exe2 target2 VOLUME 8x8 PROC 2x2 BLOCK 4x4 DEFINES "WITH_KAMUL=0")

add_custom_target(submit-lqcd2d1
  COMMAND "${MOLLY_SOURCE_DIR}/examples/submit.py" "${CMAKE_CURRENT_SOURCE_DIR}/lqcd2d.cpp" exe1=${exe1} exe2=${exe2} --jobfile "${CMAKE_CURRENT_SOURCE_DIR}/lqcd2d.ll" 
  DEPENDS ${target1} ${target2}
)
