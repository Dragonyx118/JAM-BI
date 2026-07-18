# JAM-BI 🛰️

![GitHub repo size](https://img.shields.io/github/repo-size/Dragonyx118/JAM-BI?color=blue)
![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-red.svg)
![Framework: Arduino](https://img.shields.io/badge/Framework-Arduino-teal.svg)
![Protocol: ESP--NOW](https://img.shields.io/badge/Protocol-ESP--NOW-orange.svg)
![Status: Initial Testing](https://img.shields.io/badge/Status-Initial%20Testing-green.svg)

**JAM-BI** is a firmware ecosystem based on **ESP32** microcontrollers designed for the wireless remote control of multimedia peripherals and actuators. The system leverages the high-performance **ESP-NOW** protocol to enable communication between a central unit (Master) equipped with a Touchscreen graphical user interface and one or more peripheral nodes (Client/Slave).

## 📁 Project Architecture

The core codebase is located within the `firmware/` directory, split into the following main components for initial testing:

### 1. JAM-BI Base Master (User Interface)
The Master manages a **TFT display with an XPT2046 Touchscreen controller** (connected via the SPI/HSPI bus).
* **Boot Animation**: Displays a custom pixel-art animation at startup that renders the project logo.
* **Custom UI**: Utilizes a lightweight pixel-by-pixel text rendering engine optimized to fit long strings neatly within the geometric button bounds.
* **Touch Menu**: Provides an interface with 4 main action buttons:
  * `JAMMER`
  * `UMIDIFICATORE` (Humidifier)
  * `MEDIA`
  * `HELP`
* **Calibration**: Features precise axis mapping calibrated for portrait mode orientation (`Rotation 2`).

### 2. SkibidiFart Base Client (Peripheral Actuator)
The Slave node listens for structured data packets sent by the Master and maps actions to the local hardware. It manages:
* **DFPlayer Mini Audio**: Full control of an MP3-TF-16P player via hardware serial (`HardwareSerial 2`), including volume adjustment, play/pause, stop, SD card detection, and dynamic tracking of total audio files.
* **Humidifier Relay**: Toggles the On/Off state of a relay load (configured on `GPIO 4`).
* **Auto-Pairing**: Automatically registers the Master's MAC Address upon receiving the first valid data packet.

---

## 📡 Communication Protocol (ESP-NOW)

The two modules exchange a fixed data structure called `Comando`:

```cpp
typedef struct {
  uint8_t action;   // Command ID (e.g., 1=Play, 2=Toggle Relay, 3=Volume...)
  uint16_t value;   // Associated numerical value (e.g., Track ID, volume level)
} Comando;