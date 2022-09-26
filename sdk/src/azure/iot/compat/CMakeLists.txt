# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)

project (az_iot LANGUAGES C)

set(CMAKE_C_STANDARD 99)

include(CheckAndIncludeCodeCov)

# Azure IoT Compat C-SDK Library
add_library (az_iot_compat_csdk
  ${CMAKE_CURRENT_LIST_DIR}/csdk_common_adapter.c
  ${CMAKE_CURRENT_LIST_DIR}/csdk_iothub_device_client_ll_adapter.c
  ${CMAKE_CURRENT_LIST_DIR}/csdk_prov_device_client_ll_adapter.c
)

target_include_directories (az_iot_compat_csdk
  PUBLIC
    ${az_SOURCE_DIR}/sdk/inc
)

target_link_libraries(az_iot_compat_csdk
  PUBLIC
    az::core
    az::iot::common
    az::iot::hub
    az::iot::provisioning
)

add_library (az::iot::compat::csdk ALIAS az_iot_compat_csdk)
