/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "nrf.h"
#include "gps.h"
#include "MPU6050.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "oled.h"

/* USER CODE END Includes */
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart6;
extern SPI_HandleTypeDef  hspi1;
extern I2C_HandleTypeDef  hi2c1;


/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
extern GPS_Data_t gps_data;

#define NODE_ID          0      /* 0 = Node A,  1 = Node B             */

#define FRAME_MS         100
#define TX_DURATION_MS    20
#define RX_DURATION_MS    80

#define NODE_A_TX_START   0
#define NODE_A_TX_END     TX_DURATION_MS
#define NODE_B_TX_START   (FRAME_MS / 2)
#define NODE_B_TX_END     (NODE_B_TX_START + TX_DURATION_MS)

#define DBG_INTERVAL_MS   1000

/* ---- LED ---- */
/* Nucleo-F411RE: LD2 = PA5. Adjust if your board differs. */
#define BRAKE_LED_PORT    GPIOA
#define BRAKE_LED_PIN     GPIO_PIN_5

static uint16_t  tx_seq            = 0;
static V2VPacket local_pkt;
static char      dbg[256];
static bool      tx_done_this_slot = false;

static V2VPacket remote_pkt;
static bool      remote_fresh      = false;
static uint32_t  last_rx_tick      = 0;

static uint16_t  pdr_last_seq      = 0;
static uint32_t  pdr_rx_count      = 0;
static uint32_t  pdr_expected      = 0;
static bool      pdr_initialized   = false;
#define PDR_MAX_GAP  20

uint32_t last_oled_ms = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_USART2_UART_Init(void);
void MX_USART6_UART_Init(void);
void MX_SPI1_Init(void);
void MX_I2C1_Init(void);
void Error_Handler(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE BEGIN 0 */
/* ================================================================
   Helper: UART debug print
================================================================ */
static inline void DBG_Print(const char *s)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)s, strlen(s), 200);
}

