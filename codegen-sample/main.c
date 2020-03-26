#include <stdio.h>
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/xlogging.h"

#include "dps_prov.h"
#include "pnp_device.h"

#ifdef SET_TRUSTED_CERT_IN_CODE
#include "certs.h"
#else
static const char *certificates = NULL;
#endif // SET_TRUSTED_CERT_IN_CODE

static bool iotHubConnected = false;

static void setup()
{
    iotHubConnected = false;

    // Initialize device model application
    bool provResult = registerDevice(false, certificates);
    if (provResult)
    {      
        if (pnp_device_initialize(connectionString, certificates) == 0)
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
