
include_directories("${MOLLY_SOURCE_DIR}/include/mollyrt")

function(add_molly_example _name)
  add_llvm_example(${_name} ${ARGN})
  set_target_properties(${_name} PROPERTIES FOLDER "Molly")
  target_link_libraries(${_name} MollyRT ${MPI_CXX_LIBRARIES})
endfunction()


add_subdirectory(islpp)
add_subdirectory(stencil)
add_subdirectory(lqcd)
add_subdirectory(gameoflife)
