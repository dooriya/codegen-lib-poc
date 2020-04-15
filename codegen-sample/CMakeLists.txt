#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for pnp_sample_app
project("pnp_sample_app")
cmake_minimum_required (VERSION 3.10)


set(TARGET_NAME "pnp_sample_app")

set(CMAKE_C_STANDARD 99)
if (POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)
endif()

add_compile_options(/W3)

#option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
#include(global_compile_options.txt)
set(sample_h_files
    ./dps_prov.h
)

set(sample_c_files
    ./dps_prov.c
    ./main.c
)

add_executable(${TARGET_NAME} ${sample_c_files} ${sample_h_files})

add_subdirectory(capabilities)
add_subdirectory(serializer)
add_subdirectory(interface)
add_subdirectory(az_core)
add_subdirectory(az_platform/noplatform)

# Find azure_iot_sdks Vcpkg package
find_package(azure_iot_sdks REQUIRED)

include_directories(.)
include_directories(${azure_iot_sdks_SOURCE_DIR}/deps/parson ${MQTT_INC_FOLDER} ${SHARED_UTIL_INC_FOLDER} ${UMOCK_C_INC_FOLDER} ${MACRO_UTILS_INC_FOLDER} ${IOTHUB_CLIENT_INC_FOLDER} ${DIGITALTWIN_CLIENT_INC_FOLDER} ${DEV_AUTH_MODULES_CLIENT_INC_FOLDER})

target_link_libraries(
${TARGET_NAME}
parson
iothub_client_mqtt_transport
iothub_client
digitaltwin_client
prov_device_ll_client
prov_mqtt_transport
prov_auth_client
aziotsharedutil
umqtt
pnp_capabilities
pnp_serializer
pnp_interface
)

