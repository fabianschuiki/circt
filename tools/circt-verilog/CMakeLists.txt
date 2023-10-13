add_circt_tool(circt-verilog
  circt-verilog.cpp
  DEPENDS
  SUPPORT_PLUGINS
)

llvm_update_compile_flags(circt-verilog)

target_link_libraries(circt-verilog PRIVATE
  CIRCTImportVerilog
  MLIRIR
)

export_executable_symbols_for_plugins(circt-verilog)
