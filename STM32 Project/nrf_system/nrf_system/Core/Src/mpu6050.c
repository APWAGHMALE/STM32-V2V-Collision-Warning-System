/**
 * @file MPU6050.c
 * @brief MPU-6050 I2C Driver
 *
 * Configuration:
 *   Sample rate : 200 Hz  (SMPRT_DIV = 39, gyro output rate 8kHz / (1+39))
 *   Accel range : ±2g     (LSB = 16384)
 *   Gyro range  : ±250°/s (LSB = 131)
 *   DLPF        : off     (CONFIG = 0x00)
 */

#include "MPU6050.h"
#include "main.h"
#include <stdio.h>

extern I2C_HandleTypeDef hi2c1;

Struct_MPU6050 MPU6050;

static const float LSB_ACC  = 16384.0f;
static const float LSB_GYRO = 131.0f;

/* ============================================================
   INIT
   ============================================================ */
void MPU6050_Initialization(void)
{
    uint8_t check;
    uint8_t data;

    /* 1. Wake the device (clear SLEEP bit) */
    data = 0x00;
    HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, MPU6050_PWR_MGMT_1,
                      I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
    HAL_Delay(100);

    /* 2. Verify WHO_AM_I = 0x68 */
    HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, MPU6050_WHO_AM_I,
                     I2C_MEMADD_SIZE_8BIT, &check, 1, 100);

    if (check != 0x68)
    {
        printf("MPU6050: NOT FOUND (WHO_AM_I=0x%02X, expected 0x68)\r\n", check);
        return;
    }

    printf("MPU6050: Found OK\r\n");

    /* 3. Device reset */
    data = 0x80;
    HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, MPU6050_PWR_MGMT_1,
                      I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
    HAL_Delay(100);

    /* 4. Wake again after reset */
    data = 0x00;
    HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, MPU6050_PWR_MGMT_1,
                      I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
    HAL_Delay(50);

    /* 5. Sample rate = gyro_rate / (1 + SMPLRT_DIV)
          = 8000 / (1 + 39) = 200 Hz */
    data = 39;
    HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, MPU6050_SMPRT_DIV,
                      I2C_MEMADD_SIZE_8BIT, &data, 1, 100);

    /* 6. DLPF disabled (raw bandwidth) */
    data = 0x00;
    HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, MPU6050_CONFIG,
                      I2C_MEMADD_SIZE_8BIT, &data, 1, 100);

    /* 7. Gyro full scale ±250 deg/s */
    data = 0x00;
    HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, MPU6050_GYRO_CONFIG,
                      I2C_MEMADD_SIZE_8BIT, &data, 1, 100);

    /* 8. Accel full scale ±2g */
    data = 0x00;
    HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, MPU6050_ACCEL_CONFIG,
                      I2C_MEMADD_SIZE_8BIT, &data, 1, 100);

    printf("MPU6050: Init complete — 200Hz, ±2g, ±250deg/s\r\n");
}

/* ============================================================
   RAW READ  (14 bytes: AX AY AZ TEMP GX GY GZ, each 2 bytes big-endian)
   ============================================================ */
void MPU6050_Get6AxisRawData(Struct_MPU6050 *mpu)
{
    uint8_t data[14];

    if (HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, MPU6050_ACCEL_XOUT_H,
                         I2C_MEMADD_SIZE_8BIT, data, 14, 100) != HAL_OK)
    {
        return;  /* I2C error — keep last values */
    }

    mpu->acc_x_raw  = (int16_t)((data[0]  << 8) | data[1]);
    mpu->acc_y_raw  = (int16_t)((data[2]  << 8) | data[3]);
    mpu->acc_z_raw  = (int16_t)((data[4]  << 8) | data[5]);

    mpu->temperature_raw = (int16_t)((data[6]  << 8) | data[7]);

    mpu->gyro_x_raw = (int16_t)((data[8]  << 8) | data[9]);
    mpu->gyro_y_raw = (int16_t)((data[10] << 8) | data[11]);
    mpu->gyro_z_raw = (int16_t)((data[12] << 8) | data[13]);
}

/* ============================================================
   CONVERT raw ADC → physical units
   ============================================================ */
void MPU6050_ProcessData(Struct_MPU6050 *mpu)
{
    mpu->acc_x  = mpu->acc_x_raw  / LSB_ACC;
    mpu->acc_y  = mpu->acc_y_raw  / LSB_ACC;
    mpu->acc_z  = mpu->acc_z_raw  / LSB_ACC;

    mpu->gyro_x = mpu->gyro_x_raw / LSB_GYRO;
    mpu->gyro_y = mpu->gyro_y_raw / LSB_GYRO;
    mpu->gyro_z = mpu->gyro_z_raw / LSB_GYRO;

    /* Datasheet: Temp_degC = raw / 340 + 36.53 */
    mpu->temperature = (float)mpu->temperature_raw / 340.0f + 36.53f;
}
