# Keyboard-Mesh

Devices with the same device ID will form a network once powered on. Each device broadcasts a bluetooth connection that users can connect to with their iphone. This bluetooth connection has a characteristic, that when modified, will send the new value to the rest of the devices in the network. 

donwload the nRF connect app and connect to VinnyNet001 or VinnyNetXXX based on the device ID assigned. 

Mesh network of virtual keyboards using ESP32-S3 USB OTG devices

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

The deviceID can be changed by pressing the side menu button. A new screen appears and the volume up+ dW- buttons can be used to adjust the value lower or higher. Press menu again and the ID will be saved. Once the device restarts it will have a new network ID reflecting the change in the device ID 

The last 3 digits of the MESH_SSID are equal to the device ID, allowing for the creation of separate smaller networks by adjusting device ID to values greater than 001 by using the menu button

The device support OTA updates. By adjusting the device ID to 000 and restarting the device, OTA mode is activated. A OTA symbol appears in upper right to indicate OTA mode is present. Devices not on in OTA mode and on the same network will be updated, one-by-one, with software loaded onto the SD card under "/OTA/firmware_ESP32_otareceiver.bin" on the OTA device. Devices not on the same network will not receive this firmware.

![IMG_5379](https://github.com/digivi13/Keyboard-Mesh/assets/33264428/51a60f03-62eb-408f-af15-39eb6f38eb31)
![IMG_5388](https://github.com/digivi13/Keyboard-Mesh/assets/33264428/71a883cd-e1c2-437c-8cff-66bf5fe72a1a)
