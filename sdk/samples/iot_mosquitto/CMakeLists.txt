# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

if (TRANSPORT_MOSQUITTO)

cmake_minimum_required (VERSION 3.10)

project (az_iot_samples LANGUAGES C)

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

# Azure IoT Samples Executables

# Azure IoT Mosquitto Connect Sample
add_executable (mosquitto_sample_connect
  ${CMAKE_CURRENT_LIST_DIR}/mosquitto_sample_connect.c
)
add_compile_definitions(mosquitto_sample_connect
  LIBMOSQUITTO_STATIC
)

target_include_directories(mosquitto_sample_connect
  PRIVATE
    ${MOSQUITTO_INCLUDE_DIR}
)

target_link_libraries(mosquitto_sample_connect
  PRIVATE
    ${MOSQUITTO_LIBRARIES}
    ${PTHREAD_LIBRARIES}
    OpenSSL::SSL 
    OpenSSL::Crypto
    az::iot::hub
    az::iot::provisioning
)

if(WIN32)
target_link_libraries(mosquitto_sample_connect
  PRIVATE
  crypt32
  Ws2_32
)
endif()

create_map_file(mosquitto_sample_connect mosquitto_sample_connect.map)

endif() # TRANSPORT_MOSQUITTO
