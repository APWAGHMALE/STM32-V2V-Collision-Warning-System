# 🚗 STM32-Based Vehicle-to-Vehicle (V2V) Collision Warning System

An embedded Vehicle-to-Vehicle (V2V) Collision Warning System developed using the **STM32F411RE Nucleo-64**, **NRF24L01+ wireless transceiver**, **NEO-6M GPS**, **MPU6050 IMU**, and **1.3-inch SH1106 SPI OLED Display**. The system enables two vehicles to exchange real-time information such as speed, GPS coordinates, acceleration, and braking status, providing timely warnings to improve road safety.

---

## 📌 Features

- Real-time Vehicle-to-Vehicle (V2V) Communication
- Wireless Data Exchange using NRF24L01+
- GPS-based Vehicle Speed Monitoring
- GPS-based Latitude and Longitude Tracking
- MPU6050 Acceleration Monitoring
- Emergency Brake Detection
- OLED Display for Driver Information and Warnings
- TDMA-based Communication Protocol
- Developed using STM32 HAL Library

---

## 🛠 Hardware Used

| Component | Quantity |
|-----------|:--------:|
| STM32F411RE Nucleo-64 | 2 |
| NRF24L01+ | 2 |
| NEO-6M GPS | 2 |
| MPU6050 | 2 |
| SH1106 1.3" SPI OLED | 2 |

---

## 💻 Software Used

- STM32CubeIDE
- STM32 HAL Library
- Embedded C
- Git & GitHub

---

# System Architecture

<p align="center">
<img src="Images/Block_Diagram.png" width="800">
</p>

---

# Hardware Prototype

<p align="center">
<img src="Images/Hardware_Setup.jpeg" width="700">
</p>

---

# OLED Display

### Normal Display

<p align="center">
<img src="Images/OLED_Normal.png" width="250">
</p>

Displays:

- Latitude
- Longitude
- Speed
- Acceleration (AX, AY, AZ)
- GPS Fix Status

---

### Brake Warning

<p align="center">
<img src="Images/OLED_BrakeWarning.jpeg" width="250">
</p>

Displays a warning whenever emergency braking information is received from another vehicle.

---

# Working Principle

1. GPS module continuously acquires vehicle position and speed.
2. MPU6050 measures vehicle acceleration.
3. STM32 processes the sensor data.
4. A data packet containing vehicle information is created.
5. NRF24L01 wirelessly transmits the packet.
6. The receiving vehicle decodes the received packet.
7. If emergency braking is detected, the OLED displays a warning message.
8. A TDMA communication scheme is used to reduce packet collisions.

---

# Hardware Connections

## GPS (NEO-6M)

| GPS Pin | STM32F411RE Pin | Description |
|----------|-----------------|-------------|
| TX | **PC7 (USART6_RX)** | GPS → STM32 Data |
| RX | **PC6 (USART6_TX)** | STM32 → GPS Data |
| VCC | **3.3V** | Power Supply |
| GND | **GND** | Ground |

---

## MPU6050

| MPU6050 Pin | STM32F411RE Pin | Description |
|--------------|-----------------|-------------|
| SDA | **PB9 (I2C1_SDA)** | I²C Data |
| SCL | **PB8 (I2C1_SCL)** | I²C Clock |
| VCC | **3.3V** | Power Supply |
| GND | **GND** | Ground |

---

## NRF24L01+

| NRF24L01 Pin | STM32F411RE Pin | Description |
|---------------|-----------------|-------------|
| CE | **PA1** | Chip Enable |
| CSN | **PA4 (SPI1_NSS)** | Chip Select |
| SCK | **PA5 (SPI1_SCK)** | SPI Clock |
| MOSI | **PA7 (SPI1_MOSI)** | SPI Data Out |
| MISO | **PA6 (SPI1_MISO)** | SPI Data In |
| IRQ | Not Connected | Interrupt (Unused) |
| VCC | **3.3V** | Power Supply |
| GND | **GND** | Ground |

> **Note:** A **10 µF capacitor** is connected across the NRF24L01+ **VCC** and **GND** pins to ensure a stable 3.3 V supply during RF transmission.

---

## SH1106 SPI OLED Display

| OLED Pin | STM32F411RE Pin | Description |
|-----------|-----------------|-------------|
| VCC | **3.3V** | Power Supply |
| GND | **GND** | Ground |
| DIN (MOSI) | **PA7 (SPI1_MOSI)** | SPI Data |
| CLK | **PA5 (SPI1_SCK)** | SPI Clock |
| CS | **PB6** | Chip Select |
| DC | **PB7** | Data/Command Select |
| RST | **PB5** | Display Reset |
| NC | Not Connected | Not Used |


---

# Future Improvements

- Vehicle-to-Infrastructure (V2I) Communication
- Road Side Unit (RSU) Integration
- Blind Spot Detection
- CAN Bus Integration
- C-V2X Communication
- Mobile Application Interface

---

# Author

**Atharv Pramod Waghmale**

Bachelor of Engineering (Electronics & Telecommunication)

Embedded Systems • Automotive Electronics • IoT • VLSI

---

# License

This project is licensed under the MIT License.
