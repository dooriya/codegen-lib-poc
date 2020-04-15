/**  @file digitaltwin_client_helper.h
*    @brief  DigitalTwin InterfaceClient helper functions.
*
*    @details  DIGITALTWIN_CLIENT_HELPER is wrapper of DigitalTwin device client and interface client. These helper functions can facilitate the
               usage of DigitalTwin client for receiving commands, reporting properties, updated read/write properties, and sending telemetry.
*/

#ifndef DIGITALTWIN_CLIENT_HELPER_H
#define DIGITALTWIN_CLIENT_HELPER_H

#include "iothub.h"
#include "digitaltwin_device_client_ll.h"
#include "digitaltwin_interface_client.h"
#include "iothub_device_client_ll.h"
#include "iothub_client_options.h"
#include "iothubtransportmqtt.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/threadapi.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
* @brief State of DigitalTwin registration process.  We cannot proceed with DigitalTwin until we get into the state DIGITALTWIN_REGISTRATION_SUCCEEDED.
*/
typedef enum
{
    DIGITALTWIN_REGISTRATION_PENDING,
    DIGITALTWIN_REGISTRATION_SUCCEEDED,
    DIGITALTWIN_REGISTRATION_FAILED
} digitaltwin_registration_status;

/**
* @brief    Initializes underlying IoTHub client, creates a device handle
*           with the specified connection string, and sets some options on this handle prior to beginning.
*/
DIGITALTWIN_DEVICE_CLIENT_LL_HANDLE az_dt_client_init_device_handle(const char* device_connection_string, bool trace_on, const char * trustedCert);

/**
* @brief    Invokes DigitalTwin_DeviceClient_RegisterInterfacesAsync, which indicates to Azure IoT which DigitalTwin interfaces this device supports.
*           The DigitalTwin Handle *is not valid* until this operation has completed (as indicated by the callback DigitalTwinClientHelper_RegisterInterfacesAndWait being invoked)
*           This call will block until interfaces are successfully registered, we get a failure from server, or we timeout.
*/
DIGITALTWIN_CLIENT_RESULT az_dt_client_register_interface(DIGITALTWIN_DEVICE_CLIENT_LL_HANDLE digitalTwinDeviceClientHandle, const char* deviceCapabilityModelUri, DIGITALTWIN_INTERFACE_CLIENT_HANDLE* interfaceClientHandles, int numInterfaceClientHandles);


/**
* @brief    De-initialized DigitalTwin client handle and IoT Hub device handle
*/
void az_dt_client_deinit();


#ifdef __cplusplus
}
#endif

#endif // DIGITALTWIN_CLIENT_HELPER_H
