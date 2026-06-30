/**
 * @file gps.c
 * @brief GPS NEO-6M USART6 Driver
 *
 * Interrupt-driven, single-byte circular buffer.
 * Call GPS_Process() every loop iteration to drain the buffer.
 *
 * IMPORTANT: HAL_UART_RxCpltCallback is defined at the bottom.
 * If you already have this callback in main.c (e.g. for SiK radio
 * on USART1), move the USART6 branch into that existing function
 * and DELETE the one here to avoid a duplicate symbol linker error.
 */

#include "gps.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ============================================================
   PRIVATE FUNCTION PROTOTYPES
   ============================================================ */
static void   GPS_ParseGPRMC(char *sentence);
static void   GPS_ParseGPGGA(char *sentence);
static double GPS_ConvertLatitude(char *lat_str);
static double GPS_ConvertLongitude(char *lon_str);

/* ============================================================
   CIRCULAR BUFFER
   gps_rx_buffer[gps_rx_head] is the slot HAL writes the NEXT byte into.
   GPS_Process() reads from gps_rx_tail up to (but not including) gps_rx_head.
   ============================================================ */
#define GPS_RX_BUFFER_SIZE 512

static uint8_t  gps_rx_buffer[GPS_RX_BUFFER_SIZE];
static volatile uint16_t gps_rx_head = 0;   /* written in ISR */
static uint16_t gps_rx_tail = 0;            /* read in GPS_Process */

/* ============================================================
   NMEA SENTENCE BUFFER
   ============================================================ */
#define NMEA_MAX_LENGTH 128

static char     nmea_buffer[NMEA_MAX_LENGTH];
static uint16_t nmea_index = 0;

/* ============================================================
   GLOBAL GPS DATA  (extern declared in gps.h)
   ============================================================ */
GPS_Data_t gps_data = {
    .latitude    = 0.0,
    .longitude   = 0.0,
    .altitude    = 0.0,
    .speed_kmh   = 0.0,
    .course      = 0.0,
    .satellites  = 0,
    .hdop        = 99.99,
    .fix_quality = 0,
    .fix_type    = NO_FIX,
    .timestamp   = 0
};

/* ============================================================
   STATISTICS
   ============================================================ */
static uint32_t gps_update_count = 0;
static uint32_t gps_parse_errors = 0;

/* UART handle — set in GPS_Init, used in callback to re-arm */
static UART_HandleTypeDef *gps_uart = NULL;

/* ============================================================
   PUBLIC FUNCTIONS
   ============================================================ */

/**
 * @brief Initialise GPS.
 * @param huart  Pointer to USART6 handle (pass &huart6 from main.c).
 *
 * Arms the first single-byte interrupt receive.
 * Subsequent bytes are re-armed automatically in HAL_UART_RxCpltCallback.
 */
void GPS_Init(UART_HandleTypeDef *huart)
{
    if (huart == NULL) return;

    gps_uart    = huart;
    gps_rx_head = 0;
    gps_rx_tail = 0;
    nmea_index  = 0;
    gps_update_count = 0;
    gps_parse_errors = 0;

    /* Arm first byte into slot [0] */
    HAL_UART_Receive_IT(huart, &gps_rx_buffer[0], 1);
}

/**
 * @brief Drain circular buffer and parse any complete NMEA sentences.
 * @return 1 if a new GPRMC fix was parsed this call, 0 otherwise.
 *
 * Call this every main loop iteration (or at least every 20ms).
 */
int GPS_Process(void)
{
    int new_fix = 0;

    while (gps_rx_tail != gps_rx_head)
    {
        uint8_t byte = gps_rx_buffer[gps_rx_tail];
        gps_rx_tail  = (gps_rx_tail + 1) % GPS_RX_BUFFER_SIZE;

        if (byte == '$')
        {
            /* Start of sentence — reset working buffer */
            nmea_index = 0;
            nmea_buffer[nmea_index++] = (char)byte;
        }
        else if (byte == '\n')
        {
            /* End of sentence — null-terminate and parse */
            if (nmea_index > 6)
            {
                nmea_buffer[nmea_index] = '\0';

                if (strncmp(nmea_buffer, "$GPRMC", 6) == 0)
                {
                    GPS_ParseGPRMC(nmea_buffer);
                    gps_data.timestamp = HAL_GetTick();
                    gps_update_count++;
                    new_fix = 1;
                }
                else if (strncmp(nmea_buffer, "$GPGGA", 6) == 0)
                {
                    GPS_ParseGPGGA(nmea_buffer);
                    gps_data.timestamp = HAL_GetTick();
                }
            }
            nmea_index = 0;
        }
        else if (byte != '\r' && nmea_index < NMEA_MAX_LENGTH - 1)
        {
            nmea_buffer[nmea_index++] = (char)byte;
        }
    }

    return new_fix;
}

GPS_Data_t GPS_GetData(void)        { return gps_data; }
int        GPS_HasFix(void)         { return (gps_data.fix_quality && gps_data.satellites >= 4); }
uint32_t   GPS_GetUpdateAge(void)   { return HAL_GetTick() - gps_data.timestamp; }
uint32_t   GPS_GetUpdateCount(void) { return gps_update_count; }
uint32_t   GPS_GetParseErrors(void) { return gps_parse_errors; }
void       GPS_ResetStats(void)     { gps_update_count = 0; gps_parse_errors = 0; }

