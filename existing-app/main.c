// existing-app.cpp : Defines the entry point for the application.
//

#include <stdio.h>
#include "device_capability.h"

int main()
{
    pocsensor_get_location();
    pocsensor_get_battery_remaining();
    pocsensor_update_settings(200, 3);
    pocsensor_update_firmware("http://devkit.firmware.com", 1);
    
    printf("Done!");

    (void)getchar();
    return 0;
}
