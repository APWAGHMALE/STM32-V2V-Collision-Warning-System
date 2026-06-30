/**
 * @file gps.h
 * @brief GPS NEO-6M USART6 Driver Header
 *        USART6: PC6=TX, PC7=RX, 9600 baud
 */

#ifndef GPS_H
#define GPS_H

#include "main.h"
#include <stdint.h>

/* ============================================================
   GPS FIX TYPE
   ============================================================ */
typedef enum {
    NO_FIX      = 0,
    GPS_FIX     = 1,
    DGPS_FIX    = 2,
    PPS_FIX     = 3,
    RTK_FIX     = 4,
    RTK_FLOAT   = 5,
    ESTIMATED   = 6,
    MANUAL_MODE = 7,
    SIM_MODE    = 8
} GPS_FixType_t;

/* ============================================================
   GPS DATA STRUCTURE
   double used internally for full NMEA precision;
   cast to float when packing into V2VPacket.
   ============================================================ */
typedef struct {
    double         latitude;     /* decimal degrees, negative = South */
    double         longitude;    /* decimal degrees, negative = West  */
    double         altitude;     /* metres above sea level            */
    double         speed_kmh;    /* km/h converted from knots         */
    double         course;       /* degrees true north                */
    uint8_t        satellites;   /* satellites used in fix            */
    double         hdop;         /* horizontal dilution of precision  */
    uint8_t        fix_quality;  /* 0=none 1=GPS 2=DGPS               */
    GPS_FixType_t  fix_type;
    uint32_t       timestamp;    /* HAL_GetTick() of last update      */
} GPS_Data_t;

/* ============================================================
   GLOBAL — accessible directly from main.c via extern
   ============================================================ */
extern GPS_Data_t gps_data;

/* ============================================================
   PUBLIC API
   ============================================================ */
void     GPS_Init(UART_HandleTypeDef *huart);  /* pass &huart6          */
int      GPS_Process(void);                    /* call in main loop      */
GPS_Data_t GPS_GetData(void);
int      GPS_HasFix(void);
uint32_t GPS_GetUpdateAge(void);
uint32_t GPS_GetUpdateCount(void);
uint32_t GPS_GetParseErrors(void);
void     GPS_ResetStats(void);

#endif /* GPS_H */
