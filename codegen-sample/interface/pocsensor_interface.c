#include "pocsensor_interface.h"
#include "../capabilities/device_capabilities.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/threadapi.h"

// DigitalTwin interface name from service perspective.
static const char pocsensor_interface_id[] = "urn:test:pocSensor:1";
static const char pocsensor_component_name[] = "pocsensor";

// Telemetry names for this interface.
static const char pocsensor_location_telemetry_name[] = "location";

// Property names for this interface.
static const char pocsensor_battery_remaining_property_name[] = "battery_remaining";
static const char pocsensor_settings_property_name[] = "settings";

// Command names for this interface
static const char pocsensor_update_firmware_command_name[] = "update_firmware";

#define MAX_MESSAGE_SIZE 256

static DIGITALTWIN_INTERFACE_CLIENT_HANDLE interface_client_handle;

// Callback function declarations and DigitalTwin writable (from service side) properties for this interface
static void pocsensor_settings_property_callback(const DIGITALTWIN_CLIENT_PROPERTY_UPDATE* dtClientPropertyUpdate, void* user_interface_context);

//  Callback function declarations and DigitalTwin command names for this interface.
static void pocsensor_command_update_firmware_callback(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* command_request, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* command_response, void* user_interface_context);

static void set_command_response(DIGITALTWIN_CLIENT_COMMAND_RESPONSE* command_response, const unsigned char* responseData, int status)
{
    memset(command_response, 0, sizeof(*command_response));
    command_response->version = DIGITALTWIN_CLIENT_COMMAND_RESPONSE_VERSION_1;
    command_response->status = status;
    int result = 0;

    if (responseData != NULL)
    {
        size_t responseLen = strlen((const char*)responseData);

        // Allocate a copy of the response data to return to the invoker.
        // takes responsibility for freeing this data.
        if (mallocAndStrcpy_s((char**)&command_response->responseData, (const char*)responseData) != 0)
        {
            LogError("ENVIRONMENTAL_SENSOR_INTERFACE: Unable to allocate response data");
            command_response->status = 500;
            result = MU_FAILURE;
        }
        else
        {
            command_response->responseDataLen = responseLen;;
        }
    }
}

// Callback function to process the command "updateFirmware".
void pocsensor_command_update_firmware_callback(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* command_request, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* command_response, void* user_interface_context)
{
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE* interface_state = (DIGITALTWIN_INTERFACE_CLIENT_HANDLE*)user_interface_context;
    LogInfo("POCSENSOR_INTERFACE: updateFirmware command invoked.");
    LogInfo("POCSENSOR_INTERFACE: updateFirmware request payload=<%.*s>, context=<%p>", (int)command_request->requestDataLen, command_request->requestData, interface_state);

    // Memory allocation for the request object.
    pocsensor_update_firmware_request update_firmware_request;

    uint8_t firmware_uri_buffer[MAX_MESSAGE_SIZE];
    update_firmware_request.firmware_uri= AZ_SPAN_FROM_BUFFER(firmware_uri_buffer);

    // Parse (deserialize) command request payload from JSON.
    // What's the best practice to initialize a span from a pointer here?
    az_span json_payload = AZ_SPAN_FROM_INITIALIZED_BUFFER((char*)command_request->requestData);
    json_payload._internal.capacity = command_request->requestDataLen;
    json_payload._internal.length = command_request->requestDataLen;

    az_result result = az_dt_update_firmware_request_from_json(&update_firmware_request, json_payload);
    if (az_failed(result))
    {
        LogError("POCSENSOR_INTERFACE: failed to parse the request data for updateFirmware command.");
        set_command_response(command_response, NULL, 500);
        return;
    }

    // Perform update_firmware command
    bool response = pocsensor_update_firmware(&update_firmware_request);

    // Serialize command response payload
    char responseJson[10];
    az_span out_json = AZ_SPAN_FROM_BUFFER(responseJson);
    az_result serialize_result = az_dt_pocsensor_update_firmware_response_to_json(&response, out_json, &out_json);
    if (az_failed(serialize_result))
    {
        set_command_response(command_response, NULL, 500);
    }
    else
    {
        set_command_response(command_response, (const unsigned char*)responseJson, 200);
    }
}

 /*
 DigitalTwinSample_ProcessCommandUpdate receives commands from the server.  This implementation acts as a simple dispatcher
 to the functions to perform the actual processing.
 */
