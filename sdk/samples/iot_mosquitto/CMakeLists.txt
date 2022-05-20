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

# Azure IoT Samples Executables

# Azure IoT Sample
add_executable (mosquitto_iot_sample
  ${CMAKE_CURRENT_LIST_DIR}/mosquitto_iot_sample.c
)

target_link_libraries(mosquitto_iot_sample
  PRIVATE
    az::iot::hub
    az::iot::provisioning
    mosquitto
)

create_map_file(mosquitto_iot_sample mosquitto_iot_sample.map)

endif() # TRANSPORT_PAHO
