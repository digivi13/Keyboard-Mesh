# ESP32-S3 Mesh Bluetooth Keyboard Project

This project enables a network of ESP32-S3 devices to function as HID keyboards, allowing simultaneous data entry on multiple devices. It is particularly useful for industrial applications where repetitive manual input is required on many devices.

---

## Table of Contents
1. [Overview](#overview)  
2. [Hardware & Software Requirements](#hardware--software-requirements)  
3. [Firmware Upload Procedure](#firmware-upload-procedure)  
4. [Important Notes](#important-notes)  
5. [Arduino Sketch Setup](#arduino-sketch-setup)  
6. [Usage & Network Behavior](#usage--network-behavior)  
7. [Configuration Variables](#configuration-variables)  
8. [OTA Updates](#ota-updates)  

---

## Overview
Each ESP32-S3 device joins a painlessMesh network, broadcasting a Bluetooth connection that can be accessed by an iPhone (or other BLE-enabled device). When a new value is written to the device’s Bluetooth characteristic (e.g., via the [nRF Connect app](https://www.nordicsemi.com/Products/Development-tools/nrf-connect-for-mobile)), the data is shared across the mesh network, effectively typing the same string on all connected devices.

---

## Hardware & Software Requirements
- **ESP32-S3 USB OTG development board**  
- **Silicon Labs CP210x USB-to-UART driver** (for Windows, if needed)  
- **Arduino IDE** with ESP32 board support ([Board Manager v2.0.10](https://github.com/espressif/arduino-esp32))  
- **[TFT_eSPI library (v3.0)](https://github.com/Bodmer/TFT_eSPI)**
  - Replace `user_setup.h` in the TFT_eSPI library with the provided `user_setup.h` from this repository to ensure screen compatibility.  
- **Required Arduino Libraries for painlessMesh**  
  1. [painlessMesh v1.5.0](https://github.com/gmag11/painlessMesh)  
  2. [ArduinoJson](https://arduinojson.org/)  
  3. [TaskScheduler](https://github.com/arkhipenko/TaskScheduler)  
  4. [AsyncTCP (ESP32)](https://github.com/me-no-dev/AsyncTCP)  
  5. [ESPAsyncTCP (ESP8266)](https://github.com/me-no-dev/ESPAsyncTCP) – Only required if you plan to compile for ESP8266  

---

## Firmware Upload Procedure
Process references: [Generating and uploading .bin files for ESP32 by AranaCorp](https://www.aranacorp.com/en/generating-and-uploading-bin-files-for-esp32/).

1. **Connect the ESP32-S3**  
   - Use a USB-UART adapter (e.g., CP210x) if necessary.  
2. **Open Command Prompt** on Windows.  
3. **Run the following command** (adjust COM port and file paths as needed):

```bash
C:\Users\vdigiovanni\Documents\ArduinoData\packages\esp32\tools\esptool_py\4.5.1/esptool.exe --chip esp32s3 --port COM7 --baud 921600 --before default_reset --after hard_reset write_flash -e -z --flash_mode dio --flash_freq 80m --flash_size 8MB 0x0 C:\Users\VDIGIO~1\AppData\Local\Temp\arduino_build_240785/esp32_with_menu_BLUETOOTHV6.ino.bootloader.bin 0x8000 C:\Users\VDIGIO~1\AppData\Local\Temp\arduino_build_240785/esp32_with_menu_BLUETOOTHV6.ino.partitions.bin 0xe000 C:\Users\vdigiovanni\Documents\ArduinoData\packages\esp32\hardware\esp32\2.0.10/tools/partitions/boot_app0.bin 0x10000 C:\Users\VDIGIO~1\AppData\Local\Temp\arduino_build_240785/esp32_with_menu_BLUETOOTHV6.ino.bin
```

4. **Allow the device to reset** after the flash process. Firmware installation will complete automatically.

---

## Important Notes
- **TFT_eSPI v3.0** is required. Use the `user_setup.h` from this repo to ensure the display works on the ESP32-S3 USB OTG device.  
- **ESP32 Board Manager Version 2.0.10** is mandatory to prevent panic errors.  
- If you see a boot loop or “panic,” double-check that all version requirements are met.

---

## Arduino Sketch Setup
1. **Install the ESP32 package** from the Arduino Boards Manager (Tools → Board → Boards Manager).  
2. **Install Required Libraries** listed above (painlessMesh, ArduinoJson, etc.).  
3. **Open this project** in Arduino IDE, select the **ESP32S3** board, and verify that the correct COM port is selected (Tools → Port).  
4. **Compile and Upload** the sketch as you would any Arduino project.

---

## Usage & Network Behavior
1. **Mesh Network Formation**  
   - Devices with identical MESH_SSID, MESH_PASSWORD, and MESH_PORT will automatically form a wireless mesh.  
   - The default MESH_SSID includes the last three digits of the device’s ID (e.g., `VinnyNet001`).  

2. **Bluetooth Connectivity**  
   - Each device advertises a BLE characteristic that can be changed (e.g., via nRF Connect).  
   - Any update to this characteristic is sent to all devices in the mesh, ensuring synchronized text entry.  

3. **Device ID Configuration**  
   - Press the **side menu button** on the device to bring up a configuration screen.  
   - Use **volume UP+** / **volume DW-** to increase/decrease the device ID.  
   - Press **menu** again to save. The device restarts with the new ID and joins the corresponding mesh network.

4. **Pre-Programmed Keystrokes**  
   - **UP+ button** sends the `username` string.  
   - **DW- button** sends the `password` string.  
   - **Book/OK button** sends the content of the SD card file specified by `fileName`.

---

## Configuration Variables
The following variables can be adjusted at runtime via serial commands or via the menu system:

| Variable       | Description                                                               |
|----------------|---------------------------------------------------------------------------|
| `deviceID`     | Three-digit identifier (e.g. `001`) that forms part of the MESH_SSID       |
| `MESH_SSID`    | SSID for the mesh network (all devices must share the same SSID)           |
| `MESH_PASSWORD`| Password for the mesh network                                             |
| `MESH_PORT`    | Port on which the mesh operates                                           |
| `username`     | String sent when the “UP+” button is pressed                               |
| `password`     | String sent when the “DW-” button is pressed                              |
| `fileName`     | The path on the SD card for a file whose contents are sent on “Book/OK”   |

**Example (change MESH_SSID via serial monitor):**
```
MESH_SSID: 001TestNetwork
```
This updates the `MESH_SSID` to `001TestNetwork`.

---

## OTA Updates
1. **Activate OTA Mode**  
   - Set `deviceID` to `000` and restart the device.  
   - An OTA icon will appear in the upper-right corner to confirm OTA mode.  
2. **Perform OTA Update**  
   - The device in OTA mode will update other devices on the same mesh network (not in OTA mode) one-by-one.  
   - The firmware must be located on the SD card in `/OTA/firmware_ESP32_otareceiver.bin`.  
3. **Isolated Updates**  
   - Devices outside the network (different MESH_SSID or turned off) will not receive the update.

---

**Enjoy seamless HID keyboard input across your ESP32-S3 mesh network!** If you have any questions or encounter issues, please open an [issue](../../issues) in this repository.
