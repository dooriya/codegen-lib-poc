#include "pocsensor_interface.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/threadapi.h"

#include "digitaltwin_client_helper.h"
#include "../modellib/digitaltwin_serializer.h"
#include "../capabilities/device_capabilities.h"

#define MAX_MESSAGE_SIZE 256

//
// Callback function declarations and DigitalTwin writable (from service side) properties for this interface
//
static void PocSensorInterface_Property_SettingsCallback(const DIGITALTWIN_CLIENT_PROPERTY_UPDATE* dtClientPropertyUpdate, void* userInterfaceContext);

//
//  Callback function declarations and DigitalTwin command names for this interface.
//
static void PocSensorInterface_Command_UpdateFirmwareCallback(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* commandRequest, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* commandResponse, void* userInterfaceContext);

//
// Application state associated with this interface.
// It contains the DIGITALTWIN_INTERFACE_CLIENT_HANDLE used for responses in callbacks along with properties set
// and representations of the property update and command callbacks invoked on given interface
//
typedef struct POCSENSOR_INTERFACE_STATE_TAG
{
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceClientHandle;

    pocsensor_settings *settings;

} POCSENSOR_INTERFACE_STATE;

static POCSENSOR_INTERFACE_STATE appState;

// Callback function to process the command "updateFirmware".
void PocSensorInterface_Command_UpdateFirmwareCallback(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* commandRequest, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* commandResponse, void* userInterfaceContext)
{
    /*---------------------------------------- FIX ME ------------------------------------------*/
    POCSENSOR_INTERFACE_STATE* interfaceState = (POCSENSOR_INTERFACE_STATE*)userInterfaceContext;
    LogInfo("POCSENSOR_INTERFACE: updateFirmware command invoked.");
    LogInfo("POCSENSOR_INTERFACE: updateFirmware request payload=<%.*s>, context=<%p>", (int)commandRequest->requestDataLen, commandRequest->requestData, interfaceState);

    const unsigned char errorResponse[] = "\"Failed to execute updateFirmware command.\"";

    // Memory allocation for the request object.
    pocsensor_update_firmware_request * update_firmware_request = (pocsensor_update_firmware_request *)calloc(1, sizeof(pocsensor_update_firmware_request));
    if (update_firmware_request == NULL)
    {
        LogError("POCSENSOR_INTERFACE: failed to allocate memory for request data for updateFirmware command.");
        DigitalTwinClientHelper_SetCommandResponse(commandResponse, errorResponse, 500);
        return;
    }
 
    // Parse (deserialize) command request payload from JSON.
    if (pocsensor_update_firmware_request_from_json(update_firmware_request, (const char *)commandRequest->requestData, commandRequest->requestDataLen) != DIGITALTWIN_SERIALIZER_OK)
    {
        LogError("POCSENSOR_INTERFACE: failed to parse the request data for updateFirmware command.");
        DigitalTwinClientHelper_SetCommandResponse(commandResponse, errorResponse, 500);
        return;
    }

    unsigned int statusCode = 200;
    char responsePayload[MAX_MESSAGE_SIZE];

    // Perform update_firmware command
    bool response = pocsensor_update_firmware(update_firmware_request);

    // Serialize command response payload
    digitaltwin_serializer_result serializeResult = pocsensor_update_firmware_response_to_json(&response, responsePayload, MAX_MESSAGE_SIZE);

    if (serializeResult == DIGITALTWIN_SERIALIZER_OK)
    {
        DigitalTwinClientHelper_SetCommandResponse(commandResponse, (const unsigned char*)responsePayload, statusCode);
    }
    else
    {
        LogError("Failed to execute updateFirmware command");
        DigitalTwinClientHelper_SetCommandResponse(commandResponse, errorResponse, statusCode);
    }

    if (update_firmware_request != NULL)
    {
        if (update_firmware_request->firmware_uri != NULL)
        {
            free(update_firmware_request->firmware_uri);
        }

        free(update_firmware_request);
    }
    /*---------------------------------------------------------------------------------------------------*/
}

