
add_molly_executable(playground playground.cpp)

set(LLVM_LINK_COMPONENTS ${LLVM_TARGETS_TO_BUILD} LLVMMolly)

#target_link_libraries( molly 
#  LLVMMolly
#)
