
set(MOLLY_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(MOLLY_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

set(POLLY_SOURCE_DIR ${MOLLY_SOURCE_DIR}/../polly CACHE PATH "Path to Polly source") #TODO: Make Polly dir configurable or make molly a subproject
set(POLLY_BINARY_DIR ${LLVM_BINARY_DIR}/tools/polly) #TODO: Make Polly dir configurable

# Add path for custom modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${MOLLY_SOURCE_DIR}/cmake" "${POLLY_SOURCE_DIR}/cmake")


find_package(Gmp REQUIRED)
INCLUDE_DIRECTORIES( ${GMP_INCLUDE_DIR} )

add_llvm_external_project(isl)
if (NOT LLVM_EXTERNAL_ISL_BUILD)
  find_package(Isl REQUIRED)
endif ()
INCLUDE_DIRECTORIES( ${ISL_INCLUDE_DIRS} )

#FIND_PACKAGE(Cloog REQUIRED)
#INCLUDE_DIRECTORIES( ${CLOOG_INCLUDE_DIR} )

#FIND_PACKAGE(Barvinok REQUIRED)
#INCLUDE_DIRECTORIES( ${BARVINOK_INCLUDE_DIR} )

#FIND_PACKAGE(Polylib REQUIRED)
#INCLUDE_DIRECTORIES( ${POLYLIB_INCLUDE_DIR} )

#FIND_PACKAGE(Ntl REQUIRED)
#INCLUDE_DIRECTORIES( ${NTL_INCLUDE_DIR} )

#FIND_PACKAGE(Libgcc REQUIRED)

find_package(MPI REQUIRED)


include_directories(
  ${MOLLY_SOURCE_DIR}/include
  ${MOLLY_SOURCE_DIR}/../clang/include  #TODO: Make Clang dir configurable (or required at all?)
  ${MOLLY_BINARY_DIR}/../clang/include  #TODO: Make Clang dir configurable
  ${POLLY_SOURCE_DIR}/include  
  ${POLLY_BINARY_DIR}/include 
)

if (CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wno-unknown-pragmas")
endif (CMAKE_COMPILER_IS_GNUCXX)


macro(add_molly_library name)
  parse_arguments(PARM "LINK_LIBS;LINK_COMPONENTS;TARGET_DEPENDS;EXTERNAL_LIBS;SOURCES;HEADERS;HEADER_DIRS" "" ${ARGN})
  
  set(_headers)
  if (MSVC_IDE OR XCODE)
    set(_headers ${PARM_HEADERS})
    foreach (_dir IN LISTS PARM_HEADER_DIRS)
      file(GLOB _dir_headers "${_dir}/*.h")
      list(APPEND _headers ${_dir_headers})
    endforeach ()
    set_source_files_properties(${_headers} PROPERTIES HEADER_FILE_ONLY ON)
  endif ()
  
  add_llvm_library(${name} 
    ${PARM_DEFAULT_ARGS} 
    SOURCES ${PARM_SOURCES} ${_headers} 
    LINK_LIBS ${PARM_LINK_LIBS} 
    LINK_COMPONENTS ${PARM_LINK_COMPONENTS} 
    TARGET_DEPENDS ${PARM_TARGET_DEPENDS} 
    EXTERNAL_LIBS ${PARM_EXTERNAL_LIBS}
    )
  set_target_properties(${name} PROPERTIES FOLDER "Molly")
endmacro(add_molly_library)


macro(add_molly_executable name)
  add_llvm_executable(${name} ${ARGN})
  set_target_properties(${name} PROPERTIES FOLDER "Molly")
endmacro(add_molly_executable)



macro(clang_compile _outfile)
  parse_arguments(PARM "FLAGS;DEPS;SOURCES;COMMENT" "" ${ARGN})
  if (NOT PARM_COMMENT)
    set(PARM_COMMENT "Generating ${_outfile} using Clang...")
  endif ()
  add_custom_command(
    OUTPUT ${_outfile}
    COMMAND clang ${PARM_FLAGS} ${PARM_SOURCES} -o ${_outfile} 
    DEPENDS clang ${PARM_SOURCES}
    COMMENT "${PARM_COMMENT}")
endmacro()


option(MOLLYCC_DEBUG "Compile Debug version of MollyRT (and anything compiled with mollycc)" ON)
#mark_as_advanced(MOLLYCC_DEBUG)

if (MOLLYCC_DEBUG)
message("MollyRT Debug build")
  set(MOLLYCC_DEBUG_FLAGS -O0 -g -DDEBUG)
else ()
message("MollyRT Release build")
  set(MOLLYCC_DEBUG_FLAGS -O3 -g0 -DNDEBUG)
endif ()

set(MOLLYCC_COMMON_FLAGS -std=c++11 -fms-extensions -fno-exceptions -D__mollycc__ -I${MOLLY_SOURCE_DIR}/include/mollyrt)
foreach(_include IN LISTS MPI_CXX_INCLUDE_PATH)
  list(APPEND MOLLYCC_COMMON_FLAGS "-I${_include}")
endforeach()
#set(MOLLYCC_VERBOSE_FLAGS -save-temps -v -w -mllvm -debug -mllvm -debug-only=molly -mllvm -polly-report)
set(MOLLYCC_VERBOSE_FLAGS -v -w -mllvm -debug -mllvm -debug-only=molly -mllvm -polly-report)
#set(MOLLYCC_VERBOSE_FLAGS)
set(MOLLYCC_PLATFORM_FLAGS -target "i686-pc-mingw32")
#set(MOLLYCC_PLATFORM_FLAGS -target "i686-pc-win32" -fno-exceptions)

macro(mollycc_compile _outfile)
  parse_arguments(PARM "FLAGS;SOURCES;COMMENT;LIBS" "PLAIN" ${ARGN})
  if (NOT PARM_COMMENT)
    set(PARM_COMMENT "Generating ${_outfile} using Mollycc...")
  endif ()
  # Make paths absolute
  set(_realsources)
  foreach(_rel IN LISTS PARM_SOURCES)
    get_filename_component(_real ${_rel} REALPATH)
    list(APPEND _realsources ${_real})
  endforeach()
  if (PARM_PLAIN)
    # Do not activate Molly transformations
    set(_flags ${MOLLYCC_VERBOSE_FLAGS} ${MOLLYCC_COMMON_FLAGS} ${MOLLYCC_DEBUG_FLAGS} ${PARM_FLAGS} )
  else ()
    set(_flags ${MOLLYCC_VERBOSE_FLAGS} ${MOLLYCC_COMMON_FLAGS} ${MOLLYCC_DEBUG_FLAGS} ${PARM_FLAGS} -mllvm -molly)
  endif ()
  add_custom_command(
    OUTPUT ${_outfile}
    COMMAND mollycc ${_flags} ${_realsources} -o ${_outfile} ${PARM_LIBS}
    DEPENDS mollycc ${_realsources} ${PARM_LIBS}
    IMPLICIT_DEPENDS ${_realsources}
    COMMENT "${PARM_COMMENT}")
endmacro()


macro(mollycc_compile_link _outfile)
  parse_arguments(PARM "FLAGS;SOURCES;COMMENT" "BC;LL;OBJ;AR;PLAIN" ${ARGN})
  get_filename_component(_outfile ${_outfile} ABSOLUTE)
  
  if (PARM_PLAIN)
    set(_plain PLAIN)
  else ()
    set(_plain)
  endif ()
  
  list(LENGTH PARM_SOURCES _sourceslen)
  if (_sourceslen EQUAL 1 AND NOT PARM_AR)
    if (PARM_BC)
      mollycc_compile(${_outfile} ${_plain}
        SOURCES ${PARM_SOURCES}
        FLAGS -emit-llvm -c ${PARM_FLAGS}
        COMMENT ${PARM_COMMENT})
    elseif (PARM_LL)
      mollycc_compile(${_outfile} ${_plain}
        SOURCES ${PARM_SOURCES}
        FLAGS -emit-llvm -S ${PARM_FLAGS}
        COMMENT ${PARM_COMMENT})
    elseif (PARM_OBJ)
      mollycc_compile(${_outfile} ${_plain}
        SOURCES ${PARM_SOURCES}
        FLAGS -c ${PARM_FLAGS}
        COMMENT ${PARM_COMMENT})
    else ()
      message(FATAL_ERROR "Select one linker output")
    endif ()
  else (_sourceslen EQUAL 1 AND NOT PARM_AR)
    if (NOT PARM_COMMENT)
      set(PARM_COMMENT "Linking ${_outfile}...")
    endif ()
  
    # Make paths absolute
    set(_realsources)
    foreach (_rel IN LISTS PARM_SOURCES)
      get_filename_component(_real ${_rel} REALPATH)
      list(APPEND _realsources ${_real})
    endforeach ()
  
    set(_intermediates)
    foreach(_src IN LISTS _realsources)
      get_filename_component(_base ${_src} NAME)
      if (PARM_LL)
        set(_intermediatefilename ${CMAKE_CURRENT_BINARY_DIR}/${_base}.ll)
        mollycc_compile(${_intermediatefilename} ${_plain}
          SOURCES ${_src}
          FLAGS -emit-llvm -S ${PARM_FLAGS}
          )
      elseif (PARM_BC OR PARM_OBJ)
        set(_intermediatefilename ${CMAKE_CURRENT_BINARY_DIR}/${_base}.bc)
        mollycc_compile(${_intermediatefilename} ${_plain}
          SOURCES ${_src}
          FLAGS -emit-llvm -c ${PARM_FLAGS}
          )
      elseif (PARM_AR)
        set(_intermediatefilename ${CMAKE_CURRENT_BINARY_DIR}/${_base}.o)
        mollycc_compile(${_intermediatefilename} ${_plain}
          SOURCES ${_src}
          FLAGS -c ${PARM_FLAGS}
          )
      else ()
        message(FATAL_ERROR "Select one linker output")
      endif ()
      list(APPEND _intermediates ${_intermediatefilename})
    endforeach ()
  
    if (PARM_BC)
      add_custom_command(
        OUTPUT ${_outfile}
        COMMAND llvm-link ${_intermediates} -o ${_outfile}
        DEPENDS llvm-link ${_intermediates}
        COMMENT ${PARM_COMMENT}
        VERBATIM)
    elseif (PARM_LL)
      add_custom_command(
        OUTPUT ${_outfile}
        COMMAND llvm-link -S ${_intermediates} -o ${_outfile}
        DEPENDS llvm-link ${_intermediates}
        COMMENT ${PARM_COMMENT}
        VERBATIM)
    elseif (PARM_OBJ)
      add_custom_command(
        OUTPUT ${_outfile}.bc
        COMMAND llvm-link -S ${_intermediates} -o ${_outfile}.bc
        DEPENDS llvm-link ${_intermediates}
        COMMENT ${PARM_COMMENT}
        VERBATIM)
    
      mollycc_compile(${_outfile} ${_plain}
        SOURCES ${_outfile}.bc
        FLAGS -c
        COMMENT ${PARM_COMMENT})
    elseif (PARM_AR)
      add_custom_command(
        OUTPUT ${_outfile}
        COMMAND ${CMAKE_AR} as ${_outfile} ${_intermediates} 
        DEPENDS ${_intermediates}
        COMMENT ${PARM_COMMENT}
        VERBATIM)
    else ()
        message(FATAL_ERROR "Select one linker output")
    endif ()
  endif (_sourceslen EQUAL 1 AND NOT PARM_AR)
endmacro()




set(MOLLYCC_LL_LIBS ${LLVM_BINARY_DIR}/lib/molly.ll ${MPI_CXX_LIBRARIES})
macro(mollycc_ll_compile _outfile)
  parse_arguments(PARM "FLAGS;SOURCES;COMMENT;LIBS" "" ${ARGN})
  mollycc_compile(${_outfile}
    SOURCES ${PARM_SOURCES}
    FLAGS ${PARM_FLAGS}
    LIBS ${MOLLYCC_LL_LIBS}
    COMMENT ${PARM_COMMENT}
    )
endmacro()



set(MOLLYCC_BC_LIBS ${LLVM_BINARY_DIR}/lib/molly.bc ${MPI_CXX_LIBRARIES})
macro(mollycc_bc_compile _outfile)
  parse_arguments(PARM "FLAGS;SOURCES;COMMENT" "" ${ARGN})
  mollycc_compile(${_outfile}
    SOURCES ${PARM_SOURCES}
    FLAGS ${PARM_FLAGS}
    LIBS ${MOLLYCC_BC_LIBS}
    COMMENT ${PARM_COMMENT}
    )
endmacro()



set(MOLLYCC_O_LIBS ${LLVM_BINARY_DIR}/lib/molly.o ${MPI_CXX_LIBRARIES})
macro(mollycc_o_compile _outfile)
  parse_arguments(PARM "FLAGS;SOURCES;COMMENT" "" ${ARGN})
  mollycc_compile(${_outfile}
    SOURCES ${PARM_SOURCES}
    FLAGS ${PARM_FLAGS}
    LIBS ${MOLLYCC_O_LIBS}
    COMMENT ${PARM_COMMENT}
    )
endmacro()



set(MOLLYCC_A_LIBS ${LLVM_BINARY_DIR}/lib/molly.a ${MPI_CXX_LIBRARIES})
macro(mollycc_a_compile _outfile)
  parse_arguments(PARM "FLAGS;SOURCES;COMMENT" "" ${ARGN})
  mollycc_compile(${_outfile}
    SOURCES ${PARM_SOURCES}
    FLAGS ${PARM_FLAGS}
    LIBS ${MOLLYCC_A_LIBS}
    COMMENT ${PARM_COMMENT}
    )
endmacro()



add_subdirectory(lib)
add_subdirectory(tools)
add_subdirectory(examples)
