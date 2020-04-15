#include <stdio.h>
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/tickcounter.h"
#include "digitaltwin_device_client_ll.h"
#include "interface/digitaltwin_client_helper.h"
#include "interface/pocsensor_interface.h"

#include "dps_prov.h"

#ifdef SET_TRUSTED_CERT_IN_CODE
#include "certs.h"
#else
static const char *certificates = NULL;
#endif // SET_TRUSTED_CERT_IN_CODE


static bool iotHubConnected = false;

// Number of Digital Twins interfaces that this device supports.
#define DIGITALTWIN_INTERFACE_NUM 1
#define POCSENSOR_INDEX 0

#define DEFAULT_SEND_TELEMETRY_INTERVAL_MS 3000
#define DEVICE_CAPABILITY_MODEL_URI "urn:test:pocDevice:1"

static DIGITALTWIN_DEVICE_CLIENT_LL_HANDLE dt_device_client_handle = NULL;
static DIGITALTWIN_INTERFACE_CLIENT_HANDLE interface_client_handles[DIGITALTWIN_INTERFACE_NUM];
static TICK_COUNTER_HANDLE tickcounter = NULL;
static tickcounter_ms_t lastTickSend;

int pnp_device_initialize(const char* deviceConnectionString, const char* trustedCert)
{
    if ((tickcounter = tickcounter_create()) == NULL)
    {
        return -1;
    }

    memset(&interface_client_handles, 0, sizeof(interface_client_handles));

    // Initialize DigitalTwin device handle
    if ((dt_device_client_handle = az_dt_client_init_device_handle(deviceConnectionString, false, trustedCert)) == NULL)
    {
        LogError("az_dt_client_init_device_handle failed");
        return -1;
    }

    // Invoke to the ***Interface_Create - implemented in a separate library - to create DIGITALTWIN_INTERFACE_CLIENT_HANDLE.
    // NOTE: Other than creation and destruction, NO operations may occur on any DIGITALTWIN_INTERFACE_CLIENT_HANDLE
    // until after we've completed its registration (see az_dt_client_register_interface).
    if ((interface_client_handles[POCSENSOR_INDEX] = pocsensor_create(dt_device_client_handle)) == NULL)
    {
        LogError("pocsensor_create failed");
        return -1;
    }

    // Register the interface(s) we've created with Azure IoT.  This call will block until interfaces
    // are successfully registered, we get a failure from server, or we timeout.
    if (az_dt_client_register_interface(dt_device_client_handle, DEVICE_CAPABILITY_MODEL_URI, interface_client_handles, DIGITALTWIN_INTERFACE_NUM) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("az_dt_client_register_interface failed");
        return -1;
    }

    DigitalTwin_DeviceClient_LL_DoWork(dt_device_client_handle);
    ThreadAPI_Sleep(100);
    return 0;
}

void pnp_device_run()
{
    tickcounter_ms_t nowTick;
    tickcounter_get_current_ms(tickcounter, &nowTick);

    if (nowTick - lastTickSend >= DEFAULT_SEND_TELEMETRY_INTERVAL_MS)
    {
        // report properties
        PocSensorInterface_Property_ReportBatteryRemaining();

        // Send telemetry
        pocsensor_send_location_telemetry();

        tickcounter_get_current_ms(tickcounter, &lastTickSend);
    }
    else
    {
        // Just check data from IoT Hub
        DigitalTwin_DeviceClient_LL_DoWork(dt_device_client_handle);
        ThreadAPI_Sleep(100);
    }
}

void pnp_device_close()
{
    if (interface_client_handles[POCSENSOR_INDEX] != NULL)
    {
        pocsensor_close(interface_client_handles[POCSENSOR_INDEX]);
    }

    az_dt_client_deinit();
}

static void setup()
{
    iotHubConnected = false;

    // Initialize device model application
    bool provResult = register_device(false, certificates);
    if (provResult)
    {      
        if (pnp_device_initialize(device_connection_string, certificates) == 0)
        {
            iotHubConnected = true;
            LogInfo("PnP enabled, running...");
        }
    }
    else
    {
        LogError("DPS provisioning failed.");
    }
}

// main entry point.
int main()
{
    setup();
    
    if (iotHubConnected)
    {
        while (true)
        {
            pnp_device_run();
            ThreadAPI_Sleep(100);
        }
    }

    return 0;
}
