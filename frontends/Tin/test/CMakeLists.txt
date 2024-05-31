# ===- CMakeLists.txt - Tin tests cmake -----------------------*- cmake -*-===//
#
# Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# ===-----------------------------------------------------------------------===//

set(TIN_TEST_DEPENDS
  FileCheck count not
  tinc
)

configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py
  TIN_TEST_CONFIG
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg.py
)

add_lit_testsuite(check-tin "Running the Tin tests"
  ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${TIN_TEST_DEPENDS}
  )
set_target_properties(check-tin PROPERTIES FOLDER "TinTests")