/* ================================================================
   Helper: LED update — ON if local OR remote brake active
================================================================ */
static void LED_UpdateBrake(void)
{
    bool brake_alert = (local_pkt.brake_flag) ||
                       (remote_fresh && remote_pkt.brake_flag);

    HAL_GPIO_WritePin(BRAKE_LED_PORT, BRAKE_LED_PIN,
                      brake_alert ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* ================================================================
   Dashboard print (1 Hz)
================================================================ */
static void Dashboard_Print(void)
{
    DBG_Print("\r\n========================================\r\n");

    snprintf(dbg, sizeof(dbg),
        " LOCAL  [Node %d]  TX#%-5u\r\n"
        "  GPS : lat=%-12.5f  lon=%-12.5f\r\n"
        "        spd=%.1f km/h   sats=%u   fix=%u\r\n"
        "  IMU : ax=%-7.3f  ay=%-7.3f  az=%-7.3f\r\n"
        "  BRAKE: %s\r\n",
        NODE_ID,
        local_pkt.seq_num,
        (double)(local_pkt.lat_e6    / 1000000.0f),
        (double)(local_pkt.lon_e6    / 1000000.0f),
        (double)(local_pkt.speed_x10 / 10.0f),
        local_pkt.satellites,
        local_pkt.fix_quality,
        (double)local_pkt.accel_x,
        (double)local_pkt.accel_y,
        (double)local_pkt.accel_z,
        local_pkt.brake_flag ? "!! BRAKING !!" : "Normal");
    DBG_Print(dbg);

    DBG_Print("----------------------------------------\r\n");

    if (remote_fresh)
    {
        uint32_t age_ms = HAL_GetTick() - last_rx_tick;
        float    pdr    = (pdr_expected > 0)
                          ? (100.0f * (float)pdr_rx_count / (float)pdr_expected)
                          : 0.0f;
        uint32_t lost   = (pdr_expected > pdr_rx_count)
                          ? (pdr_expected - pdr_rx_count) : 0;

        snprintf(dbg, sizeof(dbg),
            " REMOTE [Node %d]  RX#%-5u  (age: %lu ms)\r\n"
            "  GPS : lat=%-12.5f  lon=%-12.5f\r\n"
            "        spd=%.1f km/h\r\n"
            "  IMU : ax=%-7.3f  ay=%-7.3f  az=%-7.3f\r\n"
            "  BRAKE: %s\r\n"
            "  LINK: PDR=%.1f%%  RX=%lu  Lost=%lu\r\n",
            1 - NODE_ID,
            remote_pkt.seq_num,
            (unsigned long)age_ms,
            (double)(remote_pkt.lat_e6    / 1000000.0f),
            (double)(remote_pkt.lon_e6    / 1000000.0f),
            (double)(remote_pkt.speed_x10 / 10.0f),
            (double)remote_pkt.accel_x,
            (double)remote_pkt.accel_y,
            (double)remote_pkt.accel_z,
            remote_pkt.brake_flag ? "!! BRAKE ALERT !!" : "Normal",
            (double)pdr,
            (unsigned long)pdr_rx_count,
            (unsigned long)lost);
        DBG_Print(dbg);

        if (remote_pkt.brake_flag)
            DBG_Print("  >>> !!! EMERGENCY: REMOTE BRAKE DETECTED !!! <<<\r\n");
    }
    else
    {
        snprintf(dbg, sizeof(dbg),
            " REMOTE [Node %d]  -- No signal yet --\r\n", 1 - NODE_ID);
        DBG_Print(dbg);
    }

    DBG_Print("========================================\r\n");
}
/* ================================================================
   PDR update
================================================================ */
static void PDR_Update(uint16_t seq)
{
    if (!pdr_initialized)
    {
        pdr_last_seq    = seq;
        pdr_initialized = true;
        pdr_rx_count    = 1;
        pdr_expected    = 1;
        return;
    }

    uint16_t gap = (uint16_t)(seq - pdr_last_seq);

    if (gap == 0 || gap > PDR_MAX_GAP)
    {
        pdr_last_seq = seq;
        pdr_rx_count++;
        pdr_expected++;
    }
    else
    {
        pdr_expected += gap;
        pdr_rx_count += 1;
        pdr_last_seq  = seq;
    }
}

/* USER CODE END 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_USART6_UART_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  OLED_Init();
  OLED_ShowBoot();
  DBG_Print("OLED INIT\r\n");
  MPU6050_Initialization();
  GPS_Init(&huart6);
  NRF24_Init();
  NRF24_RX_Mode();

    /* Ensure LED pin is off at start */
    HAL_GPIO_WritePin(BRAKE_LED_PORT, BRAKE_LED_PIN, GPIO_PIN_RESET);

    snprintf(dbg, sizeof(dbg),
        "\r\n=== V2V Transceiver  Node %d  Ready ===\r\n"
        "    Frame: %d ms  |  TX slot: %d ms  |  RX window: %d ms\r\n"
        "    Node A TX: 0-%d ms    Node B TX: %d-%d ms\r\n\r\n",
        NODE_ID, FRAME_MS, TX_DURATION_MS, RX_DURATION_MS,
        NODE_A_TX_END, NODE_B_TX_START, NODE_B_TX_END);
   // DBG_Print(dbg);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

    uint32_t last_mpu_ms = 0;
    uint32_t last_dbg_ms = 0;
    bool     was_in_tx   = false;   /* track TX→RX transition */
  while (1)
  {
    /* USER CODE END WHILE */

	  if(HAL_GetTick() - last_oled_ms >= 200)
	      {
	          last_oled_ms = HAL_GetTick();

	          if(remote_fresh && remote_pkt.brake_flag)
	          {
	              OLED_ShowBrakeAlert();
	          }
	          else
	          {
	              OLED_ShowNormal(
	                  MPU6050.acc_x,
	                  MPU6050.acc_y,
	                  MPU6050.acc_z,
	                  gps_data.speed_kmh,
	                  gps_data.fix_quality
	              );
	          }
	      }
	  GPS_Process();

	  if (HAL_GetTick() - last_mpu_ms >= 5)
	  {
	      last_mpu_ms = HAL_GetTick();

	      MPU6050_Get6AxisRawData(&MPU6050);
	      MPU6050_ProcessData(&MPU6050);
	  }
	  uint32_t now      = HAL_GetTick();
	  uint32_t phase_ms = now % FRAME_MS;

	  bool in_tx_slot;
	  #if (NODE_ID == 0)
	      in_tx_slot = (phase_ms >= NODE_A_TX_START && phase_ms < NODE_A_TX_END);
	  #else
	      in_tx_slot = (phase_ms >= NODE_B_TX_START && phase_ms < NODE_B_TX_END);
	  #endif

	        /* ---- TX ---- */
	        if (in_tx_slot)
	        {
	            /* Switch to TX mode only on slot entry */
	            if (!was_in_tx)
	            {
	                NRF24_TX_Mode();
	                was_in_tx = true;
	            }

	            if (!tx_done_this_slot)
	            {
	                tx_done_this_slot = true;

	                V2VPacket pkt;
	                memset(&pkt, 0, sizeof(pkt));

	                pkt.lat_e6      = (int32_t)(gps_data.latitude  * 1000000.0);
	                pkt.lon_e6      = (int32_t)(gps_data.longitude * 1000000.0);
	                pkt.speed_x10   = (uint16_t)(gps_data.speed_kmh * 10.0);
	                pkt.satellites  = gps_data.satellites;
	                pkt.fix_quality = gps_data.fix_quality;

	                pkt.accel_x    = MPU6050.acc_x;
	                pkt.accel_y    = MPU6050.acc_y;
	                pkt.accel_z    = MPU6050.acc_z;
	                pkt.brake_flag = (MPU6050.acc_x < -0.6f) ? 1 : 0;

	                pkt.seq_num  = tx_seq++;
	                pkt.tx_ts_ms = now;
	                pkt.checksum = NRF24_CalcChecksum((uint8_t*)&pkt,
	                                                   sizeof(V2VPacket) - 1);

	                bool ok = NRF24_Transmit((uint8_t*)&pkt, sizeof(V2VPacket));
	                local_pkt = pkt;

	                snprintf(dbg, sizeof(dbg),
	                    "[TX#%u] phase=%lums  %s  brk=%u\r\n",
	                    pkt.seq_num,
	                    (unsigned long)phase_ms,
	                    ok ? "RF:OK" : "RF:FAIL",
	                    pkt.brake_flag);
	                DBG_Print(dbg);

	                /* Update LED immediately after TX */
	                LED_UpdateBrake();
	            }
	        }
	        else
	        {
	            /* ---- Entered RX window ---- */
	        	if (was_in_tx)
	        	{
	        	    /* Transition: TX slot just ended — switch radio to RX */
	        	    NRF24_RX_Mode();

	        	    DBG_Print("ENTER RX MODE\r\n");

	        	    was_in_tx         = false;
	        	    tx_done_this_slot = false;
	        	}

	            /* Poll for incoming packet */
	            V2VPacket rx_buf;
	            if (NRF24_ReadPacket((uint8_t*)&rx_buf, sizeof(V2VPacket)))
	            {
	            	DBG_Print("PACKET RECEIVED\r\n");
	                uint8_t calc = NRF24_CalcChecksum((uint8_t*)&rx_buf,
	                                                   sizeof(V2VPacket) - 1);
	                if (calc == rx_buf.checksum)
	                {
	                    remote_pkt   = rx_buf;
	                    remote_fresh = true;
	                    last_rx_tick = HAL_GetTick();

	                    PDR_Update(rx_buf.seq_num);
	                    LED_UpdateBrake();   /* update LED on remote brake change */

	                    snprintf(dbg, sizeof(dbg),
	                        "[RX#%u] phase=%lums  brk=%u\r\n",
	                        rx_buf.seq_num,
	                        (unsigned long)phase_ms,
	                        rx_buf.brake_flag);
	                    DBG_Print(dbg);

	                    if (rx_buf.brake_flag)
	                        DBG_Print("  !!! REMOTE BRAKE ALERT !!!\r\n");
	                }
	                else
	                {
	                    snprintf(dbg, sizeof(dbg),
	                        "[RX] BAD CHECKSUM seq=%u  calc=0x%02X  got=0x%02X\r\n",
	                        rx_buf.seq_num, calc, rx_buf.checksum);
	                    DBG_Print(dbg);
	                }
	            }
	        }

	        /* ---- Dashboard at 1 Hz ---- */
	        if (HAL_GetTick() - last_dbg_ms >= DBG_INTERVAL_MS)
	        {
	            last_dbg_ms = HAL_GetTick();
	            Dashboard_Print();
	        }


    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
