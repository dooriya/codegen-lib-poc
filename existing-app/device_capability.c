#include <string.h>
#include "device_capability.h"

void pocsensor_get_location()
{
    // Use random data to mock the sensor data.
    double latitude = 20.0f + ((double)rand() / RAND_MAX) * 15.0f;
    double longitude = 10.0f + ((double)rand() / RAND_MAX) * 20.0f;

    printf("Location latitude: %f, longitude: %f.\r\n", latitude, longitude);
}

void pocsensor_get_battery_remaining()
{
    double remain = 100.0f + ((double)rand() / RAND_MAX) * 15.0f;
    printf("battery_remaing: %f.\r\n", remain);
}

void pocsensor_update_settings(double fan_speed, double voltage)
{
    printf("Set current fan speed to: %f.\r\n", fan_speed);
    printf("Set current voltage to: %f.\r\n", voltage);

    // do somthing with the new settings ...
    // ...
}

bool pocsensor_update_firmware(char* firmware_uri, int firmware_version)
{
    printf("Parameter: firmware_uri = %s\r\n", firmware_uri);
    printf("Parameter: firmware_version = %d\r\n", firmware_version);

    // do somthing to process the updateFirmware command ...
    // ...

    printf("Device executed 'updateFirmware' command successfully\r\n");
    return true;
}
