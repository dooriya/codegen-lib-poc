/*******************************************************************************************
 *
 * PnP device implementation here to:
 *    - get telemetry data from device/sensor
 *    - set read-only property data
 *    - handle read-write property callback
 *    - process device command
 *
 *******************************************************************************************/

#include <string.h>
#include "pocDevice_impl.h"

void pocsensor_get_location(pocsensor_location * location)
{
    double latitude = 20.0f + ((double)rand() / RAND_MAX) * 15.0f;
    double longitude = 10.0f + ((double)rand() / RAND_MAX) * 20.0f;
    location->latitude = latitude;
    location->longitude = longitude;
}

double pocsensor_get_battery_remaining()
{
    double remain = 100.0f + ((double)rand() / RAND_MAX) * 15.0f;
    return remain;
}

bool pocsensor_update_settings(pocsensor_settings *settings)
{
    printf("Set current fanspeed to: %f", settings->fanSpeed);
    printf("Set current voltage to: %f", settings->voltage);

    // do somthing with the new settings ...
    // ...

    return true;
}

bool pocsensor_update_firmware(pocsensor_update_firmware_request *update_firmware_request)
{
    printf("Parameter: firmware_uri = %s", update_firmware_request->firmware_uri);
    printf("Parameter: firmware_version = %d", update_firmware_request->firmware_version);

    // do somthing to process the updateFirmware command ...
    // ...

    printf("Device executed 'updateFirmware' command successfully");
    return true;
}
