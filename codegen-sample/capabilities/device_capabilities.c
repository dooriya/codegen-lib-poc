/*******************************************************************************************
 * PnP device implementation here to:
 *    - get telemetry data from device/sensor
 *    - set read-only property data
 *    - handle read-write property callback
 *    - process device command
 *
 *******************************************************************************************/

#include <string.h>
#include "device_capabilities.h"

void pocsensor_get_location(pocsensor_location * location)
{
    // Fake implementation here.
    double latitude = 20.0f + ((double)rand() / RAND_MAX) * 15.0f;
    double longitude = 10.0f + ((double)rand() / RAND_MAX) * 20.0f;
    printf("Location latitude = %f, longitude = %f\r\n", latitude, longitude);

    location->latitude = latitude;
    location->longitude = longitude;
}

double pocsensor_get_battery_remaining()
{
    // Fake implementaion here.
    double remain = 50.0f + ((double)rand() / RAND_MAX) * 50.0f;
    printf("Battery_remaing: %f\r\n", remain);
    return remain;
}

bool pocsensor_update_settings(pocsensor_settings *settings)
{
    printf("Set current fanspeed to: %f\r\n", settings->fan_speed);
    printf("Set current voltage to: %f\r\n", settings->voltage);

    // do somthing with the new settings ...
    // ...

    return true;
}

bool pocsensor_update_firmware(pocsensor_update_firmware_request *update_firmware_request)
{
    printf("Parameter: firmware_uri = %s\r\n", az_span_ptr(update_firmware_request->firmware_uri));
    printf("Parameter: firmware_version = %d\r\n", update_firmware_request->firmware_version);

    // do somthing to process the updateFirmware command ...
    // ...

    printf("Device executed 'updateFirmware' command successfully");
    return true;
}
