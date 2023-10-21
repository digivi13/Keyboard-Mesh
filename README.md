# Keyboard-Mesh
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
  
![IMG_5379](https://github.com/digivi13/Keyboard-Mesh/assets/33264428/51a60f03-62eb-408f-af15-39eb6f38eb31)
![IMG_5388](https://github.com/digivi13/Keyboard-Mesh/assets/33264428/71a883cd-e1c2-437c-8cff-66bf5fe72a1a)
