/**
 * @file nrf24.c
 * @brief nRF24L01+ SPI driver for STM32F411RE
 *
 * Hardware:
 *   SPI1 — Full-Duplex Master, CPOL=0, CPHA=0, 8-bit, prescaler for ~4 MHz
 *   CE  → PA4  GPIO_Output
 *   CSN → PA3  GPIO_Output  (manual CS, not SPI1_NSS)
 *   SCK → PA5  SPI1_SCK
 *   MOSI→ PA7  SPI1_MOSI
 *   MISO→ PA6  SPI1_MISO
 *
 * CubeMX: enable SPI1, set PA3 and PA4 as GPIO_Output push-pull no pull.
 */

#include "nrf.h"
#include <string.h>

/* ---- HAL handle declared in main.c ---- */
extern SPI_HandleTypeDef hspi1;

/* ---- Pipe address — must match ESP32 exactly ---- */
static const uint8_t PIPE_ADDR[5] = {0xAB, 0xCD, 0xEF, 0x12, 0x34};

/* ============================================================
   PIN MACROS
   ============================================================ */
#define CE_LOW()   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET)
#define CE_HIGH()  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET)
#define CSN_LOW()  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET)
#define CSN_HIGH() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET)

/* ============================================================
   LOW-LEVEL SPI HELPERS
   ============================================================ */

static void NRF24_WriteReg(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { (uint8_t)(NRF24_CMD_W_REGISTER | (reg & 0x1F)), val };
    CSN_LOW();
    HAL_SPI_Transmit(&hspi1, buf, 2, 10);
    CSN_HIGH();
}

static uint8_t NRF24_ReadReg(uint8_t reg)
{
    uint8_t tx = (uint8_t)(NRF24_CMD_R_REGISTER | (reg & 0x1F));
    uint8_t rx = 0;
    CSN_LOW();
    HAL_SPI_Transmit(&hspi1, &tx, 1, 10);
    HAL_SPI_Receive(&hspi1, &rx, 1, 10);
    CSN_HIGH();
    return rx;
}

static void NRF24_WriteRegMulti(uint8_t reg, const uint8_t *data, uint8_t len)
{
    uint8_t cmd = (uint8_t)(NRF24_CMD_W_REGISTER | (reg & 0x1F));
    CSN_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 10);
    HAL_SPI_Transmit(&hspi1, (uint8_t *)data, len, 50);
    CSN_HIGH();
}

static void NRF24_FlushTX(void)
{
    uint8_t cmd = NRF24_CMD_FLUSH_TX;
    CSN_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 10);
    CSN_HIGH();
}

/* ============================================================
   PUBLIC API
   ============================================================ */

/**
 * @brief Initialise nRF24L01+ as a transmitter.
 *        Call once after SPI1 and GPIO are initialised by CubeMX.
 */
void NRF24_Init(void)
{
    CE_LOW();
    CSN_HIGH();
    HAL_Delay(5);   /* power-on reset time */

    /* Power down to configure cleanly */
    NRF24_WriteReg(NRF24_REG_CONFIG, 0x00);
    HAL_Delay(2);

    /* Disable auto-ACK on all pipes (broadcast mode, no ACK needed) */
    NRF24_WriteReg(NRF24_REG_EN_AA, 0x00);

    /* Enable data pipe 0 for RX address match */
    NRF24_WriteReg(NRF24_REG_EN_RXADDR, 0x01);

    /* 5-byte address width */
    NRF24_WriteReg(NRF24_REG_SETUP_AW, 0x03);

    /* No retransmit (no auto-ACK so this is irrelevant, but set clean) */
    NRF24_WriteReg(NRF24_REG_SETUP_RETR, 0x00);

    /* RF channel 76 = 2.476 GHz — above WiFi ch1/6/11 (2.412–2.462 GHz) */
    NRF24_WriteReg(NRF24_REG_RF_CH, 0x4C);

    /* 1 Mbps data rate, 0 dBm TX power */
    NRF24_WriteReg(NRF24_REG_RF_SETUP, 0x06);

    /* Static payload length */
    NRF24_WriteReg(NRF24_REG_DYNPD,   0x00);
    NRF24_WriteReg(NRF24_REG_FEATURE, 0x00);

    /* TX address */
    NRF24_WriteRegMulti(NRF24_REG_TX_ADDR, PIPE_ADDR, 5);

    /* RX pipe 0 address = TX address (required even in TX-only mode) */
    NRF24_WriteRegMulti(NRF24_REG_RX_ADDR_P0, PIPE_ADDR, 5);

    /* RX payload width = 32 bytes */
    NRF24_WriteReg(NRF24_REG_RX_PW_P0, NRF24_PAYLOAD_SIZE);

    /* Clear any stale status flags */
    NRF24_WriteReg(NRF24_REG_STATUS, 0x70);

    NRF24_FlushTX();
}

