if(0)
get_filename_component(stencilcpp_rpath stencil.cpp REALPATH)
#message(STATUS "stencil.cpp: ${stencilcpp_rpath}")

# Compile without mollycc
include_directories(${MOLLY_SOURCE_DIR}/include/mollyrt)
add_llvm_executable(stencil stencil.cpp)
set_target_properties(stencil PROPERTIES FOLDER "Molly")
target_link_libraries(stencil MollyRT)
endif()



### Compile using mollycc, link using LTO (using molly.ll)
get_filename_component(stencil_path ${LLVM_EXAMPLES_BINARY_DIR}/stencil-mollycc-ll.exe ABSOLUTE)
mollyccll_compile(${stencil_path} stencil.cpp
  FLAGS -v -O3 -g -mllvm -shape=4 -save-temps
  COMMENT "mollcc molly.ll stencil.cpp"
)
add_custom_target(stencil-mollycc-ll ALL
  DEPENDS ${stencil_path}
# SOURCES
  COMMENT "Compiling stencil using mollycc-ll"
  VERBATIM)
add_dependencies(stencil-mollycc-ll MollyRT-ll)
  

if(0)
### Compile using mollycc, link using LTO
get_filename_component(stencil_mollybc_rpath ${LLVM_EXAMPLES_BINARY_DIR}/stencil-mollybc.exe ABSOLUTE)

mollybc_compile(${stencil_mollybc_rpath} ${stencilcpp_rpath}
  CFLAGS -v -O3 -g -std=c++11 -fms-extensions -target "i686-pc-mingw32" -mllvm -shape=4
  COMMENT "Compiling stencil using mollycc molly.bc...")

add_custom_target(stencil-mollybc ALL 
  DEPENDS ${stencil_mollybc_rpath}
#  SOURCES ${stencilcpp_rpath}
  VERBATIM)
add_dependencies(stencil-mollybc MollyRT)
set_target_properties(stencil-mollybc PROPERTIES FOLDER "Molly")




### Compile using mollycc, link classically
get_filename_component(stencil_mollycc_rpath ${LLVM_EXAMPLES_BINARY_DIR}/stencil-mollycc.exe ABSOLUTE)

mollycc_compile(${stencil_mollycc_rpath} ${stencilcpp_rpath}
  CFLAGS -mllvm -shape=4
  COMMENT "Compiling stencil using mollycc -lMollyRT...")

add_custom_target(stencil-mollycc ALL 
  DEPENDS ${stencil_mollycc_rpath}
#  SOURCES ${stencilcpp_rpath}
  VERBATIM)
add_dependencies(stencil-mollycc MollyRT)
set_target_properties(stencil-mollycc PROPERTIES FOLDER "Molly")
endif()
