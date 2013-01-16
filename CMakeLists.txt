
set(MOLLY_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(MOLLY_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

# Add path for custom modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${MOLLY_SOURCE_DIR}/cmake" "${MOLLY_SOURCE_DIR}/../polly/cmake")


FIND_PACKAGE(Gmp REQUIRED)
INCLUDE_DIRECTORIES( ${GMP_INCLUDE_DIR} )

FIND_PACKAGE(Isl REQUIRED)
INCLUDE_DIRECTORIES( ${ISL_INCLUDE_DIR} )

FIND_PACKAGE(Cloog REQUIRED)
INCLUDE_DIRECTORIES( ${CLOOG_INCLUDE_DIR} )

#FIND_PACKAGE(Libgcc REQUIRED)

include_directories(
  ${MOLLY_SOURCE_DIR}/include
  ${MOLLY_SOURCE_DIR}/../clang/include  #TODO: Make Clang dir configurable
  ${MOLLY_BINARY_DIR}/../clang/include  #TODO: Make Clang dir configurable
  ${MOLLY_SOURCE_DIR}/../polly/include  #TODO: Make Polly dir configurable
  ${MOLLY_BINARY_DIR}/../polly/include  #TODO: Make Polly dir configurable
)

macro(add_molly_executable name)
  add_llvm_executable( ${name} ${ARGN} )
  set_target_properties(${name} PROPERTIES FOLDER "Molly")
endmacro(add_molly_executable)

macro(add_molly_library name)
  llvm_process_sources(srcs ${ARGN})
  if(MSVC_IDE OR XCODE)
    # Add public headers
    file( GLOB_RECURSE headers
      ${MOLLY_SOURCE_DIR}/include/molly/*.h
      ${MOLLY_SOURCE_DIR}/lib/*.h
    )
    set_source_files_properties(${headers} PROPERTIES HEADER_FILE_ONLY ON)
    set(srcs ${srcs} ${headers})
  endif(MSVC_IDE OR XCODE)
  
  add_llvm_library( ${name} ${srcs} )
  set_target_properties(${name} PROPERTIES FOLDER "Molly")
endmacro(add_molly_library)


add_molly_library(LLVMMolly
  Pass.cpp
  lib/RegisterPasses.cpp
  lib/FieldDetection.cpp
  lib/dummy.cpp
  lib/IslSet.cpp
  lib/Field.cpp
  lib/Nodegrid.cpp
  lib/DistributeField.cpp
)
  
target_link_libraries( LLVMMolly ${GMP_LIBRARY} )
target_link_libraries( LLVMMolly ${ISL_LIBRARY} )
target_link_libraries( LLVMMolly ${CLOOG_LIBRARY} )
target_link_libraries( LLVMMolly ${LIBGCC_LIBRARY} )


add_subdirectory(tools)
