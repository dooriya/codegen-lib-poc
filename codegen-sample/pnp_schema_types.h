/************************************************************************************************
 * This code was automatically generated by Digital Twin Code Generator tool 0.6.8.
 * Changes to this file may cause incorrect behavior and will be lost if the code is regenerated.
 *
 * Generated Date: 3/23/2020
 ***********************************************************************************************/

#ifndef PNP_SCHEMA_TYPES_H
#define PNP_SCHEMA_TYPES_H

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @file pnp_schema_types.h
 * @brief PnP Schema Type Definition.
 */

/*------------------------ Type Definition for Telemetry Schema ----------------------------*/
/**
 * @brief Data type definition of the location telemetry.
 */
typedef struct pocinterface_location_tag
{
    double latitude;    /* distance from equator */
    double longitude;   /* istance from meridian */
} pocinterface_location;

/*------------------------ Type Definition for Property Schema ----------------------------*/
/**
 * @brief Data type definition of the settings property.
 */
typedef struct pocinterface_settings_tag
{
    double fanSpeed;    /* target fan speed to set */
    double voltage;     /* target voltage to set */

} pocinterface_settings;

/*---------------- Type Definition for Command Request and Response Schema -----------------*/
/**
 * @brief Data type definition of the updateFirmware command request.
 */
typedef struct pocinterface_updatefirmware_request_tag
{
    char* firmwareUri;      /* the download uri for the new firmware */
    int firmwareVersion;    /* the new firmware version */
} pocinterface_updatefirmware_request;

#ifdef __cplusplus
}
#endif

#endif // PNP_SCHEMA_TYPES_H
