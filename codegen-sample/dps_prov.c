#include <stdio.h>
#include "dps_prov.h"

// Core header files for C and IoTHub layer
#include "iothub.h"
#include "iothub_device_client_ll.h"
#include "iothub_client_options.h"
#include "iothubtransportmqtt.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/xlogging.h"

// IoT Central requires DPS.  Include required header and constants
#include "azure_prov_client/iothub_security_factory.h"
#include "azure_prov_client/prov_device_ll_client.h"
#include "azure_prov_client/prov_transport_mqtt_client.h"
#include "azure_prov_client/prov_security_factory.h"

char device_connection_string[IOT_HUB_CONN_STR_MAX_LEN] = {0};

// State of DPS registration process.  We cannot proceed with DPS until we get into the state APP_DPS_REGISTRATION_SUCCEEDED.
typedef enum
{
    APP_DPS_REGISTRATION_PENDING,
    APP_DPS_REGISTRATION_SUCCEEDED,
    APP_DPS_REGISTRATION_FAILED
} APP_DPS_REGISTRATION_STATUS;

static const SECURE_DEVICE_TYPE secure_device_type = SECURE_DEVICE_TYPE_SYMMETRIC_KEY;

// The DPS global device endpoint
static const char global_dps_endpoint[] = "global.azure-devices-provisioning.net";

// DPS ID scope
static const char dps_id_scope[] = "0ne0006F1C1"; // "[DPS ID Scope]";

// DPS symmetric keys for authentication
static const char dps_sas_key[] = "0C09Aps5uSXIXRAeZrhlVCkXZwxl3hgBEAWIdQry7tdBkrpGxXtyPhSevPUVOxoHcmeZi3YpiLKDY0DZC67foQ=="; //"[DPS Symmetric Key]";

// Device Id
static const char device_id[] = "dps-device-2"; //"[Device Id]";

static const char custom_provision_data[] = "{"
                                            "\"__iot:interfaces\":"
                                            "{"
                                            "\"CapabilityModelId\": \"urn:test:pocDevice:1\""
                                            "}"
                                            "}";

// Amount in ms to sleep between querying state from DPS registration loop
#define DPS_REGISTRATION_POLL_SLEEP 100

// Maximum amount of times we'll poll for DPS registration being ready, 1 min.
#define dpsRegistrationMaxPolls (60 * 1000 / DPS_REGISTRATION_POLL_SLEEP)

// State of DigitalTwin registration process.  We cannot proceed with DigitalTwin until we get into the state APP_DIGITALTWIN_REGISTRATION_SUCCEEDED.
typedef enum APP_DIGITALTWIN_REGISTRATION_STATUS_TAG
{
    APP_DIGITALTWIN_REGISTRATION_PENDING,
    APP_DIGITALTWIN_REGISTRATION_SUCCEEDED,
    APP_DIGITALTWIN_REGISTRATION_FAILED
} APP_DIGITALTWIN_REGISTRATION_STATUS;

static char* dps_iothub_uri;
static char* dps_device_id;

static void provisioningRegisterCallback(PROV_DEVICE_RESULT register_result, const char *iothub_uri, const char *device_id, void *user_context)
{
    APP_DPS_REGISTRATION_STATUS *registration_status = (APP_DPS_REGISTRATION_STATUS *)user_context;

    if (register_result != PROV_DEVICE_RESULT_OK)
    {
        LogError("DPS Provisioning callback called with error state %d", register_result);
        *registration_status = APP_DPS_REGISTRATION_FAILED;
    }
    else
    {
        if ((mallocAndStrcpy_s(&dps_iothub_uri, iothub_uri) != 0) ||
            (mallocAndStrcpy_s(&dps_device_id, device_id) != 0))
        {
            LogError("Unable to copy provisioning information");
            *registration_status = APP_DPS_REGISTRATION_FAILED;
        }
        else
        {
            LogInfo("Provisioning callback indicates success.  iothubUri=%s, device_id=%s", dps_iothub_uri, dps_device_id);
            snprintf(device_connection_string, IOT_HUB_CONN_STR_MAX_LEN,
                     "HostName=%s;DeviceId=%s;SharedAccessKey=%s",
                     dps_iothub_uri,
                     dps_device_id,
                     dps_sas_key);
            *registration_status = APP_DPS_REGISTRATION_SUCCEEDED;
        }
    }
}

