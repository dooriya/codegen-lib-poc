cmake_minimum_required (VERSION 3.10)

set(TARGET_NAME "pnp_serializer")
project (${TARGET_NAME} LANGUAGES C)

set(CMAKE_C_STANDARD 99)

add_library (
  ${TARGET_NAME}
  pocsensor_serializer.c
  pocsensor_serializer.h
)

# Link requried library here if needed.
target_link_libraries(${TARGET_NAME} az_core)

# make sure that users can consume the project as a library.
add_library (pnp_serializer ALIAS ${TARGET_NAME})
