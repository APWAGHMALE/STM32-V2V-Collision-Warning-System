/**
 * @file nrf24.h
 * @brief nRF24L01+ SPI driver for STM32F411RE
 *
 * Pin mapping (SPI1):
 *   CE  → PA4  (GPIO output)
 *   CSN → PA3  (GPIO output)
 *   SCK → PA5  (SPI1_SCK)
 *   MOSI→ PA7  (SPI1_MOSI)
 *   MISO→ PA6  (SPI1_MISO)
 *   VCC → 3.3V (NOT 5V) + 100uF cap across VCC/GND at module pins
 */

#ifndef NRF_H
#define NRF_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* ============================================================
   REGISTER MAP
   ============================================================ */
#define NRF24_REG_CONFIG      0x00
#define NRF24_REG_EN_AA       0x01
#define NRF24_REG_EN_RXADDR   0x02
#define NRF24_REG_SETUP_AW    0x03
#define NRF24_REG_SETUP_RETR  0x04
#define NRF24_REG_RF_CH       0x05
#define NRF24_REG_RF_SETUP    0x06
#define NRF24_REG_STATUS      0x07
#define NRF24_REG_TX_ADDR     0x10
#define NRF24_REG_RX_ADDR_P0  0x0A
#define NRF24_REG_RX_PW_P0    0x11
#define NRF24_REG_FIFO_STATUS 0x17
#define NRF24_REG_DYNPD       0x1C
#define NRF24_REG_FEATURE     0x1D

/* ============================================================
   COMMAND SET
   ============================================================ */
#define NRF24_CMD_R_REGISTER    0x00
#define NRF24_CMD_W_REGISTER    0x20
#define NRF24_CMD_W_TX_PAYLOAD  0xA0
#define NRF24_CMD_FLUSH_TX      0xE1
#define NRF24_CMD_FLUSH_RX      0xE2
#define NRF24_CMD_NOP           0xFF
/* Add these alongside the existing prototypes */
void NRF24_RX_Mode(void);
bool NRF24_ReadPacket(uint8_t *data, uint8_t len);
/* ============================================================
   PAYLOAD SIZE — must match ESP32 side exactly
   ============================================================ */
#define NRF24_PAYLOAD_SIZE      32

/* ============================================================
   V2V TELEMETRY PACKET
   __attribute__((packed)) removes compiler padding so the
   byte layout is identical on STM32 (ARM) and ESP32 (Xtensa).
   ============================================================ */
typedef struct __attribute__((packed)) {
    float    latitude;          /* 4  deg — cast from GPS double        */
    float    longitude;         /* 4  deg                               */
    float    speed_kmh;         /* 4  km/h                              */
    float    accel_x;           /* 4  g   — MPU acc_x (forward axis)    */
    float    accel_y;           /* 4  g   — MPU acc_y (lateral axis)    */
    float    accel_z;           /* 4  g   — MPU acc_z (vertical axis)   */
    float    gyro_z;            /* 4  deg/s — yaw rate                  */
    uint32_t tx_timestamp_ms;   /* 4  HAL_GetTick() at TX               */
    uint16_t seq_num;           /* 2  rolling counter, detects drops    */
    uint8_t  satellites;        /* 1  GPS satellites in use             */
    uint8_t  fix_quality;       /* 1  0=no fix, 1=GPS, 2=DGPS           */
    uint8_t  brake_flag;        /* 1  1 = hard brake (ax < -0.6g)       */
    uint8_t  checksum;          /* 1  XOR of bytes [0..30]              */
    /* total = 4+4+4+4+4+4+4+4+2+1+1+1+1 = 38? — recount below         */
    /* 4*7=28 + 4 + 2 + 1+1+1+1 = 38 → need -6 bytes                   */
    /* Remove gyro_z from float to int16, save 2 bytes, keep 36 → still big */
    /* Recount: float*7=28, uint32=4, uint16=2, uint8*4=4 → 38 bytes    */
    /* Fix: drop gyro_z float, use int16_t (2 bytes), total = 36 → still */
    /* Simplest fix: remove speed_kmh float (use uint16 km/h*10), saves 2 */
    /* Final layout chosen: replace gyro_z float with int16_t gyro_z_ds  */
    /* (degrees/s * 10 as int, range ±3276.7 deg/s — plenty for a car)  */
    /* This gives 28-4+2+4+2+4 = ... let's just count the struct below  */
} V2VPacket_DRAFT;
/* ---- DISCARD ABOVE DRAFT, USE THIS EXACT STRUCT: ---- */

