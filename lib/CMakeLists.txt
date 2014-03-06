
#set(LLVM_NEXT_DEPENDS intrinsics_gen)

add_molly_library(LLVMMolly
  SOURCES
    # General
    RegisterPasses.cpp
  
    # LLVM Passes
    InlinePrepa.cpp
    MollyPassManager.cpp
    
    # Processing contexts
    MollyScopProcessor.cpp
    MollyFunctionProcessor.cpp
    MollyModuleProcessor.cpp
    MollyRegionProcessor.cpp
    MollyScopStmtProcessor.cpp
    
    # Classes representing something
    FieldType.cpp
    FieldVariable.cpp
    FieldLayout.cpp
    CommunicationBuffer.cpp
    ClusterConfig.cpp

    # Algorithms
    IslExprBuilder.cpp
    MollyIntrinsics.cpp
    
    # Available Layouts
    AffineMapping.cpp
    
    # Utilities,Helpers
    MollyFieldAccess.cpp
    ScopUtils.cpp
    MollyUtils.cpp
    ScopEditor.cpp
    Codegen.cpp
    
    # To be used in the future
    RectangularMapping.cpp
    LayoutMapping.cpp
    IndirectMapping.cpp
    ZCurveMapping.cpp
    HilbertCurveMapping.cpp
    FlattenMapping.cpp
    
  HEADER_DIRS 
    ${MOLLY_SOURCE_DIR}/include/molly 
    ${MOLLY_SOURCE_DIR}/lib
  LINK_COMPONENTS 
    support
    Polly
  LINK_LIBS 
    Islpp
  TARGET_DEPENDS 
    intrinsics_gen
  EXTERNAL_LIBS 
    #${GMP_LIBRARY} 
    #${ISL_LIBRARY} 
    #${CLOOG_LIBRARY} 
    #${BARVINOK_LIBRARY} 
    #${POLYLIB_LIBRARY} 
    #${LIBGCC_LIBRARY} 
    #${NTL_LIBRARY}
)

add_dependencies(LLVMMolly intrinsics_gen)

add_subdirectory(mollyrt)
add_subdirectory(islpp)
