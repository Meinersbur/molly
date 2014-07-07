
add_definitions(-DBENCH_MPI=1)
set(MOLLYRT_SOURCES mypapi.cpp molly.cpp bench.cpp comm.cpp combuf.cpp debug.cpp)


### MollyRT, compile using same compiler as we compile mollycc itself
# Note won't work if ABI is different
include_directories("${MOLLY_SOURCE_DIR}/include/mollyrt" ${MPI_CXX_INCLUDE_PATH})
add_library(MollyRT STATIC ${MOLLYRT_SOURCES} "${MOLLY_SOURCE_DIR}/include/mollyrt/molly.h")
set_target_properties(MollyRT PROPERTIES FOLDER "Molly")
target_link_libraries(MollyRT ${MPI_CXX_LIBRARIES}) 

foreach (_flag IN LISTS MOLLYRT_DEBUG_FLAGS)
  set_property(TARGET MollyRT APPEND_STRING PROPERTY COMPILE_FLAGS " ${_flag}")
endforeach ()



### MollyRT-ll
#mollycc_compile_link("${LLVM_BINARY_DIR}/lib/molly.ll" PLAIN
#  SOURCES ${MOLLYRT_SOURCES}
#  COMMENT "Generating molly.ll..."
#  LL)

#add_custom_target(MollyRT-ll ALL
#  DEPENDS ${LLVM_BINARY_DIR}/lib/molly.ll
#  COMMENT "Built Molly Runtime Library (ll)...")
#set_target_properties(MollyRT-ll PROPERTIES FOLDER "Molly")

#install(FILES "${LLVM_BINARY_DIR}/lib/molly.ll"
#  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
#  DESTINATION "lib${LLVM_LIBDIR_SUFFIX}")



### MollyRT-bc
#mollycc_compile_link("${LLVM_BINARY_DIR}/lib/molly.bc" PLAIN
#  SOURCES ${MOLLYRT_SOURCES}
#  COMMENT "Generating molly.bc..."
#  BC)

#add_custom_target(MollyRT-bc ALL
#  DEPENDS "${LLVM_BINARY_DIR}/lib/molly.bc"
#  COMMENT "Built Molly Runtime Library (bc)...")
#set_target_properties(MollyRT-bc PROPERTIES FOLDER "Molly")

#install(FILES "${LLVM_BINARY_DIR}/lib/molly.bc"
#  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
#  DESTINATION "lib${LLVM_LIBDIR_SUFFIX}")



### MollyRT-o
#mollycc_compile_link("${LLVM_BINARY_DIR}/lib/molly.o" PLAIN
#  SOURCES ${MOLLYRT_SOURCES}
#  COMMENT "Generating molly.o..."
#  OBJ)

#add_custom_target(MollyRT-o ALL
#  DEPENDS "${LLVM_BINARY_DIR}/lib/molly.o"
#  COMMENT "Built Molly Runtime Library (o)...")
#set_target_properties(MollyRT-o PROPERTIES FOLDER "Molly")

#install(FILES "${LLVM_BINARY_DIR}/lib/molly.o"
#  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
#  DESTINATION "lib${LLVM_LIBDIR_SUFFIX}")



### MollyRT-a
#set(MOLLYRT_COMPILER "clang" CACHE STRING "Compiler to be used to compile MollyRT")
clang_compile_link("${LLVM_BINARY_DIR}/lib/molly.a" AR
  COMPILER ${MOLLYCC_COMPILER}
  SOURCES ${MOLLYRT_SOURCES}
  FLAGS ${MOLLY_PLATFORM_FLAGS} ${MOLLYRT_COMMON_FLAGS} ${MOLLYRT_VERBOSE_FLAGS} ${MOLLYRT_DEBUG_FLAGS}
  CFLAGS ${MOLLY_PLATFORM_CFLAGS} ${MOLLYRT_COMMON_CFLAGS} ${MOLLYRT_VERBOSE_CFLAGS} ${MOLLYRT_DEBUG_CFLAGS}
  LDFLAGS ${MOLLY_PLATFORM_LDFLAGS} ${MOLLYRT_COMMON_LDFLAGS} ${MOLLYRT_VERBOSE_LDFLAGS} ${MOLLYRT_DEBUG_LDFLAGS}
)

add_custom_target(MollyRT-a ALL
  DEPENDS "${LLVM_BINARY_DIR}/lib/molly.a"
  COMMENT "Built Molly Runtime Library (a)...")
set_target_properties(MollyRT-a PROPERTIES FOLDER "Molly")

install(FILES "${LLVM_BINARY_DIR}/lib/molly.a"
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
  DESTINATION "lib${LLVM_LIBDIR_SUFFIX}")