/**
 * @brief Switch to TX mode (PWR_UP=1, PRIM_RX=0).
 *        Call after NRF24_Init().
 */
void NRF24_TX_Mode(void)
{
    CE_LOW();
    /* Bits: MASK_RX_DR=0, MASK_TX_DS=0, MASK_MAX_RT=0,
             EN_CRC=1, CRCO=1 (2-byte CRC), PWR_UP=1, PRIM_RX=0 */
    NRF24_WriteReg(NRF24_REG_CONFIG, 0x0E);
    HAL_Delay(2);   /* 1.5ms Tpd2stby */
}

/**
 * @brief Transmit exactly len bytes (should be NRF24_PAYLOAD_SIZE).
 * @return true if TX_DS flag set (packet sent into air), false on timeout/MAX_RT.
 *
 * Note: with auto-ACK disabled TX_DS fires as soon as the packet leaves
 * the FIFO, not on acknowledgement. 'true' means the radio transmitted —
 * it does NOT confirm reception at the other end.
 */
bool NRF24_Transmit(uint8_t *data, uint8_t len)
{
    uint8_t cmd;
    uint8_t status;

    NRF24_FlushTX();

    /* Write payload into TX FIFO */
    cmd = NRF24_CMD_W_TX_PAYLOAD;
    CSN_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 10);
    HAL_SPI_Transmit(&hspi1, data, len, 50);
    CSN_HIGH();

    /* Pulse CE ≥10µs to start transmission */
    CE_HIGH();
    HAL_Delay(1);
    CE_LOW();

    /* Poll STATUS for TX_DS (bit5) or MAX_RT (bit4) — timeout 5ms */
    uint32_t start = HAL_GetTick();
    while ((HAL_GetTick() - start) < 5)
    {
        status = NRF24_ReadReg(NRF24_REG_STATUS);

        if (status & 0x20)  /* TX_DS — transmitted */
        {
            NRF24_WriteReg(NRF24_REG_STATUS, 0x20);  /* clear flag */
            return true;
        }

        if (status & 0x10)  /* MAX_RT — max retransmit (shouldn't happen, no ACK) */
        {
            NRF24_WriteReg(NRF24_REG_STATUS, 0x10);
            NRF24_FlushTX();
            return false;
        }
    }

    /* Timeout — radio may be unresponsive, flush and recover */
    NRF24_FlushTX();
    NRF24_WriteReg(NRF24_REG_STATUS, 0x70);
    return false;
}
void NRF24_RX_Mode(void)
{
    CE_LOW();
    /* CONFIG: PWR_UP=1, PRIM_RX=1, CRC enabled */
    NRF24_WriteReg(NRF24_REG_CONFIG, 0x0F);
    HAL_Delay(1);

    /* Flush stale RX data */
    uint8_t cmd = NRF24_CMD_FLUSH_RX;
    CSN_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 10);
    CSN_HIGH();

    CE_HIGH();  /* CE high = actively listening */
}

bool NRF24_ReadPacket(uint8_t *data, uint8_t len)
{
    uint8_t status = NRF24_ReadReg(NRF24_REG_STATUS);

    /* RX_DR = bit6: data ready in RX FIFO */
    if (!(status & 0x40)) return false;

    uint8_t cmd = 0x61;  /* R_RX_PAYLOAD */
    CSN_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 10);
    HAL_SPI_Receive(&hspi1, data, len, 50);
    CSN_HIGH();

    /* Clear RX_DR flag */
    NRF24_WriteReg(NRF24_REG_STATUS, 0x40);
    return true;
}


/**
 * @brief XOR checksum over 'len' bytes.
 *        Call with len = sizeof(V2VPacket) - 1 (exclude checksum byte itself).
 */
uint8_t NRF24_CalcChecksum(uint8_t *data, uint8_t len)
{
    uint8_t cs = 0;
    for (uint8_t i = 0; i < len; i++) cs ^= data[i];
    return cs;
}
