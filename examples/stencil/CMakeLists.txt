
### Compile using standard compiler
get_filename_component(stencil_path ${LLVM_EXAMPLES_BINARY_DIR}/stencil.exe ABSOLUTE)
include_directories(${MOLLY_SOURCE_DIR}/include/mollyrt)
add_llvm_example(stencil stencil.cpp)
set_target_properties(stencil PROPERTIES FOLDER "Molly")
target_link_libraries(stencil MollyRT ${MPI_CXX_LIBRARIES})



### Compile using mollycc, link to MollyRT
get_filename_component(stencil_path ${LLVM_EXAMPLES_BINARY_DIR}/stencil-mollycc.exe ABSOLUTE)
mollycc_compile(${stencil_path} 
  SOURCES stencil.cpp
  FLAGS -O3 -mllvm -shape=4 -save-temps
  LIBS $<TARGET_FILE:MollyRT> ${MPI_CXX_LIBRARIES}
  COMMENT "mollcc MollyRT stencil.cpp...")

add_custom_target(stencil-mollycc ALL
  DEPENDS ${stencil_path}
  COMMENT "Built stencil using mollycc"
  VERBATIM)
set_target_properties(stencil-mollycc PROPERTIES FOLDER "Molly")



### Compile using mollycc, link to MollyRT-o
get_filename_component(stencil_path ${LLVM_EXAMPLES_BINARY_DIR}/stencil-mollycc-o.exe ABSOLUTE)
mollycc_o_compile(${stencil_path} 
  SOURCES stencil.cpp
  FLAGS -O3 -mllvm -shape=4
  COMMENT "mollcc molly.o stencil.cpp...")

add_custom_target(stencil-mollycc-o ALL
  DEPENDS ${stencil_path}
  COMMENT "Compiling stencil using mollycc-a"
  VERBATIM)
set_target_properties(stencil-mollycc-o PROPERTIES FOLDER "Molly")
add_dependencies(stencil-mollycc-o MollyRT-o)



### Compile using mollycc, link to MollyRT-a
get_filename_component(stencil_path ${LLVM_EXAMPLES_BINARY_DIR}/stencil-mollycc-a.exe ABSOLUTE)
mollycc_a_compile(${stencil_path} 
  SOURCES stencil.cpp
  FLAGS -O3 -mllvm -shape=4
  COMMENT "mollcc molly.a stencil.cpp...")

add_custom_target(stencil-mollycc-a ALL
  DEPENDS ${stencil_path}
  COMMENT "Compiling stencil using mollycc-a"
  VERBATIM)
set_target_properties(stencil-mollycc-a PROPERTIES FOLDER "Molly")
add_dependencies(stencil-mollycc-a MollyRT-a)



### Compile using mollycc, link to MollyRT-ll
get_filename_component(stencil_path ${LLVM_EXAMPLES_BINARY_DIR}/stencil-mollycc-ll.exe ABSOLUTE)
mollycc_ll_compile(${stencil_path} 
  SOURCES stencil.cpp
  FLAGS -O3 -mllvm -shape=4
  COMMENT "mollcc molly.ll stencil.cpp...")

add_custom_target(stencil-mollycc-ll ALL
  DEPENDS ${stencil_path}
  COMMENT "Compiling stencil using mollycc-ll"
  VERBATIM)
set_target_properties(stencil-mollycc-ll PROPERTIES FOLDER "Molly")
add_dependencies(stencil-mollycc-ll MollyRT-ll)
  


### Compile using mollycc, link to MollyRT-bc
get_filename_component(stencil_path ${LLVM_EXAMPLES_BINARY_DIR}/stencil-mollycc-bc.exe ABSOLUTE)
mollycc_bc_compile(${stencil_path} 
  SOURCES stencil.cpp
  FLAGS -O3 -mllvm -shape=4
  COMMENT "mollcc molly.bc stencil.cpp...")

add_custom_target(stencil-mollycc-bc ALL
  DEPENDS ${stencil_path}
  COMMENT "Compiling stencil using mollycc-bc"
  VERBATIM)
set_target_properties(stencil-mollycc-bc PROPERTIES FOLDER "Molly")
add_dependencies(stencil-mollycc-bc MollyRT-bc)
