
# Compile without molly
include_directories( ${MOLLY_SOURCE_DIR}/include/mollyrt )
add_llvm_executable(stencil stencil.cpp)
set_target_properties(stencil PROPERTIES FOLDER "Molly")
target_link_libraries(stencil MollyRT)

# Compile using molly
mollycc_compile(stencil-mollycc stencil.cpp
  CFLAGS -mllvm -shape=4
  DEPS
  )

add_custom_target(stencil-mollycc ALL 
  DEPENDS ${LLVM_BINARY_DIR}/lib/molly.bc molly.ll MollyRT 
  COMMENT "Compiling stencil using mollycc...")
set_target_properties(stencil PROPERTIES FOLDER "Molly")


