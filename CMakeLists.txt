# SPDX-License-Identifier: BSL-1.0
# Copyright (c) 2026 even-r1-esp32s3 contributors.
cmake_minimum_required(VERSION 3.16)
set(PROJECT_VER "0.3.0")
list(APPEND EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/src")
list(REMOVE_DUPLICATES EXTRA_COMPONENT_DIRS)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(r1_compat_experiment)
