/************************************************************************************************
 * This code was automatically generated by Digital Twin Code Generator tool 0.6.8.
 * Changes to this file may cause incorrect behavior and will be lost if the code is regenerated.
 *
 * Generated Date: 3/23/2020
 ***********************************************************************************************/

#ifndef DIGITALTWIN_SERIALIZER_H
#define DIGITALTWIN_SERIALIZER_H

/**
 * @file digitaltwin_serializer.h
 *
 * @brief Provide a consistent layer for serializing / deserializing PnP Schema.
 * A corresponding serialize function will be generated for:
 *     1) A telemetry.
 *     2) A read-only property.
 *     3) A command response.
 * A corresponding deserialize function will be generated for:
 *     1) A writable property.
 *     2) A command request.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "../pnp_schema_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Digital Twin data serialization / deserialization result.
 *
 */
typedef enum digitaltwin_serializer_result_tag
{
    // serializer success result
    DIGITALTWIN_SERIALIZER_OK = 0,

    // serializer error results
    DIGITALTWIN_SERIALIZER_BUFFER_OVERFLOW,
    DIGITALTWIN_SERIALIZER_INVALID_TYPE,
    DIGITALTWIN_SERIALIZER_INTERNAL_FAILURE,
    DIGITALTWIN_SERIALIZER_NOT_SUPPORTED,
    DIGITALTWIN_SERIALIZER_NULL_PASSEDIN,
    DIGITALTWIN_SERIALIZER_OUT_OF_MEMORY,
    DIGITALTWIN_SERIALIZER_UNEXPECTED_ENUM
} digitaltwin_serializer_result;

/************************** Serialization APIs ****************************/
/**
 * @brief Serialize the location data to a JSON string stored in the user-specified data buffer.
 *
 * @param location Pointer to the location data to be serialized.
 * @param out_buffer Pointer to the output data buffer allocated by user.
 * @param max_size Allocated buffer size.
 * @return DIGITALTWIN_SERIALIZER_OK if the serialization is successful, other value indicates a failure.
 */
digitaltwin_serializer_result pocinterface_location_to_json(pocinterface_location * location, char * out_buffer, size_t max_size);

/**
 * @brief Serialize the battery_remaining data to a JSON string stored in the user-specified data buffer.
 *
 * @param battery_remaining Pointer to the battery_remaining data to be serialized.
 * @param out_buffer Pointer to the output data buffer allocated by user.
 * @param max_size Allocated buffer size.
 * @return DIGITALTWIN_SERIALIZER_OK if the serialization is successful, other value indicates a failure.
 */
digitaltwin_serializer_result pocinterface_battery_remaining_to_json(double * battery_remaining, char* out_buffer, size_t max_size);

/**
 * @brief Serialize the response data of update_firmware command to a JSON string stored in the user-specified data buffer.
 *
 * @param response Pointer to the response data to be serialized.
 * @param out_buffer Pointer to the output data buffer allocated by user.
 * @param max_size Allocated buffer size.
 * @return DIGITALTWIN_SERIALIZER_OK if the serialization is successful, other value indicates a failure.
 */
digitaltwin_serializer_result pocinterface_update_firmware_response_to_json(bool * response, char* out_buffer, size_t max_size);

/************************** Deserialization APIs ****************************/

/**
 * @brief Deserialize the desired value for settings property from a specified data buffer.
 *
 * @param desiredValue Pointer to the deserialized object allocated by user.
 * @param data_buffer Pointer to the buffer containing data to be deserialized.
 * @param size Length of the buffer containing data to be deserialized.
 * @return DIGITALTWIN_SERIALIZER_OK if the deserialization is successful, other value indicates a failure.
 */
digitaltwin_serializer_result pocinterface_settings_from_json(pocinterface_settings* settings, const char* data_buffer, size_t size);

/**
 * @brief Deserialize the request value for updateFirmware command from a specified data buffer.
 *
 * @param request Pointer to the deserialized object allocated by user.
 * @param data_buffer Pointer to the buffer containing data to be deserialized.
 * @param size Length of the buffer containing data to be deserialized.
 * @return DIGITALTWIN_SERIALIZER_OK if the deserialization is successful, other value indicates a failure.
 */
digitaltwin_serializer_result pocinterface_update_firmware_request_from_json(pocinterface_updatefirmware_request* request, const char* data_buffer, size_t size);

#ifdef __cplusplus
}
#endif

#endif // DIGITALTWIN_SERIALIZER_H