// DigitalTwinSample_ProcessCommandUpdate receives commands from the server.  This implementation acts as a simple dispatcher
// to the functions to perform the actual processing.
void PocSensorInterface_ProcessCommandUpdate(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtCommandRequest, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, void* userInterfaceContext)
{
    if (strcmp(dtCommandRequest->commandName, PocSensorInterface_UpdateFirmwareCommand) == 0)
    {
        PocSensorInterface_Command_UpdateFirmwareCallback(dtCommandRequest, dtCommandResponse, userInterfaceContext);
        return;
    }

    // If the command is not implemented by this interface, by convention we return a 501 error to server.
    LogError("POCSENSOR_INTERFACE: Command name <%s> is not associated with this interface", dtCommandRequest->commandName);
    const unsigned char commandNotImplementedResponse[] = "\"Requested command not implemented on this interface\"";
    (void)DigitalTwinClientHelper_SetCommandResponse(dtCommandResponse, commandNotImplementedResponse, 501);
}

// PocSensorInterface_PropertyCallback is invoked when a property is updated (or failed) going to server.
// ALL property callbacks will be routed to this function and just have the userContextCallback set to the propertyName.
// Product code will potentially have context stored in this userContextCallback.
static void PocSensorInterface_PropertyCallback(DIGITALTWIN_CLIENT_RESULT digitalTwinReportedStatus, void* userContextCallback)
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
static void PocSensorInterface_Property_SettingsCallback(const DIGITALTWIN_CLIENT_PROPERTY_UPDATE* dtClientPropertyUpdate, void* userInterfaceContext)
{
    POCSENSOR_INTERFACE_STATE* interfaceState = (POCSENSOR_INTERFACE_STATE*)userInterfaceContext;

    LogInfo("POCSENSOR_INTERFACE: settings property invoked...");
    LogInfo("POCSENSOR_INTERFACE: settings data=<%.*s>", (int)dtClientPropertyUpdate->propertyDesiredLen, dtClientPropertyUpdate->propertyDesired);

    DIGITALTWIN_CLIENT_PROPERTY_RESPONSE propertyResponse;

    // Version of this structure for C SDK.
    propertyResponse.version = DIGITALTWIN_CLIENT_PROPERTY_RESPONSE_VERSION_1;
    propertyResponse.responseVersion = dtClientPropertyUpdate->desiredVersion;

    if (interfaceState->settings != NULL)
    {
        free(interfaceState->settings);
    }

    interfaceState->settings = (pocsensor_settings *)calloc(1, sizeof(pocsensor_settings));
    if (interfaceState->settings == NULL)
    {
        LogError("POCSENSOR_INTERFACE: Out of memory updating settings...");
        propertyResponse.statusCode = 500;
        propertyResponse.statusDescription = "Out of memory";
    }
    else
    {
        digitaltwin_serializer_result serializeResult = pocsensor_settings_from_json(interfaceState->settings, (const char*)dtClientPropertyUpdate->propertyDesired, dtClientPropertyUpdate->propertyDesiredLen);
        if (serializeResult != DIGITALTWIN_SERIALIZER_OK)
        {
            LogError("POCSENSOR_INTERFACE: failed to deserialize property settings...");
            propertyResponse.statusCode = 500;
            propertyResponse.statusDescription = "failed to deserialize desired property value";
        }
        else
        {
            LogInfo("POCSENSOR_INTERFACE: Updating settings property with new desired value <%s> ...", dtClientPropertyUpdate->propertyDesired);
            bool updateResult = pocsensor_update_settings(interfaceState->settings);

            if (updateResult)
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
        }
    }
    // Returns information back to service.
    DIGITALTWIN_CLIENT_RESULT result = DigitalTwin_InterfaceClient_ReportPropertyAsync(appState.interfaceClientHandle, PocSensorInterface_SettingsProperty, dtClientPropertyUpdate->propertyDesired, dtClientPropertyUpdate->propertyDesiredLen, &propertyResponse, PocSensorInterface_PropertyCallback, PocSensorInterface_SettingsProperty);
    if (result != DIGITALTWIN_CLIENT_OK)
    {
        LogError("POCSENSOR_INTERFACE: DigitalTwin_InterfaceClient_ReportPropertyAsync for settings failed, error=<%s>", MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, result));
    }
    else
    {
        LogInfo("POCSENSOR_INTERFACE: Successfully queued property update for settings");
    }
}

