set(LLVM_LINK_COMPONENTS Support)

include_directories(include)
add_circt_tool(tinc
  src/Lexer.cpp
  src/Parser.cpp
  tinc.cpp
)
target_link_libraries(tinc
  PRIVATE
  MLIRIR
  MLIRParser
  MLIRSupport
)

add_subdirectory(test)