void pocsensor_command_callback(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtCommandRequest, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, void* user_interface_context)
{
    if (strcmp(dtCommandRequest->commandName, pocsensor_update_firmware_command_name) == 0)
    {
        pocsensor_command_update_firmware_callback(dtCommandRequest, dtCommandResponse, user_interface_context);
    }
    else
    {
        // If the command is not implemented by this interface, by convention we return a 501 error to server.
        LogError("POCSENSOR_INTERFACE: Command name <%s> is not associated with this interface", dtCommandRequest->commandName);
        const unsigned char commandNotImplementedResponse[] = "\"Requested command not implemented on this interface\"";
        (void)set_command_response(dtCommandResponse, commandNotImplementedResponse, 501);
    }
}

 /*
 pocsensor_report_property_callback is invoked when a property is updated (or failed) going to server.
 ALL property callbacks will be routed to this function and just have the userContextCallback set to the propertyName.
 Product code will potentially have context stored in this userContextCallback.
 */
static void pocsensor_report_property_callback(DIGITALTWIN_CLIENT_RESULT digitalTwinReportedStatus, void* userContextCallback)
{
    if (digitalTwinReportedStatus == DIGITALTWIN_CLIENT_OK)
    {
        LogInfo("POCSENSOR_INTERFACE: Updating property=<%s> succeeded", (const char*)userContextCallback);
    }
    else
    {
        LogError("POCSENSOR_INTERFACE: Updating property=<%s> failed, error=<%s>", (const char*)userContextCallback, MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, digitalTwinReportedStatus));
    }
}

// Processes a property update, which the server initiated, for 'settings' property.
static void pocsensor_settings_property_callback(const DIGITALTWIN_CLIENT_PROPERTY_UPDATE* dtClientPropertyUpdate, void* user_interface_context)
{
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE* interface_state = (DIGITALTWIN_INTERFACE_CLIENT_HANDLE*)user_interface_context;

    LogInfo("POCSENSOR_INTERFACE: settings property invoked...");
    LogInfo("POCSENSOR_INTERFACE: settings data=<%.*s>", (int)dtClientPropertyUpdate->propertyDesiredLen, dtClientPropertyUpdate->propertyDesired);

    DIGITALTWIN_CLIENT_PROPERTY_RESPONSE propertyResponse;

    // Version of this structure for C SDK.
    propertyResponse.version = DIGITALTWIN_CLIENT_PROPERTY_RESPONSE_VERSION_1;
    propertyResponse.responseVersion = dtClientPropertyUpdate->desiredVersion;

    pocsensor_settings settings;

    // What's the best practice to initialize a span from a pointer here?
    az_span json_span = AZ_SPAN_FROM_INITIALIZED_BUFFER((uint8_t *)dtClientPropertyUpdate->propertyDesired);
    json_span._internal.capacity = dtClientPropertyUpdate->propertyDesiredLen;
    json_span._internal.length = dtClientPropertyUpdate->propertyDesiredLen;

    az_result result = az_dt_pocsensor_settings_from_json(&settings, json_span);
    if (az_failed(result))
    {
        LogError("POCSENSOR_INTERFACE: failed to deserialize property settings...");
        propertyResponse.statusCode = 500;
        propertyResponse.statusDescription = "failed to deserialize desired property value";

        DigitalTwin_InterfaceClient_ReportPropertyAsync(interface_client_handle, pocsensor_settings_property_name, dtClientPropertyUpdate->propertyDesired, dtClientPropertyUpdate->propertyDesiredLen, &propertyResponse, pocsensor_report_property_callback, (void*)pocsensor_settings_property_name);
        return;
    }

    if (pocsensor_update_settings(&settings))
    {
        propertyResponse.statusCode = 200;
        propertyResponse.statusDescription = "settings property is updated successfully";
    }
    else
    {
        LogError("POCSENSOR_INTERFACE: failed to update property settings...");
        propertyResponse.statusCode = 500;
        propertyResponse.statusDescription = "failed to update writable property";
    }

    DigitalTwin_InterfaceClient_ReportPropertyAsync(interface_client_handle, pocsensor_settings_property_name, dtClientPropertyUpdate->propertyDesired, dtClientPropertyUpdate->propertyDesiredLen, &propertyResponse, pocsensor_report_property_callback, (void*)pocsensor_settings_property_name);
}

