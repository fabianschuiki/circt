set(LLVM_LINK_COMPONENTS Support)

include_directories(include)
add_circt_tool(tinc
  src/AST.cpp
  src/Codegen.cpp
  src/Lexer.cpp
  src/Names.cpp
  src/Parser.cpp
  tinc.cpp
)
target_link_libraries(tinc
  PRIVATE
  CIRCTComb
  CIRCTDebug
  CIRCTHW
  MLIRIR
  MLIRParser
  MLIRSupport
)

add_subdirectory(test)
