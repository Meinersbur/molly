
include_directories(${MPI_CXX_INCLUDE_PATH})
add_definitions("-DOMP")
add_llvm_example(benchtest benchtest.cpp)
set_target_properties(benchtest PROPERTIES FOLDER "Molly")
target_link_libraries(benchtest MollyRT ${MPI_CXX_LIBRARIES})
