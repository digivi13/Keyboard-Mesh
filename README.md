# Keyboard-Mesh
Mesh network of virtual keyboards using ESP32-S3 USB OTG devices

the following coniguration variables can be adjusted
  "deviceID" -> The identification number o the device
  "MESH_SSID" -> Username for mesh network (all devices need to have same value!)
  "MESH_PASSWORD" -> Password for mesh network (all devices need to have same value!)
  "MESH_PORT" -> Port that the mesh is operating on (all devices need to have same value!)
  "username" -> Pre programmed text string that is sent when the "UP+" button is pushed on the device
  "password" -> Pre programmed text string that is sent when the "DW-" button is pushed on the device
  "fileName" -> Name of file on SD card containing the text you want to send

example of changing a config value:  connect to device using arduino IDE and send the following text by serial monitor to change the MESH_SSID

MESH_SSID: 001TestNetwork

This will change the MESH_SSID to 001TestNetwork
  
![IMG_5379](https://github.com/digivi13/Keyboard-Mesh/assets/33264428/51a60f03-62eb-408f-af15-39eb6f38eb31)
![IMG_5388](https://github.com/digivi13/Keyboard-Mesh/assets/33264428/71a883cd-e1c2-437c-8cff-66bf5fe72a1a)
