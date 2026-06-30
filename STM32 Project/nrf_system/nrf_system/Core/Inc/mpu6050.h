/**
 * @file MPU6050.h
 * @brief MPU-6050 I2C Driver Header
 *        I2C1: PB8=SCL, PB9=SDA, 400kHz
 *        I2C address: 0x68 (AD0 tied low)
 */

#ifndef __MPU6050_H
#define __MPU6050_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

/* ============================================================
   I2C ADDRESS  (AD0=0 → 0x68, shifted left 1 for HAL)
   ============================================================ */
#define MPU6050_ADDR         (0x68 << 1)

/* ============================================================
   REGISTER MAP
   ============================================================ */
#define MPU6050_WHO_AM_I     0x75
#define MPU6050_PWR_MGMT_1   0x6B
#define MPU6050_SMPRT_DIV    0x19
#define MPU6050_CONFIG       0x1A
#define MPU6050_GYRO_CONFIG  0x1B
#define MPU6050_ACCEL_CONFIG 0x1C
#define MPU6050_ACCEL_XOUT_H 0x3B   /* 14 bytes: AX AY AZ TEMP GX GY GZ */

/* ============================================================
   DATA STRUCTURE
   ============================================================ */
typedef struct {
    /* Raw 16-bit ADC values */
    int16_t acc_x_raw;
    int16_t acc_y_raw;
    int16_t acc_z_raw;
    int16_t gyro_x_raw;
    int16_t gyro_y_raw;
    int16_t gyro_z_raw;
    int16_t temperature_raw;

    /* Converted physical units */
    float acc_x;        /* g  (±2g range, 16384 LSB/g)         */
    float acc_y;        /* g                                    */
    float acc_z;        /* g                                    */
    float gyro_x;       /* deg/s (±250 deg/s, 131 LSB/deg/s)   */
    float gyro_y;       /* deg/s                                */
    float gyro_z;       /* deg/s                                */
    float temperature;  /* °C                                   */
} Struct_MPU6050;

/* ============================================================
   GLOBAL INSTANCE  (defined in MPU6050.c)
   ============================================================ */
extern Struct_MPU6050 MPU6050;

/* ============================================================
   PUBLIC API
   ============================================================ */
void MPU6050_Initialization(void);
void MPU6050_Get6AxisRawData(Struct_MPU6050 *mpu);
void MPU6050_ProcessData(Struct_MPU6050 *mpu);

#endif /* __MPU6050_H */
