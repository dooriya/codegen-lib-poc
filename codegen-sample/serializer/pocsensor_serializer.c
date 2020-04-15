/**
 * @file pocsensor_serializer.c
 * @brief Implements APIs to serialize and deserialize data from JSON format. The file relies on
 * the parson C library to handle the JSON serialization and deserialization. Supports all primitive
 * data types (e.g. integer, string, boolean), Enum and Object scheme defined in DTDL (http://aka.ms/dtdl).
 * Other complex data schemas, like Array, nested Object, Map are not supported currently.
 */


#include <string.h>
#include <az_json.h>
#include <az_precondition_internal.h>
#include <_az_cfg.h>
#include "pocsensor_serializer.h"

/**************** Serialize telemetry value ******************/
az_result az_dt_procsensor_location_to_json(pocsensor_location const* source, az_span destination, az_span* out_json)
{
    AZ_PRECONDITION_NOT_NULL(source);
    AZ_PRECONDITION_NOT_NULL(out_json);
    AZ_PRECONDITION_VALID_SPAN(destination, 0, true);

    az_json_builder builder = { 0 };

    AZ_RETURN_IF_FAILED(az_json_builder_init(&builder, *out_json));
    AZ_RETURN_IF_FAILED(az_json_builder_append_token(&builder, az_json_token_object_start()));
    AZ_RETURN_IF_FAILED(az_json_builder_append_object(&builder, AZ_SPAN_FROM_STR("latitude"), az_json_token_number(source->latitude)));
    AZ_RETURN_IF_FAILED(az_json_builder_append_object(&builder, AZ_SPAN_FROM_STR("longitude"), az_json_token_number(source->longitude)));
    AZ_RETURN_IF_FAILED(az_json_builder_append_token(&builder, az_json_token_object_end()));

    // Append the 0-terminating byte to the serialized json string.
    *out_json = az_json_builder_span_get(&builder);
    AZ_RETURN_IF_FAILED(az_span_append_uint8(*out_json, '\0', out_json));

    return AZ_OK;
}

/******************** Serialize reported property value ***********************/
az_result az_dt_procsensor_battery_remaining_to_json(double const* source, az_span destination, az_span* out_json)
{
    AZ_PRECONDITION_NOT_NULL(source);
    AZ_PRECONDITION_NOT_NULL(out_json);
    AZ_PRECONDITION_VALID_SPAN(destination, 0, true);

    az_json_builder builder = { 0 };

    AZ_RETURN_IF_FAILED(az_json_builder_init(&builder, *out_json));
    AZ_RETURN_IF_FAILED(az_json_builder_append_token(&builder, az_json_token_number(*source)));

    // Append the 0-terminating byte to the serialized json string.
    *out_json = az_json_builder_span_get(&builder);
    AZ_RETURN_IF_FAILED(az_span_append_uint8(*out_json, '\0', out_json));

    return AZ_OK;
}

/*********************** Serialize command response data ***********************/
az_result az_dt_pocsensor_update_firmware_response_to_json(bool const* source, az_span destination, az_span* out_json)
{
    AZ_PRECONDITION_NOT_NULL(source);
    AZ_PRECONDITION_NOT_NULL(out_json);
    AZ_PRECONDITION_VALID_SPAN(destination, 0, true);

    az_json_builder builder = { 0 };

    AZ_RETURN_IF_FAILED(az_json_builder_init(&builder, *out_json));
    AZ_RETURN_IF_FAILED(az_json_builder_append_token(&builder, az_json_token_boolean(*source)));

    // Append the 0-terminating byte to the serialized json string.
    *out_json = az_json_builder_span_get(&builder);
    AZ_RETURN_IF_FAILED(az_span_append_uint8(*out_json, '\0', out_json));

    return AZ_OK;
}

/********************* Deserialize desired property value ***********************/
az_result az_dt_pocsensor_settings_from_json(pocsensor_settings* destination, az_span json_buffer)
{
    AZ_PRECONDITION_NOT_NULL(destination);
    AZ_PRECONDITION_VALID_SPAN(json_buffer, 0, true);

    az_json_parser parser = { 0 };
    (void)az_json_parser_init(&parser, json_buffer);
    az_json_token token;
    (void)az_json_parser_parse_token(&parser, &token);
    if (token.kind != AZ_JSON_TOKEN_OBJECT_START)
    {
        return AZ_ERROR_ITEM_NOT_FOUND;
    }

    while (true)
    {
        az_json_token_member member;
        az_result result = az_json_parser_parse_token_member(&parser, &member);
        if (result == AZ_ERROR_ITEM_NOT_FOUND)
        {
            return az_json_parser_done(&parser);
        }
        else if (az_failed(result))
        {
            return result;
        }

        if (az_span_is_content_equal(member.name, AZ_SPAN_FROM_STR("fan_speed")))
        {
            destination->fan_speed = member.token._internal.number;
        }
        else if (az_span_is_content_equal(member.name, AZ_SPAN_FROM_STR("voltage")))
        {
            destination->voltage = member.token._internal.number;
        }
        else
        {
            return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
        }
    }

    return AZ_OK;
}

/********************* Deserialize command request data **************************/
az_result az_dt_update_firmware_request_from_json(pocsensor_update_firmware_request* destination, az_span json_buffer)
{
    // Suspend C6011 warning
    if (destination == NULL)
    {
        return AZ_ERROR_ARG;
    }

    AZ_PRECONDITION_NOT_NULL(destination);
    AZ_PRECONDITION_VALID_SPAN(destination->firmware_uri, 0, true);
    AZ_PRECONDITION_VALID_SPAN(json_buffer, 0, true);

    az_json_parser parser = { 0 };
    (void)az_json_parser_init(&parser, json_buffer);
    az_json_token token;
    (void)az_json_parser_parse_token(&parser, &token);
    if (token.kind != AZ_JSON_TOKEN_OBJECT_START)
    {
        return AZ_ERROR_ITEM_NOT_FOUND;
    }

    while (true)
    {
        az_json_token_member member;
        az_result result = az_json_parser_parse_token_member(&parser, &member);
        if (result == AZ_ERROR_ITEM_NOT_FOUND)
        {
            return az_json_parser_done(&parser);
        }
        else if (az_failed(result))
        {
            return result;
        }

        if (az_span_is_content_equal(member.name, AZ_SPAN_FROM_STR("firmware_uri")))
        {
            az_span firmware_uri_span = destination->firmware_uri;
            AZ_RETURN_IF_FAILED(az_span_copy(firmware_uri_span, member.token._internal.string, &firmware_uri_span));
            AZ_RETURN_IF_FAILED(az_span_append_uint8(firmware_uri_span, '\0', &firmware_uri_span));
        }
        else if (az_span_is_content_equal(member.name, AZ_SPAN_FROM_STR("firmware_version")))
        {
            destination->firmware_version = (int)member.token._internal.number;
        }
        else
        {
            return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
        }
    }

    return AZ_OK;
}
