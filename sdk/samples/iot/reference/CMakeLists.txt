# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

if (TRANSPORT_PAHO)

cmake_minimum_required (VERSION 3.10)

project (az_iot_reference_samples LANGUAGES C)

set(CMAKE_C_STANDARD 99)

find_package(eclipse-paho-mqtt-c CONFIG)
if(NOT eclipse-paho-mqtt-c_FOUND)
  find_package(eclipse-paho-mqtt-c REQUIRED)
endif()

find_package(OpenSSL CONFIG)
if(NOT OpenSSL_FOUND)
  find_package(OpenSSL REQUIRED)
endif()

# State machine example
add_executable (hsm_example 
  src/hsm_example.c
)

# SDK deps
target_link_libraries(hsm_example
    PRIVATE
      az::core
)

# Paho async based sample
add_executable (paho_iot_reference_example 
  src/paho_iot_reference_example.c
  src/reference/az_iot_hsm.c
  src/reference/az_iot_hsm_mqtt.c
  src/reference/az_iot_hsm_mqtt_pal_paho.c
)

# SDK deps
target_link_libraries(paho_iot_reference_example
    PRIVATE
      az::iot::provisioning
      az::iot::hub
)

# External deps
target_link_libraries(paho_iot_reference_example
  PRIVATE
    eclipse-paho-mqtt-c::paho-mqtt3as-static
    OpenSSL::SSL
    OpenSSL::Crypto
)

endif()
