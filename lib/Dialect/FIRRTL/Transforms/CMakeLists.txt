add_circt_dialect_library(CIRCTFIRRTLTransforms
  BlackBoxMemory.cpp
  BlackBoxReader.cpp
  CheckWidths.cpp
  ExpandWhens.cpp
  GrandCentral.cpp
  GrandCentralTaps.cpp
  IMConstProp.cpp
  InferWidths.cpp
  LowerTypes.cpp
  LowerBundleVectorTypes.cpp
  ModuleInliner.cpp
  PrintInstanceGraph.cpp

  DEPENDS
  CIRCTFIRRTLTransformsIncGen

  LINK_LIBS PUBLIC
  CIRCTFIRRTL
  CIRCTHW
  CIRCTSV
  CIRCTSupport
  MLIRIR
  MLIRPass
  MLIRTransformUtils
)
