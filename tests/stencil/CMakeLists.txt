
include_directories( ${MOLLY_SOURCE_DIR}/include/mollyrt )
add_llvm_executable(stencil stencil.cpp)
set_target_properties(stencil PROPERTIES FOLDER "Molly")

target_link_libraries(MollyRT)
