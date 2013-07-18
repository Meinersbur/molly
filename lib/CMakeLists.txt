
add_molly_library(LLVMMolly
  SOURCES
    RegisterPasses.cpp
    FieldDetection.cpp
    FieldDistribution.cpp
    FieldType.cpp
    FieldVariable.cpp
    FieldCodeGen.cpp
    ScopDistribution.cpp
    MollyContextPass.cpp
    ScopUtils.cpp
    ScopFieldCodeGen.cpp
    InsertInOut.cpp
    ClusterConfig.cpp
    MollyFieldAccess.cpp
    IslExprBuilder.cpp
    
    MollyContext.cpp
    Nodegrid.cpp  # Obsolete?
    ValueSourceAnalysis.cpp
    ScopStmtSplit.cpp
    MollyUtils.cpp
    MollyInlinePrepa.cpp # Obsolete?
    PatternSearchAnalysis.cpp # Obsolete
    GlobalPassManager.cpp
    MollyPassManager.cpp # Obsolete
    SCEVAffinator.cpp
    UnusedValueElimination.cpp
    CommunicationBuffer.cpp
    ScopEditor.cpp
    FieldLayout.cpp
    LayoutMapping.cpp
    AffineMapping.cpp
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

add_subdirectory(mollyrt)
add_subdirectory(islpp)