/* ============================================================
   PRIVATE PARSE FUNCTIONS
   ============================================================ */

/**
 * GPRMC: $GPRMC,hhmmss.ss,A/V,ddmm.mmmm,N/S,dddmm.mmmm,E/W,spd,crs,ddmmyy,...
 * Fields: 0=$GPRMC 1=time 2=status 3=lat 4=lat_dir 5=lon 6=lon_dir 7=speed 8=course
 */
static void GPS_ParseGPRMC(char *sentence)
{
    if (sentence == NULL || strlen(sentence) < 20) { gps_parse_errors++; return; }

    char  temp[NMEA_MAX_LENGTH];
    strncpy(temp, sentence, NMEA_MAX_LENGTH - 1);
    temp[NMEA_MAX_LENGTH - 1] = '\0';

    char *ptr   = NULL;
    char *token = strtok_r(temp, ",", &ptr);
    int   field = 0;

    while (token != NULL && field <= 9)
    {
        switch (field)
        {
            case 2:  /* A = active/valid, V = void */
                gps_data.fix_quality = (token[0] == 'A') ? 1 : 0;
                break;

            case 3:  /* Latitude DDMM.MMMM */
                if (gps_data.fix_quality && strlen(token) > 0)
                    gps_data.latitude = GPS_ConvertLatitude(token);
                break;

            case 4:  /* N or S */
                if (token[0] == 'S')
                    gps_data.latitude = -gps_data.latitude;
                break;

            case 5:  /* Longitude DDDMM.MMMM */
                if (gps_data.fix_quality && strlen(token) > 0)
                    gps_data.longitude = GPS_ConvertLongitude(token);
                break;

            case 6:  /* E or W */
                if (token[0] == 'W')
                    gps_data.longitude = -gps_data.longitude;
                break;

            case 7:  /* Speed in knots → convert to km/h */
                if (strlen(token) > 0)
                    gps_data.speed_kmh = atof(token) * 1.852;
                break;

            case 8:  /* Course over ground, degrees true */
                if (strlen(token) > 0)
                    gps_data.course = atof(token);
                break;

            default: break;
        }

        token = strtok_r(NULL, ",", &ptr);
        field++;
    }
}

/**
 * GPGGA: $GPGGA,time,lat,N/S,lon,E/W,fix,sats,hdop,alt,M,...
 * Fields: 0=$GPGGA 1=time 2=lat 3=dir 4=lon 5=dir 6=fix 7=sats 8=hdop 9=alt
 */
static void GPS_ParseGPGGA(char *sentence)
{
    if (sentence == NULL || strlen(sentence) < 20) { gps_parse_errors++; return; }

    char  temp[NMEA_MAX_LENGTH];
    strncpy(temp, sentence, NMEA_MAX_LENGTH - 1);
    temp[NMEA_MAX_LENGTH - 1] = '\0';

    char *ptr   = NULL;
    char *token = strtok_r(temp, ",", &ptr);
    int   field = 0;

    while (token != NULL && field <= 9)
    {
        switch (field)
        {
            case 6:
                gps_data.fix_type = (GPS_FixType_t)atoi(token);
                break;
            case 7:
                if (strlen(token) > 0)
                    gps_data.satellites = (uint8_t)atoi(token);
                break;
            case 8:
                if (strlen(token) > 0)
                    gps_data.hdop = atof(token);
                break;
            case 9:
                if (strlen(token) > 0)
                    gps_data.altitude = atof(token);
                break;
            default: break;
        }

        token = strtok_r(NULL, ",", &ptr);
        field++;
    }
}

/* NMEA DDMM.MMMM → decimal degrees */
static double GPS_ConvertLatitude(char *lat_str)
{
    if (!lat_str || strlen(lat_str) < 5) return 0.0;
    double  raw     = atof(lat_str);
    int     degrees = (int)(raw / 100);
    double  minutes = raw - (degrees * 100.0);
    return degrees + (minutes / 60.0);
}

static double GPS_ConvertLongitude(char *lon_str)
{
    if (!lon_str || strlen(lon_str) < 6) return 0.0;
    double  raw     = atof(lon_str);
    int     degrees = (int)(raw / 100);
    double  minutes = raw - (degrees * 100.0);
    return degrees + (minutes / 60.0);
}

/* ============================================================
   HAL UART RX COMPLETE CALLBACK
   ============================================================
   IMPORTANT: If main.c already defines HAL_UART_RxCpltCallback
   (e.g. for SiK radio on USART1), DO NOT include this function.
   Instead copy the USART6 branch below into your existing callback.
   ============================================================ */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART6)
    {
        /* Advance head — the byte HAL just wrote is at [gps_rx_head] */
        uint16_t next_head = (gps_rx_head + 1) % GPS_RX_BUFFER_SIZE;

        /* Only advance if buffer not full (protect against overflow) */
        if (next_head != gps_rx_tail)
            gps_rx_head = next_head;

        /* Re-arm: receive next byte into new head slot */
        if (gps_uart != NULL)
            HAL_UART_Receive_IT(gps_uart, &gps_rx_buffer[gps_rx_head], 1);
    }

    /* ---- Add other UART instances here if needed ---- */
    /* if (huart->Instance == USART1) { ... SiK radio RX ... } */
}