bool register_device(bool trace_on, const char *certificates)
{
    (void)certificates;
    
    PROV_DEVICE_RESULT prov_device_result;
    PROV_DEVICE_LL_HANDLE prov_device_ll_handle = NULL;
    bool result = false;
    memset(device_connection_string, 0, IOT_HUB_CONN_STR_MAX_LEN);

    APP_DPS_REGISTRATION_STATUS registration_status = APP_DPS_REGISTRATION_PENDING;

    if (IoTHub_Init() != 0)
    {
        LogError("IoTHub_Init failed");
        return false;
    }

    if (prov_dev_set_symmetric_key_info(device_id, dps_sas_key) != 0)
    {
        LogError("prov_dev_set_symmetric_key_info failed.");
    }
    else if (prov_dev_security_init(secure_device_type) != 0)
    {
        LogError("prov_dev_security_init failed");
    }
    else if ((prov_device_ll_handle = Prov_Device_LL_Create(global_dps_endpoint, dps_id_scope, Prov_Device_MQTT_Protocol)) == NULL)
    {
        LogError("failed calling Prov_Device_Create");
    }
    else if ((prov_device_result = Prov_Device_LL_SetOption(prov_device_ll_handle, PROV_OPTION_LOG_TRACE, &trace_on)) != PROV_DEVICE_RESULT_OK)
    {
        LogError("Setting provisioning tracing on failed, error=%d", prov_device_result);
    }
#ifdef SET_TRUSTED_CERT_IN_CODE
    else if ((prov_device_result = Prov_Device_LL_SetOption(prov_device_ll_handle, "TrustedCerts", certificates)) != PROV_DEVICE_RESULT_OK)
    {
        LogError("Setting provisioning TrustedCerts failed, error=%d", prov_device_result);
    }
#endif // SET_TRUSTED_CERT_IN_CODE
    else if ((prov_device_result = Prov_Device_LL_Set_Provisioning_Payload(prov_device_ll_handle, custom_provision_data)) != PROV_DEVICE_RESULT_OK)
    {
        LogError("Failed setting provisioning data, error=%d", prov_device_result);
    }
    else if ((prov_device_result = Prov_Device_LL_Register_Device(prov_device_ll_handle, provisioningRegisterCallback, &registration_status, NULL, NULL)) != PROV_DEVICE_RESULT_OK)
    {
        LogError("Prov_Device_Register_Device failed, error=%d", prov_device_result);
    }
    else
    {
        // Pulling the registration status
        for (int i = 0; (i < dpsRegistrationMaxPolls) && (registration_status == APP_DPS_REGISTRATION_PENDING); i++)
        {
            ThreadAPI_Sleep(DPS_REGISTRATION_POLL_SLEEP);
            Prov_Device_LL_DoWork(prov_device_ll_handle);
        }

        if (registration_status == APP_DPS_REGISTRATION_SUCCEEDED)
        {
            LogInfo("DPS successfully registered.  Continuing on to creation of IoTHub device client handle.");
            result = true;
        }
        else if (registration_status == APP_DPS_REGISTRATION_PENDING)
        {
            LogError("Timed out attempting to register DPS device");
        }
        else
        {
            LogError("Error registering device for DPS");
        }
    }

    if (prov_device_ll_handle != NULL)
    {
        Prov_Device_LL_Destroy(prov_device_ll_handle);
    }
    IoTHub_Deinit();

    return result;
}
