
set(MOLLY_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(MOLLY_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")

set(POLLY_SOURCE_DIR "${MOLLY_SOURCE_DIR}/../polly" CACHE PATH "Path to Polly source") #TODO: Make Polly dir configurable or make molly a subproject
set(POLLY_BINARY_DIR "${LLVM_BINARY_DIR}/tools/polly") #TODO: Make Polly dir configurable

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
message("add_molly_library(${name} ${ARGN})")
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
    ${PARM_SOURCES} ${_headers} 
    ${PARM_DEFAULT_ARGS} 
    LINK_LIBS ${PARM_LINK_LIBS} 
    LINK_COMPONENTS ${PARM_LINK_COMPONENTS} 
#    TARGET_DEPENDS ${PARM_TARGET_DEPENDS} 
#   EXTERNAL_LIBS ${PARM_EXTERNAL_LIBS}
    )
    if (PARM_TARGET_DEPENDS)
      add_dependencies(${name} ${PARM_TARGET_DEPENDS})
    endif ()
  set_target_properties(${name} PROPERTIES FOLDER "Molly")
endmacro(add_molly_library)


macro(add_molly_executable name)
  add_llvm_executable(${name} ${ARGN})
  set_target_properties(${name} PROPERTIES FOLDER "Molly")
endmacro(add_molly_executable)





option(MOLLYRT_DEBUG "Compile Debug version of MollyRT" ON)
if (MOLLYRT_DEBUG)
message("MollyRT Debug build")
  set(MOLLYRT_DEBUG_FLAGS -O0 -g -DNTRACE)
else ()
message("MollyRT Release build")
  set(MOLLYRT_DEBUG_FLAGS -O3 -g -DNDEBUG -DNTRACE)
endif ()
set(MOLLYRT_DEBUG_CFLAGS )
set(MOLLYRT_DEBUG_LDFLAGS )


option(MOLLYCC_DEBUG "Compile examples with debug info" ON)
if (MOLLYCC_DEBUG)
message("Debug examples")
  set(MOLLYCC_DEBUG_FLAGS -O0 -g -DNTRACE)
else ()
message("Release examples")
  set(MOLLYCC_DEBUG_FLAGS -O3 -g -DNDEBUG -DNTRACE)
endif ()
set(MOLLYCC_DEBUG_CFLAGS )
set(MOLLYCC_DEBUG_LDFLAGS )

set(MOLLYCC_COMPILER "mollycc" CACHE STRING "The mollycc executable to compile examples")
option(MOLLYCC_ADD_MPI "Whether to add MPI paramters to mollycc command line" ON)



set(MOLLYCC_TARGET "${LLVM_DEFAULT_TARGET_TRIPLE}")

#set(MOLLY_PLATFORM_FLAGS -target "i686-pc-mingw32" -fno-exceptions)
set(MOLLY_PLATFORM_FLAGS -target "${MOLLYCC_TARGET}" -fno-exceptions -DBGQ_SPI)
set(MOLLY_PLATFORM_CFLAGS )
set(MOLLY_PLATFORM_LDFLAGS )

set(MOLLYRT_COMMON_FLAGS )
set(MOLLYRT_COMMON_CFLAGS -stdlib=libstdc++ -std=gnu++11 -D__mollycc__ "-I${MOLLY_SOURCE_DIR}/include/mollyrt" -Wno-error=gnu-array-member-paren-init -DPAPI=1)
set(MOLLYRT_COMMON_LDFLAGS )

set(MOLLYCC_COMMON_FLAGS )
set(MOLLYCC_COMMON_CFLAGS -stdlib=libstdc++ -std=gnu++11 -D__mollycc__ "-I${MOLLY_SOURCE_DIR}/include/mollyrt" -DWITH_MOLLY -Wno-error=gnu-array-member-paren-init -mllvm -molly -mllvm -polly-ignore-aliasing -mllvm -polly-dependences-computeout=0)
set(MOLLYCC_COMMON_LDLAGS )


foreach(_include IN LISTS MPI_CXX_INCLUDE_PATH)
if (MOLLYCC_ADD_MPI)
  list(APPEND MOLLYCC_COMMON_CFLAGS "-I${_include}")
endif ()
  list(APPEND MOLLYRT_COMMON_CFLAGS "-I${_include}")
endforeach()


set(MOLLYRT_VERBOSE_FLAGS -w -fmax-errors=1)
set(MOLLYRT_VERBOSE_CFLAGS )
set(MOLLYRT_VERBOSE_LDFLAGS )

#set(MOLLYCC_VERBOSE_FLAGS -save-temps -v -mllvm -debug -mllvm -debug-only=molly -mllvm -verify-region-info -mllvm -verify-dom-info -mllvm -verify-loop-info -mllvm -verify-scev -mllvm -molly-mark-stmts)
set(MOLLYCC_VERBOSE_FLAGS -v)
set(MOLLYCC_VERBOSE_CFLAGS -mllvm -polly-report)
set(MOLLYCC_VERBOSE_LDFLAGS )

option(MOLLYCC_DEBUG_OUTPUT "Debug output while compiling with mollycc" OFF)
if (MOLLYCC_DEBUG_OUTPUT)
  #list(APPEND MOLLYCC_VERBOSE_FLAGS -mllvm -debug -mllvm -debug-only=molly -fdiagnostics-show-category=name -Weverything)
  list(APPEND MOLLYCC_VERBOSE_FLAGS -mllvm -debug -mllvm -debug-only=molly)
endif ()

set(MOLLYCC_FLAGS "" CACHE STRING "Add flags when compiling Molly examples")
separate_arguments(MOLLYCC_FLAGS)




# Used to compile MollyRT
# Needs to be clang in order to match the ABI
function (clang_compile _outfile)
#message("   clang_compile(${_outfile}, ${ARGN})")
  cmake_parse_arguments("" "OBJ;LL;BC;ASM;II;EXE" "COMPILER" "COMMENT;SOURCES;FLAGS;CFLAGS;LDFLAGS;LIBS" ${ARGN})
  get_filename_component(_outfile "${_outfile}" ABSOLUTE)

  if (NOT _COMPILER)
    set(_COMPILER "clang")
    #set(_COMPILER "mollycc")
  endif ()
  
  if (NOT _COMMENT)
    set(_COMMENT "Compiling ${_outfile} using ${_COMPILER}...")
  endif ()
  
  # Make paths absolute
  set(_realsources)
  foreach (_rel IN LISTS _SOURCES)
    get_filename_component(_real "${_rel}" REALPATH)
    list(APPEND _realsources "${_real}")
  endforeach ()

  set(_libs)
  if (_OBJ)
    set(_kindflags "-c")
    set(_flags ${_FLAGS} ${_CFLAGS})
  elseif (_LL)
    set(_kindflags "-S -emit-llvm")
    set(_flags ${_FLAGS} ${_CFLAGS})
  elseif (_BC)
    set(_kindflags "-c -emit-llvm")
    set(_flags ${_FLAGS} ${_CFLAGS})
  elseif (_ASM)
    set(_kindflags "-S")
    set(_flags ${_FLAGS} ${_CFLAGS})
  elseif (_II)
    set(_kindflags "-E")
    set(_flags ${_FLAGS} ${_CFLAGS})
  else () # EXE
    set(_kindflags)
    set(_flags ${_FLAGS} ${_CFLAGS} ${_LDFLAGS} ${_LIBS})
    set(_libs ${_LIBS})
  endif ()
  
#message("add_custom_command(${_outfile}, ${_COMPILER} ${MOLLYCC_VERBOSE_FLAGS} -o ${_outfile} ${_kindflags} ${_realsources} ${_FLAGS}  ${_CFLAGS} ${_LDFLAGS} ${_LIBS})")
  add_custom_command(
    OUTPUT "${_outfile}"
    COMMAND "${_COMPILER}" -o "${_outfile}" ${_kindflags} ${_realsources} ${_flags} ${_libs}
    DEPENDS "${_COMPILER}" ${_realsources} ${_libs}
    IMPLICIT_DEPENDS ${_realsources}
    COMMENT "${_COMMENT}"
  )
endfunction ()


function (clang_compile_link _outfile)
#message("   clang_compile_link(${_outfile}, ${ARGN})")
  cmake_parse_arguments("" "OBJ;LL;BC;AR;EXE" "COMPILER" "FLAGS;CFLAGS;LDFLAGS;SOURCES;COMMENT;LIBS;DEPENDS" ${ARGN})
  get_filename_component(_outfile "${_outfile}" ABSOLUTE)
  
  if (NOT _COMPILER)
    set(_COMPILER "clang")
    #set(_COMPILER "mollycc")
  endif ()

  if (NOT _COMMENT)
    set(_COMMENT "Linking ${_outfile} using ${_COMPILER}...")
  endif ()

  if (_OBJ)
    set(_mode OBJ)
    set(_suffix ".o")
  elseif (_LL)
    set(_mode LL)
    set(_suffix ".ll")
  elseif (_BC)
    set(_mode BC)
    set(_suffix ".bc")
  elseif (_AR)
    set(_mode AR)
    set(_suffix ".a")
  else () # EXE;default
    set(_EXE True)
    set(_mode EXE)
    set(_suffix "${CMAKE_EXECUTABLE_SUFFIX}")
  endif ()
#message("${_AR} ${_suffix} ${_mode}")

  # Make paths absolute
  set(_realsources)
  foreach (_rel IN LISTS _SOURCES)
    get_filename_component(_real "${_rel}" REALPATH)
    list(APPEND _realsources "${_real}")
  endforeach ()
  
  
  list(LENGTH _SOURCES _sourceslen)
  if (_AR)
  
    # .a need to compiled separately and then merged into an archive
    # TODO: Visul C++ .lib files use LINK command
    set(_intermediates)
    foreach(_src IN LISTS _realsources)  
      get_filename_component(_base "${_src}" NAME)
      set(_intermediatefilename "${CMAKE_CURRENT_BINARY_DIR}/${_base}.o")
      clang_compile("${_intermediatefilename}" OBJ
        SOURCES "${_src}"
        COMPILER "${_COMPILER}"
        FLAGS ${_FLAGS} 
        CFLAGS ${_CFLAGS}
      )
      list(APPEND _intermediates "${_intermediatefilename}")
    endforeach ()
    
    add_custom_command(
      OUTPUT "${_outfile}"
#      COMMAND "${CMAKE_COMMAND}" remove "${_outfile}"
      COMMAND "${CMAKE_AR}" rsc "${_outfile}" ${_intermediates} 
      DEPENDS ${_DEPENDS} ${_intermediates}
      COMMENT "${_COMMENT}"
      VERBATIM
    )

  elseif (_sourceslen EQUAL 1 OR _EXE)
  
    # Single file can just be compiled with one command
    # EXE can also be directly linked by clang driver
    clang_compile("${_outfile}" "${_mode}"
      SOURCES ${_SOURCES}
      COMPILER "${_COMPILER}"
      FLAGS ${_FLAGS} 
      CFLAGS ${_CFLAGS}
      LDFLAGS ${_LDFLAGS}
      LIBS ${_LIBS}
    )
    
  else ()
    
    set(_intermediates)
    foreach(_src IN LISTS _realsources)  
      get_filename_component(_base "${_src}" NAME)
      set(_intermediatefilename "${CMAKE_CURRENT_BINARY_DIR}/${_base}.bc")
      clang_compile("${_intermediatefilename}" BC
        SOURCES ${_SOURCES}
        COMPILER "${_COMPILER}"
        FLAGS ${_FLAGS} 
        CFLAGS ${_CFLAGS}
      )
      list(APPEND _intermediates "${_intermediatefilename}")
    endforeach ()

    if (_BC)
      add_custom_command(
        OUTPUT "${_outfile}"
        COMMAND llvm-link ${_intermediates} -o "${_outfile}"
        DEPENDS llvm-link ${_intermediates}
        COMMENT "${_COMMENT}"
        VERBATIM
      )
    elseif (_LL)
      add_custom_command(
        OUTPUT "${_outfile}"
        COMMAND llvm-link -S ${_intermediates} -o "${_outfile}"
        DEPENDS llvm-link ${_intermediates}
        COMMENT "${_COMMENT}"
        VERBATIM
       )
    else ()
      add_custom_command(
        OUTPUT "${_outfile}"
        COMMAND llvm-link -S ${_intermediates} -o "${_outfile}.bc"
        DEPENDS llvm-link ${_intermediates}
        VERBATIM
      )

      clang_compile("${_outfile}" "${_mode}"
        COMPILER "${_COMPILER}"
        SOURCES "${_outfile}.bc"
        FLAGS ${_FLAGS} 
        CFLAGS ${_CFLAGS}
        LDFLAGS ${_LDFLAGS}
        LIBS ${_LIBS}
        COMMENT "${_COMMENT}"
      )
    endif ()
  endif ()
endfunction ()





























macro(mollycc_compile2 _outfile)
message("   mollycc_compile(${_outfile}, ${ARG})")
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
    OUTPUT "${_outfile}"
    COMMAND mollycc ${_flags} ${_realsources} -o "${_outfile}" ${PARM_LIBS}
    DEPENDS mollycc ${_realsources} ${PARM_LIBS}
    IMPLICIT_DEPENDS ${_realsources}
    COMMENT "${PARM_COMMENT}")
