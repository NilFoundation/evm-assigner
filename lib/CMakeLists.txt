# evmone: Fast Ethereum Virtual Machine implementation
# Copyright 2018 The evmone Authors.
# SPDX-License-Identifier: Apache-2.0

hunter_add_package(intx)
find_package(intx CONFIG REQUIRED)

# Find crypto3 and blueprint packages
# TODO: replace with find_package if possible
string(REPLACE ":" ";" MODULE_LIST $ENV{CMAKE_PREFIX_PATH})
foreach(module_path ${MODULE_LIST})
    if (module_path MATCHES ".*blueprint")
        set(BLUEPRINT_DIR ${module_path})
    elseif(module_path MATCHES ".*[cC]rypto3")
        set(CRYPTO3_DIR ${module_path})
    endif()
endforeach()

if ("${BLUEPRINT_DIR}" STREQUAL "")
    message(FATAL_ERROR "blueprint library was not found. You should provide it through CMAKE_PREFIX_PATH")
endif()
if ("${CRYPTO3_DIR}" STREQUAL "")
    message(FATAL_ERROR "crypto3 library was not found. You should provide it through CMAKE_PREFIX_PATH")
endif()

add_subdirectory(fprinter)
add_subdirectory(evmone)
add_subdirectory(assigner)
