
set(MOLLYRT_SOURCES molly.cpp comm.cpp debug.cpp combuf.cpp)

### MollyRT, compile using standard compiler
include_directories(${MOLLY_SOURCE_DIR}/include/mollyrt ${MPI_CXX_INCLUDE_PATH})
add_library(MollyRT STATIC ${MOLLYRT_SOURCES} ${MOLLY_SOURCE_DIR}/include/mollyrt/molly.h)
set_target_properties(MollyRT PROPERTIES FOLDER "Molly")
target_link_libraries(MollyRT ${MPI_CXX_LIBRARIES}) 



### MollyRT-ll
mollycc_compile_link(${LLVM_BINARY_DIR}/lib/molly.ll PLAIN
  SOURCES ${MOLLYRT_SOURCES}
  COMMENT "Generating molly.ll..."
  LL)

add_custom_target(MollyRT-ll ALL
  DEPENDS ${LLVM_BINARY_DIR}/lib/molly.ll
  COMMENT "Built Molly Runtime Library (ll)...")
set_target_properties(MollyRT-ll PROPERTIES FOLDER "Molly")

install(FILES ${LLVM_BINARY_DIR}/lib/molly.ll
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
  DESTINATION lib${LLVM_LIBDIR_SUFFIX})



### MollyRT-bc
mollycc_compile_link(${LLVM_BINARY_DIR}/lib/molly.bc PLAIN
  SOURCES ${MOLLYRT_SOURCES}
  COMMENT "Generating molly.bc..."
  BC)

add_custom_target(MollyRT-bc ALL
  DEPENDS ${LLVM_BINARY_DIR}/lib/molly.bc
  COMMENT "Built Molly Runtime Library (bc)...")
set_target_properties(MollyRT-bc PROPERTIES FOLDER "Molly")

install(FILES ${LLVM_BINARY_DIR}/lib/molly.bc
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
  DESTINATION lib${LLVM_LIBDIR_SUFFIX})



### MollyRT-o
mollycc_compile_link(${LLVM_BINARY_DIR}/lib/molly.o PLAIN
  SOURCES ${MOLLYRT_SOURCES}
  COMMENT "Generating molly.o..."
  OBJ)

add_custom_target(MollyRT-o ALL
  DEPENDS ${LLVM_BINARY_DIR}/lib/molly.o
  COMMENT "Built Molly Runtime Library (o)...")
set_target_properties(MollyRT-o PROPERTIES FOLDER "Molly")

install(FILES ${LLVM_BINARY_DIR}/lib/molly.o
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
  DESTINATION lib${LLVM_LIBDIR_SUFFIX})



### MollyRT-a
mollycc_compile(${LLVM_BINARY_DIR}/lib/molly.a PLAIN
  SOURCES ${MOLLYRT_SOURCES}
  COMMENT "Generating molly.a...")

add_custom_target(MollyRT-a ALL
  DEPENDS ${LLVM_BINARY_DIR}/lib/molly.a
  COMMENT "Built Molly Runtime Library (a)...")
set_target_properties(MollyRT-a PROPERTIES FOLDER "Molly")

install(FILES ${LLVM_BINARY_DIR}/lib/molly.a
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
  DESTINATION lib${LLVM_LIBDIR_SUFFIX})