// PocSensorInterface_ProcessPropertyUpdate receives updated properties from the server.  This implementation
// acts as a simple dispatcher to the functions to perform the actual processing.
static void PocSensorInterface_ProcessPropertyUpdate(const DIGITALTWIN_CLIENT_PROPERTY_UPDATE* dtClientPropertyUpdate, void* userInterfaceContext)
{
    if (strcmp(dtClientPropertyUpdate->propertyName, PocSensorInterface_SettingsProperty) == 0)
    {
        PocSensorInterface_Property_SettingsCallback(dtClientPropertyUpdate, userInterfaceContext);
        return;
    }

    // If the property is not implemented by this interface, presently we only record a log message but do not have a mechanism to report back to the service
    LogError("POCSENSOR_INTERFACE: Property name <%s> is not associated with this interface", dtClientPropertyUpdate->propertyName);
}

DIGITALTWIN_CLIENT_RESULT PocSensorInterface_Property_ReportBatteryRemaining()
{
    /*--------------------------------------- FIX ME -------------------------------------------*/
    if (appState.interfaceClientHandle == NULL)
    {
        LogError("POCSENSOR_INTERFACE: interfaceClientHandle is required to be initialized before reporting properties");
    }

    char payload_buffer[MAX_MESSAGE_SIZE];
    double battery_remaining = pocsensor_get_battery_remaining();
    if (pocsensor_battery_remaining_to_json(&battery_remaining, payload_buffer, MAX_MESSAGE_SIZE) == DIGITALTWIN_SERIALIZER_OK)
    {
        return DigitalTwin_InterfaceClient_ReportPropertyAsync(appState.interfaceClientHandle, PocSensorInterface_BatteryRemainingProperty,
            (const unsigned char*)payload_buffer, strlen(payload_buffer), NULL,
            PocSensorInterface_PropertyCallback, NULL);
    }
    else
    {
        LogError("POCSENSOR_INTERFACE: serialize read only property battery_remaining failed");
        return DIGITALTWIN_CLIENT_ERROR;
    }
    /*                                                                                          */
}

// PocSensorInterface_TelemetryCallback is invoked when a DigitalTwin telemetry message is either successfully delivered to the service or else fails.
static void PocSensorInterface_TelemetryCallback(DIGITALTWIN_CLIENT_RESULT digitalTwinTelemetryStatus, void* userContextCallback)
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

