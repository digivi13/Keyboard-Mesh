# Keyboard-Mesh

Process was developed using guide here: 

https://www.aranacorp.com/en/generating-and-uploading-bin-files-for-esp32/

Quick upload using my HAE laptop (has CP210X serial driver):

1) Connect to ESP32-S3 USB OTG device through USB-UART 
   
2) open command prompt
   
3) copy command below and press enter (may need to modify the COM port to be used)

<div>
C:\Users\vdigiovanni\Documents\ArduinoData\packages\esp32\tools\esptool_py\4.5.1/esptool.exe --chip esp32s3 --port COM9 --baud 921600 --before default_reset --after hard_reset write_flash -e -z --flash_mode dio --flash_freq 80m --flash_size 8MB 0x0 C:\Users\VDIGIO~1\AppData\Local\Temp\arduino_build_240785/esp32_with_menu_BLUETOOTHV6.ino.bootloader.bin 0x8000 C:\Users\VDIGIO~1\AppData\Local\Temp\arduino_build_240785/esp32_with_menu_BLUETOOTHV6.ino.partitions.bin 0xe000 C:\Users\vdigiovanni\Documents\ArduinoData\packages\esp32\hardware\esp32\2.0.10/tools/partitions/boot_app0.bin 0x10000 C:\Users\VDIGIO~1\AppData\Local\Temp\arduino_build_240785/esp32_with_menu_BLUETOOTHV6.ino.bin
</div>


4) Device will reset and firmware will be installed
   
Important notes: 
tft_espi version: 3.0  --> Download the user_setup.h file that is included in this repo and replace user_setup.h in the tft_espi library (essential for screen to work on ESP32-S3 USB OTG) 

ESP 32 board manager version (wrong version will cause panic) 2.0.10

Arduino sketch setup:
install esp32 from arduino boards manager (Tools -> boards)
Required Arduino libraries to compile:
Painless mesh v1.5.0

Dependencies required for painless mesh:
ArduinoJson
TaskScheduler
ESPAsyncTCP (ESP8266)
AsyncTCP (ESP32)

Network of ESP32-S3 USB OTG devices acting as HID keyboards, allowing you to type the same string onto many diferent devices. Useful for industrial applications where manual, repetative user input on many devices is required. 

Devices with the same device ID will form a network once powered on. Each device broadcasts a bluetooth connection that users can connect to with their iphone. This bluetooth connection has a characteristic, that when modified, will send the new value to the rest of the devices in the network. 

donwload the nRF connect app on your phone and connect to VinnyNet001 or VinnyNetXXX based on the device ID assigned. ESP32-S3 USB OTG devices are used for this setup. 

The deviceID can be changed by pressing the side menu button. A new screen appears and the volume UP+ DW- buttons can be used to adjust the value higher or lower. Press menu again and the ID will be saved. Once the device restarts it will have a new network ID reflecting the change in the device ID 

The last 3 digits of the MESH_SSID are equal to the device ID, allowing for the creation of separate smaller networks by adjusting device ID to values greater than 001 by using the menu button

The device support OTA updates. By adjusting the device ID to 000 and restarting the device, OTA mode is activated. A OTA symbol appears in upper right to indicate OTA mode is present. Devices not on in OTA mode and on the same network will be updated, one-by-one, with software loaded onto the SD card under "/OTA/firmware_ESP32_otareceiver.bin" on the OTA device. Devices not on the same network will not receive this firmware.

the following coniguration variables can be adjusted <br>
  "deviceID" -> The three digit (use three digits!) identification number of the device <br>
  "MESH_SSID" -> Username for mesh network (all devices need to have same value!) <br>
  "MESH_PASSWORD" -> Password for mesh network (all devices need to have same value!) <br>
  "MESH_PORT" -> Port that the mesh is operating on (all devices need to have same value!) <br>
  "username" -> Pre programmed text string that is sent when the "UP+" button is pushed on the device <br>
  "password" -> Pre programmed text string that is sent when the "DW-" button is pushed on the device <br>
  "fileName" -> Name of file on SD card containing the text you want to send when the book/ok button is pressed <br>
<br>
example of changing a config value:  connect to device using arduino IDE and send the following text by serial monitor to change the MESH_SSID <br>

MESH_SSID: 001TestNetwork <br>

This will change the MESH_SSID to 001TestNetwork <br>

![IMG_5379](https://github.com/digivi13/Keyboard-Mesh/assets/33264428/51a60f03-62eb-408f-af15-39eb6f38eb31)
![IMG_5388](https://github.com/digivi13/Keyboard-Mesh/assets/33264428/71a883cd-e1c2-437c-8cff-66bf5fe72a1a)
