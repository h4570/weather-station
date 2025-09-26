#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        float temperature;
        float humidity;
        int32_t pressure;
        int32_t bat_in;
    } station_data;

#ifdef __cplusplus
}
#endif