DIGITALTWIN_CLIENT_RESULT PocSensorInterface_Telemetry_SendLocation()
{
    /*-------------------------------- FIX ME -----------------------------*/
    if (appState.interfaceClientHandle == NULL)
    {
        LogError("POCSENSOR_INTERFACE: interfaceClientHandle is required to be initialized before sending telemetries");
    }

    DIGITALTWIN_CLIENT_RESULT result;

    char location_data[MAX_MESSAGE_SIZE];

    // Define a location data.
    pocsensor_location location;

    // Retrieve location data from sensor.
    pocsensor_get_location(&location);

    // Serialize location data to JSON.
    pocsensor_location_to_json(&location, location_data, MAX_MESSAGE_SIZE);

    // Send location telemetry to Azure IoT.
    if ((result = DigitalTwin_InterfaceClient_SendTelemetryAsync(appState.interfaceClientHandle, (unsigned char*)location_data, strlen(location_data), PocSensorInterface_TelemetryCallback, NULL)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("POCSENSOR_INTERFACE: DigitalTwin_InterfaceClient_SendTelemetryAsync failed for sending telemetry.");
    }

    return result;
    /*------------------------- ------------------------------------------*/
}

// PocSensorInterface_InterfaceRegisteredCallback is invoked when this interface
// is successfully or unsuccessfully registered with the service, and also when the interface is deleted.
static void PocSensorInterface_InterfaceRegisteredCallback(DIGITALTWIN_CLIENT_RESULT dtInterfaceStatus, void* userInterfaceContext)
{
    LogInfo("PocSensorInterface_InterfaceRegisteredCallback with status=<%s>, userContext=<%p>", MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, dtInterfaceStatus), userInterfaceContext);
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

//
// Create DigitalTwin interface client handle
//
DIGITALTWIN_INTERFACE_CLIENT_HANDLE PocSensorInterface_Create()
{
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle;
    DIGITALTWIN_CLIENT_RESULT result;

    memset(&appState, 0, sizeof(POCSENSOR_INTERFACE_STATE));

    result = DigitalTwin_InterfaceClient_Create(PocSensorInterfaceId, PocSensorInterfaceInstanceName, PocSensorInterface_InterfaceRegisteredCallback, (void*)&appState, &interfaceHandle);
    if (result != DIGITALTWIN_CLIENT_OK)
    {
        LogError("POCSENSOR_INTERFACE: Unable to allocate interface client handle for interfaceId=<%s>, interfaceInstanceName=<%s>, error=<%s>", PocSensorInterfaceId, PocSensorInterfaceInstanceName, MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, result));
        interfaceHandle = NULL;
        return NULL;
    }

    result = DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback(interfaceHandle, PocSensorInterface_ProcessPropertyUpdate, (void*)&appState);
    if (result != DIGITALTWIN_CLIENT_OK)
    {
        LogError("POCSENSOR_INTERFACE: DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback failed. error=<%s>", MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, result));
        PocSensorInterface_Close(interfaceHandle);
        interfaceHandle = NULL;
        return NULL;
    }

    result = DigitalTwin_InterfaceClient_SetCommandsCallback(interfaceHandle, PocSensorInterface_ProcessCommandUpdate, (void*)&appState);
    if (result != DIGITALTWIN_CLIENT_OK)
    {
        LogError("POCSENSOR_INTERFACE: DigitalTwin_InterfaceClient_SetCommandsCallbacks failed. error=<%s>", MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, result));
        PocSensorInterface_Close(interfaceHandle);
        interfaceHandle = NULL;
        return NULL;
    }
    else
    {
        LogInfo("POCSENSOR_INTERFACE: Created DIGITALTWIN_INTERFACE_CLIENT_HANDLE successfully for interfaceId=<%s>, interfaceInstanceName=<%s>, handle=<%p>", PocSensorInterfaceId, PocSensorInterfaceInstanceName, interfaceHandle);
        appState.interfaceClientHandle = interfaceHandle;
        return interfaceHandle;
    }
}

void PocSensorInterface_Close(DIGITALTWIN_INTERFACE_CLIENT_HANDLE digitalTwinInterfaceClientHandle)
{
    // On shutdown, in general the first call made should be to DigitalTwin_InterfaceClient_Destroy.
    // This will block if there are any active callbacks in this interface, and then
    // mark the underlying handle such that no future callbacks shall come to it.
    DigitalTwin_InterfaceClient_Destroy(digitalTwinInterfaceClientHandle);

    // After DigitalTwin_InterfaceClient_Destroy returns, it is safe to assume
    // no more callbacks shall arrive for this interface and it is OK to free
    // resources callbacks otherwise may have needed.

    if (appState.settings != NULL)
    {
        free(appState.settings);
        appState.settings = NULL;
    }

}
