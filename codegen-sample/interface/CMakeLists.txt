cmake_minimum_required (VERSION 3.10)

set(TARGET_NAME "pnp_interface")

project (${TARGET_NAME} LANGUAGES C)

set(CMAKE_C_STANDARD 99)

add_library (
  ${TARGET_NAME}
  digitaltwin_client_helper.h
  digitaltwin_client_helper.c
  pocsensor_interface.h
  pocsensor_interface.c
  )

# Find azure_iot_sdks Vcpkg package
find_package(azure_iot_sdks REQUIRED)

# Link requried library here if needed.
target_link_libraries(
${TARGET_NAME}
parson
aziotsharedutil
prov_auth_client
iothub_client_mqtt_transport
iothub_client
digitaltwin_client
umqtt
pnp_capabilities
pnp_serializer
)

# make sure that users can consume the project as a library.
add_library (pnp_interface ALIAS ${TARGET_NAME})
