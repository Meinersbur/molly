
add_molly_executable(playground playground.cpp
  LINK_LIBS LLVMSupport Islpp ${ISL_LIBRARY} ${GMP_LIBRARY}
)

target_link_libraries( playground ${ISL_LIBRARY} )
target_link_libraries( playground ${GMP_LIBRARY} )