endmacro()


macro(mollycc_compile_link _outfile)
#message("mollycc_compile_link(${_outfile}, ${ARG})")
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
      mollycc_compile("${_outfile}" ${_plain}
        SOURCES ${PARM_SOURCES}
        FLAGS -emit-llvm -c ${PARM_FLAGS}
        COMMENT ${PARM_COMMENT})
    elseif (PARM_LL)
      mollycc_compile("${_outfile}" ${_plain}
        SOURCES ${PARM_SOURCES}
        FLAGS -emit-llvm -S ${PARM_FLAGS}
        COMMENT ${PARM_COMMENT})
    elseif (PARM_OBJ)
      mollycc_compile("${_outfile}" ${_plain}
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
      get_filename_component(_base "${_src}" NAME)
      if (PARM_LL)
        set(_intermediatefilename "${CMAKE_CURRENT_BINARY_DIR}/${_base}.ll")
        mollycc_compile(${_intermediatefilename} ${_plain}
          SOURCES ${_src}
          FLAGS -emit-llvm -S ${PARM_FLAGS}
          )
      elseif (PARM_BC OR PARM_OBJ)
        set(_intermediatefilename "${CMAKE_CURRENT_BINARY_DIR}/${_base}.bc")
        mollycc_compile(${_intermediatefilename} ${_plain}
          SOURCES ${_src}
          FLAGS -emit-llvm -c ${PARM_FLAGS}
          )
      elseif (PARM_AR)
        set(_intermediatefilename "${CMAKE_CURRENT_BINARY_DIR}/${_base}.o")
        mollycc_compile("${_intermediatefilename}" ${_plain}
          SOURCES ${_src}
          FLAGS -c ${PARM_FLAGS}
          )
      else ()
        message(FATAL_ERROR "Select one linker output")
      endif ()
      list(APPEND _intermediates "${_intermediatefilename}")
    endforeach ()
  
    if (PARM_BC)
      add_custom_command(
        OUTPUT ${_outfile}
        COMMAND llvm-link ${_intermediates} -o "${_outfile}"
        DEPENDS llvm-link ${_intermediates}
        COMMENT ${PARM_COMMENT}
        VERBATIM)
    elseif (PARM_LL)
      add_custom_command(
        OUTPUT ${_outfile}
        COMMAND llvm-link -S ${_intermediates} -o "${_outfile}"
        DEPENDS llvm-link ${_intermediates}
        COMMENT ${PARM_COMMENT}
        VERBATIM)
    elseif (PARM_OBJ)
      add_custom_command(
        OUTPUT "${_outfile}.bc"
        COMMAND llvm-link -S ${_intermediates} -o "${_outfile}.bc"
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
        COMMAND "${CMAKE_AR}" as "${_outfile}" ${_intermediates} 
        DEPENDS ${_intermediates}
        COMMENT "${PARM_COMMENT}"
        VERBATIM)
    else ()
        message(FATAL_ERROR "Select one linker output")
    endif ()
  endif (_sourceslen EQUAL 1 AND NOT PARM_AR)
endmacro()




set(MOLLYCC_LL_LIBS "${LLVM_BINARY_DIR}/lib/molly.ll" ${MPI_CXX_LIBRARIES})
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



set(MOLLYCC_O_LIBS "${LLVM_BINARY_DIR}/lib/molly.o" ${MPI_CXX_LIBRARIES})
macro(mollycc_o_compile _outfile)
  parse_arguments(PARM "FLAGS;SOURCES;COMMENT" "" ${ARGN})
  mollycc_compile(${_outfile}
    SOURCES ${PARM_SOURCES}
    FLAGS ${PARM_FLAGS}
    LIBS ${MOLLYCC_O_LIBS}
    COMMENT ${PARM_COMMENT}
    )
endmacro()


# Compile and link to MollyRT that CMake is responsible to compile
function (mollycc_compile _outfile)
#message("   mollycc_compile(${_outfile}, ${ARGN})")
  cmake_parse_arguments("" "" "" "SOURCES;FLAGS;CFLAGS;LDFLAGS;COMMENT;LIBS" ${ARGN})
  get_property(_MollyRT_location TARGET MollyRT PROPERTY LOCATION)
  clang_compile_link("${_outfile}" EXE
    COMPILER mollycc
    SOURCES ${_SOURCES}
    FLAGS ${MOLLY_PLATFORM_FLAGS} ${MOLLYCC_COMMON_FLAGS} ${MOLLYCC_VERBOSE_FLAGS} ${MOLLYCC_DEBUG_FLAGS} ${_FLAGS} 
    CFLAGS ${MOLLY_PLATFORM_CFLAGS} ${MOLLYCC_COMMON_CFLAGS} ${MOLLYCC_VERBOSE_CFLAGS} ${MOLLYCC_DEBUG_CFLAGS} ${_CFLAGS}
    LDFLAGS ${MOLLY_PLATFORM_LDFLAGS} ${MOLLYCC_COMMON_LDFLAGS} ${MOLLYCC_VERBOSE_LDFLAGS} ${MOLLYCC_DEBUG_LDFLAGS} ${_LDFLAGS}
#    LIBS $<TARGET_FILE:MollyRT> ${MPI_CXX_LIBRARIES} ${_LIBS}
    LIBS "${_MollyRT_location}" ${MPI_CXX_LIBRARIES} ${_LIBS}
    COMMENT "${_COMMENT}"
    )
endfunction ()



# Compile and link to molly.a
set(MOLLYCC_A_LIBS "${LLVM_BINARY_DIR}/lib/molly.a" "/bgsys/drivers/ppcfloor/spi/lib/libSPI_l1p.a" "/bgsys/drivers/ppcfloor/bgpm/lib/libbgpm.a")
if (MOLLYCC_ADD_MPI)
  set(MOLLYCC_A_LIBS ${MOLLYCC_A_LIBS} ${MPI_CXX_LIBRARIES})
endif ()
function (mollycc_a_compile _outfile)
#message("   mollycc_a_compile(${_outfile}, ${ARGN})")
  cmake_parse_arguments("" "" "" "SOURCES;FLAGS;CFLAGS;LDFLAGS;COMMENT;LIBS" ${ARGN})
  clang_compile_link("${_outfile}" EXE
    COMPILER ${MOLLYCC_COMPILER}
    DEPENDS mollycc
    SOURCES ${_SOURCES}
    FLAGS ${MOLLYCC_FLAGS} ${MOLLY_PLATFORM_FLAGS} ${MOLLYCC_COMMON_FLAGS} ${MOLLYCC_VERBOSE_FLAGS} ${MOLLYCC_DEBUG_FLAGS} ${_FLAGS} 
    CFLAGS ${MOLLYCC_CFLAGS} ${MOLLY_PLATFORM_CFLAGS} ${MOLLYCC_COMMON_CFLAGS} ${MOLLYCC_VERBOSE_CFLAGS} ${MOLLYCC_DEBUG_CFLAGS} ${_CFLAGS}
    LDFLAGS ${MOLLYCC_LDFLAGS} ${MOLLY_PLATFORM_LDFLAGS} ${MOLLYCC_COMMON_LDFLAGS} ${MOLLYCC_VERBOSE_LDFLAGS} ${MOLLYCC_DEBUG_LDFLAGS} ${_LDFLAGS}
    LIBS ${MOLLYCC_A_LIBS} ${_LIBS}
    COMMENT "${_COMMENT}"
    )
endfunction ()



add_subdirectory(lib)
add_subdirectory(tools)
add_subdirectory(examples)
