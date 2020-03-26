#ifndef DPS_PROV_H
#define DPS_PROV_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define IOT_HUB_CONN_STR_MAX_LEN 512
    extern char connectionString[];

bool registerDevice(bool traceOn, const char * certificates);

#ifdef __cplusplus
}
#endif
#endif // DPS_PROV_H