
add_molly_executable(playground playground.cpp)

target_link_libraries( playground 
  LLVMMolly
  LLVMSupport
)