// pocsensor_property_update_callback receives updated properties from the server.  This implementation
// acts as a simple dispatcher to the functions to perform the actual processing.
static void pocsensor_property_update_callback(const DIGITALTWIN_CLIENT_PROPERTY_UPDATE* dtClientPropertyUpdate, void* user_interface_context)
{
    if (strcmp(dtClientPropertyUpdate->propertyName, (const char*)pocsensor_settings_property_name) == 0)
    {
        pocsensor_settings_property_callback(dtClientPropertyUpdate, user_interface_context);
        return;
    }

    // If the property is not implemented by this interface, presently we only record a log message but do not have a mechanism to report back to the service
    LogError("POCSENSOR_INTERFACE: Property name <%s> is not associated with this interface", dtClientPropertyUpdate->propertyName);
}

DIGITALTWIN_CLIENT_RESULT pocsensor_repoty_battery_remaining()
{
    if (interface_client_handle == NULL)
    {
        LogError("POCSENSOR_INTERFACE: interfaceClientHandle is required to be initialized before reporting properties");
        return DIGITALTWIN_CLIENT_ERROR;
    }

    double battery_remaining = pocsensor_get_battery_remaining();
    uint8_t payload_buffer[MAX_MESSAGE_SIZE];
    az_span span = AZ_SPAN_FROM_BUFFER(payload_buffer);
    az_result result = az_dt_procsensor_battery_remaining_to_json(&battery_remaining, span, &span);
    if (az_failed(result))
    {
        return DIGITALTWIN_CLIENT_ERROR;
    }

    char* json_str = (char*)az_span_ptr(span);
    return DigitalTwin_InterfaceClient_ReportPropertyAsync(interface_client_handle, pocsensor_battery_remaining_property_name,
        (const unsigned char*)json_str, strlen(json_str), NULL,
        pocsensor_report_property_callback, NULL);
}

// pocsensor_send_telemetry_callback is invoked when a DigitalTwin telemetry message is either successfully delivered to the service or else fails.
static void pocsensor_send_telemetry_callback(DIGITALTWIN_CLIENT_RESULT digitalTwinTelemetryStatus, void* userContextCallback)
{
    (void)userContextCallback;
    if (digitalTwinTelemetryStatus == DIGITALTWIN_CLIENT_OK)
    {
        LogInfo("POCSENSOR_INTERFACE: DigitalTwin successfully delivered telemetry message.");
    }
    else
    {
        LogError("POCSENSOR_INTERFACE: DigitalTwin failed to deliver telemetry message, error=<%s> ", MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, digitalTwinTelemetryStatus));
    }
}

