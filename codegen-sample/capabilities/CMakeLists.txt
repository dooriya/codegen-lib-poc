cmake_minimum_required (VERSION 3.10)

set(TARGET_NAME "pnp_capabilities")

project (${TARGET_NAME} LANGUAGES C)

set(CMAKE_C_STANDARD 99)

add_library (
  ${TARGET_NAME}
  device_capabilities.h
  device_capabilities.c
  )

target_link_libraries(${TARGET_NAME} pnp_serializer)

# make sure that users can consume the project as a library.
add_library (pnp_capabililties ALIAS ${TARGET_NAME})
