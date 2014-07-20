
include_directories(${MPI_CXX_INCLUDE_PATH})
add_definitions("-DOMP")
add_molly_example(benchtest benchtest.cpp)
set_target_properties(benchtest PROPERTIES FOLDER "Molly")
target_link_libraries(benchtest MollyRT ${MPI_CXX_LIBRARIES})

add_custom_target(run-benchtest
   COMMAND "$<TARGET_FILE:benchtest>"
   COMMENT "Running ref Benchtest..."
)
set_target_properties(run-benchtest PROPERTIES FOLDER "Molly")
