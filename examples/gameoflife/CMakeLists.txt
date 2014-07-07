
### Compile using standard compiler (Will miscompile since this needs mollycc, but adds it to Visual Studio project)
include_directories("${MOLLY_SOURCE_DIR}/include/mollyrt")
add_llvm_example(gameoflife gameoflife.cpp)
set_target_properties(gameoflife PROPERTIES FOLDER "Molly")
target_link_libraries(gameoflife MollyRT ${MPI_CXX_LIBRARIES})



### Compile using mollycc, link to MollyRT that has been compiled using CMAKE_CXX_COMPILER
get_filename_component(gameoflife_path "${LLVM_EXAMPLES_BINARY_DIR}/${CMAKE_EXECUTABLE_PREFIX}gameoflife-mollycc${CMAKE_EXECUTABLE_SUFFIX}" ABSOLUTE)
#get_target_property(MollyRT_LOCATION MollyRT LOCATION)
mollycc_compile("${gameoflife_path}"
  SOURCES gameoflife.cpp
  CFLAGS -mllvm -shape=2x2
  COMMENT "mollycc MollyRT gameoflife.cpp..."
)

add_custom_target(gameoflife-mollycc
  DEPENDS "${gameoflife_path}"
  COMMENT "Built gameoflife using mollycc"
  VERBATIM
)
add_dependencies(gameoflife-mollycc MollyRT)
set_target_properties(gameoflife-mollycc PROPERTIES FOLDER "Molly")





### Compile using mollycc, link to MollyRT-a
get_filename_component(gameoflife_path "${LLVM_EXAMPLES_BINARY_DIR}/${CMAKE_EXECUTABLE_PREFIX}gameoflife-mollycc-a${CMAKE_EXECUTABLE_SUFFIX}" ABSOLUTE)
mollycc_a_compile("${gameoflife_path}"
  SOURCES "gameoflife.cpp"
  FLAGS -mllvm -shape=2x2
  COMMENT "mollycc molly.a gameoflife.cpp..."
)

add_custom_target(gameoflife-mollycc-a
  DEPENDS "${gameoflife_path}"
  COMMENT "Compiling gameoflife using mollycc-a"
  VERBATIM
)
set_target_properties(gameoflife-mollycc-a PROPERTIES FOLDER "Molly")
add_dependencies(gameoflife-mollycc-a MollyRT-a)





add_custom_target(gameoflife-all)

set(target-all gameoflife-all)
set(dimnames X Y)
add_example_config(gameoflife.cpp exe1 target1 VOLUME 8x8 PROC 2x2 BLOCK 4x4)

add_custom_target(submit-gameoflife1
  COMMAND "${MOLLY_SOURCE_DIR}/examples/submit.py" "${CMAKE_CURRENT_SOURCE_DIR}/gameoflife.cpp" exe1=${exe1} --jobfile "${CMAKE_CURRENT_SOURCE_DIR}/gameoflife1.ll" 
  DEPENDS ${target1} ${target2}
)

