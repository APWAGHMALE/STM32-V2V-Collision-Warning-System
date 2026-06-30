/* oled.c — OLED screen renderer */

#include "oled.h"
#include "gps.h"
#include <stdio.h>

extern GPS_Data_t gps_data;

void OLED_Init(void) {
    ssd1306_Init();
    OLED_ShowBoot();
}

void OLED_ShowBoot(void) {
    ssd1306_Fill(Black);
    ssd1306_SetCursor(10, 10);
    ssd1306_WriteString("V2V SYSTEM", Font_11x18, White);
    ssd1306_SetCursor(18, 35);
    ssd1306_WriteString("Initialising..", Font_7x10, White);
    ssd1306_UpdateScreen();
}

void OLED_ShowNormal(float ax,
                     float ay,
                     float az,
                     float speed,
                     uint8_t gps_fix)
{
    char buf[32];

    ssd1306_Fill(Black);

    sprintf(buf,"LAT:%.4f", gps_data.latitude);
    ssd1306_SetCursor(6,0);
    ssd1306_WriteString(buf, Font_6x8, White);

    sprintf(buf,"LON:%.4f", gps_data.longitude);
    ssd1306_SetCursor(6,8);
    ssd1306_WriteString(buf, Font_6x8, White);

    sprintf(buf,"SPD:%3.1f", speed);
    ssd1306_SetCursor(6,16);
    ssd1306_WriteString(buf, Font_6x8, White);

    sprintf(buf,"AX:%1.2f", ax);
    ssd1306_SetCursor(6,24);
    ssd1306_WriteString(buf, Font_6x8, White);

    sprintf(buf,"AY:%1.2f", ay);
    ssd1306_SetCursor(6,32);
    ssd1306_WriteString(buf, Font_6x8, White);

    sprintf(buf,"AZ:%1.2f", az);
    ssd1306_SetCursor(6,40);
    ssd1306_WriteString(buf, Font_6x8, White);

    sprintf(buf,"GPS:%u SAT:%u",
            gps_fix,
            gps_data.satellites);

    ssd1306_SetCursor(6,52);
    ssd1306_WriteString(buf, Font_6x8, White);

    ssd1306_UpdateScreen();
}

void OLED_ShowBrakeAlert(void)
{
    ssd1306_Fill(Black);

    ssd1306_SetCursor(6,0);
    ssd1306_WriteString("!!! WARNING !!!",
                        Font_7x10,
                        White);

    ssd1306_SetCursor(6,20);
    ssd1306_WriteString("REMOTE VEHICLE",
                        Font_11x18,
                        White);

    ssd1306_SetCursor(6,42);
    ssd1306_WriteString("BRAKING",
                        Font_11x18,
                        White);

    ssd1306_UpdateScreen();
}

void OLED_ShowBlindSpot(uint8_t side) {
    ssd1306_Fill(Black);
    ssd1306_SetCursor(6, 10);
    ssd1306_WriteString("BLIND SPOT", Font_11x18, White);

    ssd1306_SetCursor(20, 42);
    if (side == 1)
        ssd1306_WriteString("<<< LEFT", Font_7x10, White);
    else
        ssd1306_WriteString("RIGHT >>>", Font_7x10, White);

    ssd1306_UpdateScreen();
}

void OLED_ShowFallback(void) {
    ssd1306_Fill(Black);
    ssd1306_SetCursor(6, 20);
    ssd1306_WriteString("SiK LOST", Font_11x18, White);
    ssd1306_SetCursor(6, 46);
    ssd1306_WriteString("Using ZigBee", Font_7x10, White);
    ssd1306_UpdateScreen();
}
