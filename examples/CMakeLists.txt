
include_directories("${MOLLY_SOURCE_DIR}/include/mollyrt")

function(add_molly_example _name)
  add_llvm_example(${_name} ${ARGN})
  set_target_properties(${_name} PROPERTIES FOLDER "Molly")
  target_link_libraries(${_name} MollyRT ${MPI_CXX_LIBRARIES}) # For benchmarking stuff
endfunction()

add_subdirectory(islpp)
add_subdirectory(stencil)
add_subdirectory(lqcd)
add_subdirectory(gameoflife)
add_subdirectory(jacobi)
add_subdirectory(gemm)
add_subdirectory(flow)
add_subdirectory(lqcd2d)
add_subdirectory(bench)
