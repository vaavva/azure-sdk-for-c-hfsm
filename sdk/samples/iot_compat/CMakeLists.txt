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

# Azure IoT C-SDK Compat Samples Executables

# IoT Hub Client C-SDK Compat Sample
add_executable (iothub_ll_client_x509_sample
  ${CMAKE_CURRENT_LIST_DIR}/iothub_ll_client_x509_sample.c
)
add_compile_definitions(iothub_ll_client_x509_sample
  LIBMOSQUITTO_STATIC
)

target_include_directories(iothub_ll_client_x509_sample
  PRIVATE
    ${MOSQUITTO_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/sdk/inc/azure/iot/compat
)

target_link_libraries(iothub_ll_client_x509_sample
  PRIVATE
    ${MOSQUITTO_LIBRARIES}
    ${PTHREAD_LIBRARIES}
    OpenSSL::SSL 
    OpenSSL::Crypto
    az::iot::hub
    az::iot::provisioning
    az::iot::compat::csdk
)

if(WIN32)
target_link_libraries(iothub_ll_client_x509_sample
  PRIVATE
  crypt32
  Ws2_32
)
endif()

create_map_file(iothub_ll_client_x509_sample iothub_ll_client_x509_sample.map)

# Provisioning Client C-SDK Compat Sample
add_executable (prov_dev_client_ll_x509_sample
  ${CMAKE_CURRENT_LIST_DIR}/prov_dev_client_ll_x509_sample.c
)
add_compile_definitions(prov_dev_client_ll_x509_sample
  LIBMOSQUITTO_STATIC
)

target_include_directories(prov_dev_client_ll_x509_sample
  PRIVATE
    ${MOSQUITTO_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/sdk/inc/azure/iot/compat
)

target_link_libraries(prov_dev_client_ll_x509_sample
  PRIVATE
    ${MOSQUITTO_LIBRARIES}
    ${PTHREAD_LIBRARIES}
    OpenSSL::SSL 
    OpenSSL::Crypto
    az::mqtt_impl
    az::iot::hub
    az::iot::provisioning
    az::iot::compat::csdk
)

if(WIN32)
target_link_libraries(prov_dev_client_ll_x509_sample
  PRIVATE
  crypt32
  Ws2_32
)
endif()

create_map_file(prov_dev_client_ll_x509_sample prov_dev_client_ll_x509_sample.map)

endif() # TRANSPORT_MOSQUITTO
