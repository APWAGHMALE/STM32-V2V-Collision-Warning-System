# STM32-V2V-Collision-Warning-System
STM32-based Vehicle-to-Vehicle Collision Warning System using NRF24L01, GPS, MPU6050 and SH1106 OLED.

# 🚗 STM32-Based Vehicle-to-Vehicle (V2V) Collision Warning System

An embedded Vehicle-to-Vehicle (V2V) Collision Warning System developed using the **STM32F411RE Nucleo Board**, **NRF24L01+ wireless transceiver**, **GPS (NEO-6M)**, **MPU6050 IMU**, and **SH1106 SPI OLED Display**. The system enables two vehicles to exchange real-time information such as speed, GPS location, acceleration, and braking status, providing early warnings to improve road safety.

---

## 📌 Features

- Vehicle-to-Vehicle (V2V) Communication
- Real-time Wireless Data Exchange using NRF24L01+
- GPS-based Speed and Position Monitoring
- MPU6050 Acceleration Monitoring
- Emergency Brake Detection
- OLED-based Driver Warning Display
- TDMA-based Communication Scheme
- Developed using STM32 HAL Library

---

## 🛠 Hardware Used

| Component | Quantity |
|-----------|---------:|
| STM32F411RE Nucleo-64 | 2 |
| NRF24L01+ Wireless Module | 2 |
| NEO-6M GPS Module | 2 |
| MPU6050 IMU Sensor | 2 |
| SH1106 1.3" SPI OLED Display | 2 |
| USB Power Supply | 2 |

---

## 💻 Software Used

- STM32CubeIDE
- STM32 HAL Drivers
- Embedded C
- Git & GitHub

---

## 📡 System Architecture

```
                 Vehicle A                         Vehicle B
          ┌──────────────────┐              ┌──────────────────┐
          │ STM32F411RE      │              │ STM32F411RE      │
          │                  │              │                  │
 GPS ───▶ │                  │              │                  │ ◀── GPS
 MPU6050 ▶│                  │◀────────────▶│                  │ ◀── MPU6050
 OLED ───▶│                  │  NRF24L01+   │                  │ ◀── OLED
          └──────────────────┘              └──────────────────┘
```

---

## ⚙ Working Principle

1. GPS module continuously measures vehicle speed and position.
2. MPU6050 monitors vehicle acceleration and braking.
3. STM32 processes the sensor data.
4. Vehicle information is transmitted through the NRF24L01+ module.
5. The receiving STM32 decodes the received packet.
6. Emergency braking information is displayed on the OLED.
7. Communication follows a TDMA schedule to minimize packet collisions.

---

## 📦 Packet Structure

| Field | Size |
|------|------:|
| Vehicle ID | 1 Byte |
| Latitude | 4 Bytes |
| Longitude | 4 Bytes |
| Speed | 4 Bytes |
| Acceleration X | 4 Bytes |
| Acceleration Y | 4 Bytes |
| Acceleration Z | 4 Bytes |
| Brake Flag | 1 Byte |
| Sequence Number | 2 Bytes |

---

## 📷 Project Images

### Hardware Setup

> *(Insert image here)*

```
Images/Hardware_Setup.jpg
```

### OLED Display

> *(Insert image here)*

```
Images/OLED_Display.jpg
```

### Block Diagram

> *(Insert image here)*

```
Images/BlockDiagram.png
```

---

## 📂 Repository Structure

```
STM32-V2V-Collision-Warning-System
│
├── STM32_Project/
├── Hardware/
├── Images/
├── Documents/
├── Videos/
├── LICENSE
└── README.md
```

---

## 🚀 Future Improvements

- Vehicle-to-Infrastructure (V2I) Communication
- Road Side Unit (RSU) Integration
- Blind Spot Detection
- CAN Bus Integration
- C-V2X / DSRC Communication
- Mobile Application Interface

---

## 👨‍💻 Author

**Atharv Waghmale**

Bachelor of Engineering (Electronics & Telecommunication)

Embedded Systems | IoT | Automotive Electronics | VLSI

---

## 📄 License

This project is licensed under the MIT License.
