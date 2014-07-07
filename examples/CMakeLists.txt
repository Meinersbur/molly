
include_directories("${MOLLY_SOURCE_DIR}/include/mollyrt")

function(add_molly_example _name)
  add_llvm_example(${_name} ${ARGN})
  set_target_properties(${_name} PROPERTIES FOLDER "Molly")
  target_link_libraries(${_name} MollyRT ${MPI_CXX_LIBRARIES}) # For benchmarking stuff
endfunction()




function (add_example_config src var_exepath var_target)
  cmake_parse_arguments("ARG" "" "VOLUME;PROC;BLOCK" "DEFINES" ${ARGN})
  get_filename_component(src_path "${src}" ABSOLUTE)

  set(_defsuffix)
  foreach (_define IN LISTS ARG_DEFINES)
    #string(REPLACE "=" "" _define "${_define}")
    set(_defsuffix "${_defsuffix}_${_define}" )
  endforeach ()
  
  get_filename_component(src_we "${src}" NAME_WE)
  set(target_name ${src_we}_L${ARG_VOLUME}_P${ARG_PROC}_B${ARG_BLOCK}${_defsuffix}-mollycc-a)
  get_filename_component(dst_path "${LLVM_EXAMPLES_BINARY_DIR}/${CMAKE_EXECUTABLE_PREFIX}${target_name}${CMAKE_EXECUTABLE_SUFFIX}" ABSOLUTE)
  string(REPLACE "x" ";" volume_list ${ARG_VOLUME})
  string(REPLACE "x" ";" proc_list ${ARG_PROC})
  string(REPLACE "x" ";" block_list ${ARG_BLOCK})

  list(LENGTH proc_list procdims)

  set(i 0)
  set(defines )
  foreach (_dimname IN LISTS dimnames)
    list(GET volume_list ${i} _l)
    list(APPEND defines "-DL${_dimname}=${_l}")
    list(GET block_list ${i} _p)
    list(APPEND defines "-DB${_dimname}=${_p}")

    if (i LESS ${procdims})
      list(GET proc_list ${i} _p)
    else ()
      set (_p 1)
    endif ()
    list(APPEND defines "-DP${_dimname}=${_p}")

    math(EXPR i "${i}+1")
  endforeach ()
  foreach (_define IN LISTS ARG_DEFINES)
     list(APPEND defines "-D${_define}")
  endforeach ()


  mollycc_a_compile("${dst_path}"
    SOURCES ${src}
    FLAGS -mllvm "-shape=${ARG_PROC}" -mllvm -polly-only-marked ${defines}
  )

add_custom_target(${target_name}
  DEPENDS "${dst_path}"
  VERBATIM
)
add_dependencies(${target-all} "${target_name}")

  set("${var_exepath}" "${dst_path}" PARENT_SCOPE)
  set("${var_target}" "${target_name}" PARENT_SCOPE)
endfunction ()


add_subdirectory(islpp)
add_subdirectory(stencil)
add_subdirectory(lqcd)
add_subdirectory(gameoflife)
add_subdirectory(jacobi)
add_subdirectory(gemm)
add_subdirectory(flow)
add_subdirectory(lqcd2d)
add_subdirectory(bench)