DIGITALTWIN_CLIENT_RESULT pocsensor_send_location_telemetry()
{
    if (interface_client_handle == NULL)
    {
        LogError("POCSENSOR_INTERFACE: interfaceClientHandle is required to be initialized before sending telemetries");
        return DIGITALTWIN_CLIENT_ERROR;
    }

    DIGITALTWIN_CLIENT_RESULT result;

    // Define a location data.
    pocsensor_location location;

    // Retrieve location data from sensor.
    pocsensor_get_location(&location);

    // Serialize location data to JSON.
    uint8_t location_data[MAX_MESSAGE_SIZE];
    az_span span = AZ_SPAN_FROM_BUFFER(location_data);
    if (az_failed(az_dt_procsensor_location_to_json(&location, span, &span)))
    {
        LogError("Failed to serialize location data.");
        return DIGITALTWIN_CLIENT_ERROR;
    }

    char message_data[MAX_MESSAGE_SIZE];
    sprintf(message_data, "{\"location\": %s}", location_data);

    // Send location telemetry to Azure IoT.
    if ((result = DigitalTwin_InterfaceClient_SendTelemetryAsync(interface_client_handle, (unsigned char*)message_data, strlen(message_data), pocsensor_send_telemetry_callback, NULL)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("POCSENSOR_INTERFACE: DigitalTwin_InterfaceClient_SendTelemetryAsync failed for sending telemetry.");
        return DIGITALTWIN_CLIENT_ERROR;
    }

    return result;
}

/*
pocsensor_create_interface_callback is invoked when this interface
is successfully or unsuccessfully registered with the service, and also when the interface is deleted.
*/
static void pocsensor_create_interface_callback(DIGITALTWIN_CLIENT_RESULT dtInterfaceStatus, void* user_interface_context)
{
    LogInfo("pocsensor_create_interface_callback with status=<%s>, userContext=<%p>", MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, dtInterfaceStatus), user_interface_context);
    if (dtInterfaceStatus == DIGITALTWIN_CLIENT_OK)
    {
        // Once the interface is registered, send our reported properties to the service.  
        // It *IS* safe to invoke most DigitalTwin API calls from a callback thread like this, though it 
        // is NOT safe to create/destroy/register interfaces now.
        LogInfo("POCSENSOR_INTERFACE: Interface successfully registered.");
    }
    else if (dtInterfaceStatus == DIGITALTWIN_CLIENT_ERROR_INTERFACE_UNREGISTERING)
    {
        // Once an interface is marked as unregistered, it cannot be used for any DigitalTwin SDK calls.
        LogInfo("POCSENSOR_INTERFACE: Interface received unregistering callback.");
    }
    else
    {
        LogError("POCSENSOR_INTERFACE: Interface received failed, status=<%s>.", MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, dtInterfaceStatus));
    }
}

/* Create DigitalTwin interface client handle */
DIGITALTWIN_INTERFACE_CLIENT_HANDLE pocsensor_create()
{
    DIGITALTWIN_CLIENT_RESULT result;

    memset(&interface_client_handle, 0, sizeof(DIGITALTWIN_INTERFACE_CLIENT_HANDLE));

    result = DigitalTwin_InterfaceClient_Create(pocsensor_interface_id, pocsensor_component_name, pocsensor_create_interface_callback, (void*)&interface_client_handle, &interface_client_handle);

    if (result != DIGITALTWIN_CLIENT_OK)
    {
        LogError("POCSENSOR_INTERFACE: Unable to allocate interface client handle for interfaceId=<%s>, interfaceInstanceName=<%s>, error=<%s>", pocsensor_interface_id, pocsensor_component_name, MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, result));
        interface_client_handle = NULL;
        return NULL;
    }

    result = DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback(interface_client_handle, pocsensor_property_update_callback, (void*)&interface_client_handle);
    if (result != DIGITALTWIN_CLIENT_OK)
    {
        LogError("POCSENSOR_INTERFACE: DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback failed. error=<%s>", MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, result));
        pocsensor_close(interface_client_handle);
        interface_client_handle = NULL;
        return NULL;
    }

    result = DigitalTwin_InterfaceClient_SetCommandsCallback(interface_client_handle, pocsensor_command_callback, (void*)&interface_client_handle);
    if (result != DIGITALTWIN_CLIENT_OK)
    {
        LogError("POCSENSOR_INTERFACE: DigitalTwin_InterfaceClient_SetCommandsCallbacks failed. error=<%s>", MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, result));
        pocsensor_close(interface_client_handle);
        interface_client_handle = NULL;
        return NULL;
    }
    else
    {
        LogInfo("POCSENSOR_INTERFACE: Created DIGITALTWIN_INTERFACE_CLIENT_HANDLE successfully for interfaceId=<%s>, interfaceInstanceName=<%s>, handle=<%p>", pocsensor_interface_id, pocsensor_component_name, interface_client_handle);
        return interface_client_handle;
    }
}

void pocsensor_close(DIGITALTWIN_INTERFACE_CLIENT_HANDLE digitalTwinInterfaceClientHandle)
{
    // On shutdown, in general the first call made should be to DigitalTwin_InterfaceClient_Destroy.
    // This will block if there are any active callbacks in this interface, and then
    // mark the underlying handle such that no future callbacks shall come to it.
    DigitalTwin_InterfaceClient_Destroy(digitalTwinInterfaceClientHandle);
}
