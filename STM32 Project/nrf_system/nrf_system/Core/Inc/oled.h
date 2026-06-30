#ifndef OLED_H
#define OLED_H

#include "stm32f4xx_hal.h"
#include "ssd1306.h"        /* pulls ssd1306_conf.h → hi2c1, fonts */
#include "ssd1306_fonts.h"  /* FontDef, Font_7x10, Font_11x18      */
#include <stdint.h>

void OLED_Init(void);
void OLED_ShowNormal(float ax,
                     float ay,
                     float az,
                     float speed,
                     uint8_t gps_fix);
void OLED_ShowBrakeAlert(void);
void OLED_ShowBlindSpot(uint8_t side);
void OLED_ShowFallback(void);
void OLED_ShowBoot(void);

#endif /* OLED_H */
