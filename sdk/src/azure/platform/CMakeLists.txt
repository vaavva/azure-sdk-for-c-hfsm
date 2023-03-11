# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)

project(az_platform LANGUAGES C)
set(CMAKE_C_STANDARD 99)

if(AZ_PLATFORM_IMPL STREQUAL "WIN32")
  # build windows platform
  add_library(az_win32 STATIC
    ${CMAKE_CURRENT_LIST_DIR}/az_win32.c
  )

  target_link_libraries(az_win32
    PRIVATE
      az_core
  )
elseif(AZ_PLATFORM_IMPL STREQUAL "POSIX")
  # build linux platform
  add_library(az_posix STATIC
      ${CMAKE_CURRENT_LIST_DIR}/az_posix.c
  )

  target_link_libraries(az_posix
    PRIVATE
      az_core
  )
else()
  #noplatform
  add_library(az_noplatform STATIC
    ${CMAKE_CURRENT_LIST_DIR}/az_noplatform.c
  )

  target_link_libraries(az_noplatform
    PRIVATE
      az_core
  )
endif()

# HTTP Client
add_library (
  az_nohttp
    STATIC
      ${CMAKE_CURRENT_LIST_DIR}/az_nohttp.c
)

target_link_libraries(az_nohttp PRIVATE az_core)

# make sure that users can consume the project as a library.
add_library (az::nohttp ALIAS az_nohttp)

# Curl Platform
if (TRANSPORT_CURL)
  set(CURL_MIN_REQUIRED_VERSION 7.1) #Min curl version to support CURLOPT_HTTPHEADER option
  find_package(CURL ${CURL_MIN_REQUIRED_VERSION} CONFIG)
  if(NOT CURL_FOUND)
    find_package(CURL ${CURL_MIN_REQUIRED_VERSION} REQUIRED)
  endif()

  add_library (
    az_curl
      STATIC
      ${CMAKE_CURRENT_LIST_DIR}/az_curl.c
  )

  target_link_libraries(az_curl PRIVATE az_core)

  # make sure that users can consume the project as a library.
  add_library (az::curl ALIAS az_curl)

  target_link_libraries(az_curl PUBLIC CURL::libcurl)
  target_include_directories(az_curl INTERFACE ${CURL_INCLUDE_DIR})

endif()

# Mosquitto Platform

if (TRANSPORT_MOSQUITTO)
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

  add_library (
    az_mosquitto
      STATIC
      ${CMAKE_CURRENT_LIST_DIR}/az_mosquitto.c
  )

  add_compile_definitions(az_mosquitto
    LIBMOSQUITTO_STATIC
  )

  target_link_libraries(az_mosquitto PRIVATE az_core)

  target_link_libraries(az_mosquitto 
    PUBLIC 
      ${MOSQUITTO_LIBRARIES}
      ${PTHREAD_LIBRARIES}
      #${MOSQUITTO_LIBRARY}
      OpenSSL::SSL 
      OpenSSL::Crypto
  )
  
  if(WIN32)
  target_link_libraries(az_mosquitto
    PUBLIC
      crypt32
      Ws2_32
  )
  endif()

  target_include_directories(az_mosquitto INTERFACE ${MOSQUITTO_INCLUDE_DIR})

  # make sure that users can consume the project as a library.
  add_library (az::mqtt_impl ALIAS az_mosquitto)
endif() # TRANSPORT_MOSQUITTO


# Paho Platform
if (TRANSPORT_PAHO)
  find_package(eclipse-paho-mqtt-c CONFIG)
  if(NOT eclipse-paho-mqtt-c_FOUND)
    find_package(eclipse-paho-mqtt-c REQUIRED)
  endif()

  find_package(OpenSSL CONFIG)
  if(NOT OpenSSL_FOUND)
    find_package(OpenSSL REQUIRED)
  endif()

  add_library (
    az_paho
      STATIC
      ${CMAKE_CURRENT_LIST_DIR}/az_paho.c
  )

  target_link_libraries(az_paho PRIVATE az_core)

  target_link_libraries(az_paho
  PUBLIC
    eclipse-paho-mqtt-c::paho-mqtt3cs-static
    OpenSSL::SSL
    OpenSSL::Crypto
  )

  target_include_directories(az_paho INTERFACE ${PAHO_INCLUDE_DIR})

  # make sure that users can consume the project as a library.
  add_library (az::mqtt_impl ALIAS az_paho)
endif() # TRANSPORT_PAHO
