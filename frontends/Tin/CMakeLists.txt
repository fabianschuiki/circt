set(LLVM_LINK_COMPONENTS Support)

include_directories(include)
add_circt_tool(tinc
  src/AST.cpp
  src/Codegen.cpp
  src/Lexer.cpp
  src/Parser.cpp
  tinc.cpp
)
target_link_libraries(tinc
  PRIVATE
  CIRCTHW
  CIRCTComb
  MLIRIR
  MLIRParser
  MLIRSupport
)

add_subdirectory(test)