/* Recount cleanly:
   float latitude        4
   float longitude       4
   float speed_kmh       4
   float accel_x         4
   float accel_y         4
   float accel_z         4
   int16_t gyro_z_ds     2  (gyro_z * 10, range ±3276.7 °/s)
   uint32_t tx_ts        4
   uint16_t seq_num      2
   uint8_t satellites    1
   uint8_t fix_quality   1
   uint8_t brake_flag    1
   uint8_t checksum      1
   TOTAL = 4+4+4+4+4+4+2+4+2+1+1+1+1 = 36 bytes  → too big still

   Remove course entirely, encode speed as uint16 (kmh*10, range 0-6553.5):
   float latitude        4
   float longitude       4
   uint16_t speed_x10    2  (km/h * 10)
   float accel_x         4
   float accel_y         4
   float accel_z         4
   int16_t gyro_z_ds     2  (deg/s * 10)
   uint32_t tx_ts        4
   uint16_t seq_num      2
   uint8_t satellites    1
   uint8_t fix_quality   1
   uint8_t brake_flag    1
   uint8_t checksum      1
   uint8_t _pad[2]       2
   TOTAL = 4+4+2+4+4+4+2+4+2+1+1+1+1+2 = 36 → still 4 over

   FINAL: keep float speed, drop gyro (add later when expanding to 64-byte):
   float latitude        4
   float longitude       4
   float speed_kmh       4
   float accel_x         4
   float accel_y         4
   float accel_z         4
   uint32_t tx_ts        4
   uint16_t seq_num      2
   uint8_t satellites    1
   uint8_t fix_quality   1
   uint8_t brake_flag    1
   uint8_t checksum      1
   uint8_t _pad[2]       2
   TOTAL = 4+4+4+4+4+4+4+2+1+1+1+1+2 = 36 → STILL 4 over 32

   Root cause: 6 floats * 4 = 24, uint32 = 4, total 28 for sensor data alone.
   With seq(2)+sats(1)+fix(1)+brk(1)+cs(1) = 6 → 28+4+6 = 38 minimum.

   Resolution: use 64-byte payload (nRF24 max is 32 bytes per packet).
   We split into two packets OR compress GPS to int32.

   CHOSEN SOLUTION: GPS as int32 (lat*1e6, lon*1e6), speed as uint16.
   This fits everything in 32 bytes AND improves GPS precision (int32 lat*1e6 = ~0.11m resolution):

   int32_t  lat_e6       4  (latitude  * 1,000,000)
   int32_t  lon_e6       4  (longitude * 1,000,000)
   uint16_t speed_x10    2  (km/h * 10, 0–6553.5 km/h)
   int16_t  gyro_z_x10   2  (deg/s * 10)
   float    accel_x      4
   float    accel_y      4
   float    accel_z      4
   uint32_t tx_ts_ms     4
   uint16_t seq_num      2
   uint8_t  satellites   1
   uint8_t  fix_quality  1
   uint8_t  brake_flag   1
   uint8_t  checksum     1
   TOTAL = 4+4+2+2+4+4+4+4+2+1+1+1+1 = 34 → 2 bytes over

   Remove gyro for now (can add in v2 with 2-packet scheme):
   int32_t  lat_e6       4
   int32_t  lon_e6       4
   uint16_t speed_x10    2
   float    accel_x      4
   float    accel_y      4
   float    accel_z      4
   uint32_t tx_ts_ms     4
   uint16_t seq_num      2
   uint8_t  satellites   1
   uint8_t  fix_quality  1
   uint8_t  brake_flag   1
   uint8_t  checksum     1
   TOTAL = 4+4+2+4+4+4+4+2+1+1+1+1 = 32  ✓ EXACT FIT
*/

typedef struct __attribute__((packed)) {
    int32_t  lat_e6;        /* 4  latitude  × 1,000,000  e.g. 18.521 → 18521000  */
    int32_t  lon_e6;        /* 4  longitude × 1,000,000  e.g. 73.856 → 73856000  */
    uint16_t speed_x10;     /* 2  speed km/h × 10        e.g. 60.5 → 605         */
    float    accel_x;       /* 4  g  (forward, brake axis)                        */
    float    accel_y;       /* 4  g  (lateral)                                    */
    float    accel_z;       /* 4  g  (vertical)                                   */
    uint32_t tx_ts_ms;      /* 4  HAL_GetTick() at transmit time                  */
    uint16_t seq_num;       /* 2  rolling packet counter                          */
    uint8_t  satellites;    /* 1  GPS sats in use                                 */
    uint8_t  fix_quality;   /* 1  0=none 1=GPS 2=DGPS                             */
    uint8_t  brake_flag;    /* 1  1 = hard brake detected (accel_x < -0.6g)       */
    uint8_t  checksum;      /* 1  XOR over bytes [0..30]                          */
    /* TOTAL = 32 bytes exactly                                                    */
} V2VPacket;

_Static_assert(sizeof(V2VPacket) == 32, "V2VPacket must be exactly 32 bytes");

/* ============================================================
   PUBLIC API
   ============================================================ */
void    NRF24_Init(void);
void    NRF24_TX_Mode(void);
bool    NRF24_Transmit(uint8_t *data, uint8_t len);
uint8_t NRF24_CalcChecksum(uint8_t *data, uint8_t len);

/* Decode helpers (used on ESP32 side too — mirrored there in .ino) */
static inline float V2V_DecodeLat(int32_t lat_e6) { return lat_e6 / 1000000.0f; }
static inline float V2V_DecodeLon(int32_t lon_e6) { return lon_e6 / 1000000.0f; }
static inline float V2V_DecodeSpeed(uint16_t speed_x10) { return speed_x10 / 10.0f; }

#endif /* NRF24_H */
