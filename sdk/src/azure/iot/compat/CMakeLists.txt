# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)

project (az_iot_compat LANGUAGES C)

set(CMAKE_C_STANDARD 99)

find_package(mosquitto  CONFIG)
if(NOT mosquitto_FOUND)
  find_package(mosquitto REQUIRED)
endif()

find_package(OpenSSL CONFIG)
if(NOT OpenSSL_FOUND)
  find_package(OpenSSL REQUIRED)
endif()

if (WIN32)
  find_package(pthread CONFIG)
  if(NOT pthread_FOUND)
    find_package(pthread REQUIRED)
  endif()
  # HFSM_TODO: From https://github.com/microsoft/vcpkg/issues/23705 and associated PR:
  #find_package(PThreads4W REQUIRED)
  #target_link_libraries(main PRIVATE PThreads4W::PThreads4W)
endif()

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
    ${MOSQUITTO_LIBRARIES}
    ${PTHREAD_LIBRARIES}
    OpenSSL::SSL 
    OpenSSL::Crypto
    az::mosquitto
    az::iot::hub
    az::iot::provisioning
    az::iot::hfsm
    rt  #HFSM_TODO: Test only: -lrt (Posix) should be included to az_platform, not here.
)

add_library (az::iot::compat::csdk ALIAS az_iot_compat_csdk)
