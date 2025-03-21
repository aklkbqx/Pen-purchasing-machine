# 🖊️ ESP32 Pen Purchasing Machine

[![PlatformIO](https://img.shields.io/badge/IDE-PlatformIO-orange?style=for-the-badge&logo=platformio)](https://platformio.org/)
[![VSCode](https://img.shields.io/badge/IDE-VSCode-blue?style=for-the-badge&logo=visualstudiocode)](https://code.visualstudio.com/)
[![ESP32](https://img.shields.io/badge/Device-ESP32-red?style=for-the-badge&logo=espressif)](https://www.espressif.com/)
[![BLYNK](https://img.shields.io/badge/Control-BLYNK-pink?style=for-the-badge&logo=blynk)](https://blynk.io/)
[![LINE](https://img.shields.io/badge/Notification-LINE-green?style=for-the-badge&logo=line)](https://developers.line.biz/)

## 📝 Project Overview

An automated pen vending machine built with ESP32. This system accepts coins, dispenses pens, and provides notifications through LINE and monitoring via BLYNK.

![Project Overview](https://github.com/user-attachments/assets/7d01bb1e-367c-4e41-b702-de5b7ce2bf36)

## ✨ Features

- **Coin Acceptance**: Validates and accepts coins for payment
- **Automatic Dispensing**: Releases pens when payment is received
- **LCD Display**: Shows transaction information and machine status
- **LINE Notifications**: Sends alerts and status updates via LINE
- **BLYNK Integration**: Remote monitoring and control capabilities
- **Power Management**: Using 12V and 5V relay systems

## 🛠️ Hardware Components

- ESP32 Microcontroller
- Step-down converter (12V)
- Coin Validator
- LCD Display (No I2C)
- 12V Relay
- 5V Relay
- Vending mechanism

## 📸 Project Gallery

<div align="center">
  <img src="https://github.com/user-attachments/assets/8f115d26-956d-4090-bcd6-de8adb0712a2" width="400" alt="Machine Front View"/>
  <img src="https://github.com/user-attachments/assets/d809819b-c9fa-4795-ad80-193c2e2f0d94" width="400" alt="Internal Components"/>
</div>

<div align="center">
  <img src="https://github.com/user-attachments/assets/023c02ab-9ba3-4010-a7c0-f7e2f19729f6" width="400" alt="Circuit Setup"/>
  <img src="https://github.com/user-attachments/assets/eb16e6fe-dc35-46a6-bd1d-645e51d22f66" width="400" alt="Control Panel"/>
</div>

<div align="center">
  <img src="https://github.com/user-attachments/assets/de58a4c2-e368-4d38-a103-0451afef634b" width="400" alt="Display View"/>
  <img src="https://github.com/user-attachments/assets/7bf5e325-f595-4f51-9bdb-25211a54554b" width="400" alt="Full Assembled View"/>
</div>

<div align="center">
  <img src="https://github.com/user-attachments/assets/35b23c1d-f4d4-4b0c-9ce5-677cd9b438e1" width="400" alt="Testing Setup"/>
  <img src="https://github.com/user-attachments/assets/c2cbe3ad-8df1-4858-b188-44c457e3ffc3" width="400" alt="Installation View"/>
</div>

## 🔌 Wiring Diagram

```
ESP32 ──────┐
            ├── LCD Display
            ├── Coin Validator
            ├── 5V Relay ─── Dispensing Mechanism
            └── 12V Relay ─── Power Components
```

## 💻 Software Requirements

- PlatformIO
- Visual Studio Code
- LINE Messaging API Token
- BLYNK Account and Authentication

## ⚙️ Installation & Setup

1. Clone this repository
2. Open the project in Visual Studio Code with PlatformIO extension
3. Configure your LINE token and BLYNK credentials in the `config.h` file
4. Build and upload to your ESP32

## 📱 Mobile Integration

This project integrates with:
- **LINE** for instant notifications
- **BLYNK** for remote monitoring and control
