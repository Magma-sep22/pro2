# TCE SAT - CubeSat Ground Station
This project provides a comprehensive dashboard for satellite telemetry, including Orbit Mapping, Intersat Link analysis, and LIDAR scanning.

## Firmware Setup: Uploading Bootloader to ATmega328PB
To program the custom Ground Station board, follow these steps:

### Required Hardware
- Arduino UNO (as ISP programmer)
- Target board with ATmega328PB
- 6 jumper wires

### Wiring Connections
Connect Arduino UNO to target board as follows:

| Arduino Pin | Target Board Pin | Function |
|------------|------------------|----------|
| 10 | RESET | RESET |
| 11 | MOSI | MOSI |
| 12 | MISO | MISO |
| 13 | SCK | SCK |
| 3.3V | VCC | Power (3.3V) |
| GND | GND | Ground |

### Uploading Bootloader via MiniCore
1. Install **MiniCore** in Arduino IDE (Boards Manager).
2. Settings: **ATmega328**, Clock: **8 MHz internal**, Variant: **328PB**, Bootloader: **Yes (UART 0)**.
3. Select **Arduino as ISP** and click **Burn Bootloader**.

## Application Features
- **Real-time Telemetry:** Monitor battery voltage, RSSI, and internal temperature.
- **Orbit Map:** TLE Propagation and target tracking (e.g., ISS).
- **LIDAR Tracking:** Object azimuth and collision assessment using Garmin LIDAR-Lite v4.
- **Intersat Link:** Signal analysis for QPSK modulation and K-band frequencies.

---
*Developed by Telecommunication Engineering student  - SUT.LIDARS Team*
### 🖥️ Mission Control Dashboard
![Dashboard](./assets/images/dashboard.jpg)

### 🛰️ Orbit Propagation (TLE Target: ISS)
![Orbit Map](./assets/images/orbit.jpg)

### 🛡️ LIDAR Tracking Matrix
![LIDAR Matrix](./assets/images/lidar.jpg)