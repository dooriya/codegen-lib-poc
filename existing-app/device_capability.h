#ifndef DEVICE_CAPABILITY_H
#define DEVICE_CAPABILITY_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Get location data from sensor.
void pocsensor_get_location();

// Get remaining battery of the device.
void pocsensor_get_battery_remaining();

// Update settings for the device
void pocsensor_update_settings(double fanSpeed, double voltage);

// Perform update firmware operation.
bool pocsensor_update_firmware(char * firmware_uri, int firmware_version);

#ifdef __cplusplus
}
#endif

#endif // DEVICE_CAPABILITY_H
