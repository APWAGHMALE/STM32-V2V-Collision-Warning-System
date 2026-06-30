#ifndef SSD1306_CONF_H
#define SSD1306_CONF_H

/* ── MUST BE FIRST: pull in all HAL types + MCU family macros ── */
#include "stm32f4xx_hal.h"

/* Display size */
#define SSD1306_WIDTH   128
#define SSD1306_HEIGHT   64

/* Use I2C */
/* #define SSD1306_USE_I2C */
#define SSD1306_USE_SPI

/* I2C address: 0x3C (SA0=GND) or 0x3D (SA0=VCC) */
#define SSD1306_I2C_ADDR    (0x3C << 1)

/* I2C handle — must match your CubeMX handle name exactly */
#define SSD1306_I2C_PORT    hi2c1

/* Fonts to include */
#define SSD1306_INCLUDE_FONT_6x8
#define SSD1306_INCLUDE_FONT_7x10
#define SSD1306_INCLUDE_FONT_11x18
#define SSD1306_INCLUDE_FONT_16x26

#endif /* SSD1306_CONF_H */
