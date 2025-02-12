#include <Preferences.h>
#include <painlessMesh.h>
#include <Arduino.h>
#include "USB.h"
#include "USBHIDKeyboard.h"
#include <USBHID.h>
#include "driver/gpio.h"
#include <FS.h>
#include "SD.h"
#include "SPI.h"
#include <TFT_eSPI.h>
#include <DigitalRainAnimation.hpp>
#include "usb/usb_host.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#define OTA_PART_SIZE 1024
unsigned long lastPasswordBroadcastTime = 0;
const unsigned long passwordBroadcastDelay = 335;  // Delay in milliseconds

unsigned long lastButtonCheckTime = 0;
const unsigned long buttonCheckDelay = 335;  // Delay in milliseconds

unsigned long lastConnectionTime = 0;
const unsigned long connectionInterval = 3 * 60 * 1000;  // Reconnect every 3 minutes

bool characteristicChanged = false;  // Flag to track characteristic changes

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/


BLECharacteristic *pCharacteristic;
BLEServer *pServer;
TFT_eSPI tft = TFT_eSPI(); 
USBHIDKeyboard Keyboard;


// some gpio pin that is connected to an LED...
// on my rig, this is 15, change to the right number of your LED.
#ifdef LED_BUILTIN
#define LED LED_BUILTIN
#else
#define LED 15
#endif
#define BUTTON_1_GPIO GPIO_NUM_10
#define BUTTON_2_GPIO GPIO_NUM_11
#define   BLINK_PERIOD    3000 // milliseconds until cycle repeat
#define   BLINK_DURATION  100  // milliseconds LED is on for
const char* scriptName = __FILE__; //load name of the script
String scriptVersion = "V" + String(scriptName[strlen(scriptName) - 5]); //get the last character of the sketch name and use it as the version
String deviceID = "001";
String MESH_SSID = "whateverYouLike";
String MESH_PASSWORD = "somethingSneaky";
String bleNetwork = "VinnyNet";
int MESH_PORT = 5555;
String username = "haelab\n";
String password = "teg6s234\n";
String fileName = "/patientID.txt";
String OTA = "receiver";
struct ConfigVariable {
  String name;
  String* variable;
};

// Create an array of configuration variables
ConfigVariable configVariables[] = {
  {"deviceID", &deviceID},
  {"OTA", &OTA},
  {"MESH_SSID", &MESH_SSID},
  {"MESH_PASSWORD", &MESH_PASSWORD},
  {"MESH_PORT", (String*)&MESH_PORT}, // Convert int to String
  {"username", &username},
  {"password", &password},
  {"fileName", &fileName}
};
String usbInput;
File dataFile;
// Prototypes
void sendMessage(); 
void receivedCallback(uint32_t from, String & msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback(); 
void nodeTimeAdjustedCallback(int32_t offset); 
void delayReceivedCallback(uint32_t from, int32_t delay);

Scheduler     userScheduler; // to control your personal task
painlessMesh  mesh;

bool calc_delay = false;
SimpleList<uint32_t> nodes;

void sendMessage() ; // Prototype
Task taskSendMessage( TASK_SECOND * 1, TASK_FOREVER, &sendMessage ); // start with a one second interval

// Task to blink the number of nodes
Task blinkNoNodes;
bool onFlag = false;
const int pin10 = 10;
const int pin11 = 11;
const int maxMessages = 4; // Maximum number of messages to buffer
String messages[maxMessages]; // Buffer to store received messages
int messageCount = 0; // Number of messages in the buffer
int currentMessage = 0; // Index of the message to print next
int timeUnconnected = 0;
bool configMode = false; // Flag to indicate if the device is in configuration mode
const int GPIO_BUTTON_MENU = 14;
Preferences preferences;
bool deviceConnected = false;

void updateDisplay() { //call this function to go back to the main viewport or update it 
  const int maxDisplayedLines = (tft.height() / (tft.fontHeight() * 2)) - 2; // Calculate maximum number of lines to display
  const int maxCharsPerLine = tft.width() / tft.textWidth("W"); // Calculate maximum characters per line
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE);

  // Print device ID
  int16_t deviceID_x = (tft.width() - tft.textWidth(deviceID)) / 2;
  int16_t deviceID_y = tft.height() / 6;
  tft.fillRect(0, deviceID_y + tft.fontHeight(), tft.width(), tft.height() - deviceID_y - tft.fontHeight(), TFT_BLACK);
  tft.setCursor(3, deviceID_y);
  tft.print("DeviceID:" + deviceID);

  // Display number of nodes in the network
  tft.setTextSize(2);
  tft.setTextColor(TFT_GREEN); // Choose a color for the node count
  String nodeCountText = "Node Count: " + String(mesh.getNodeList().size() + 1);
  int16_t nodeCount_x = (tft.width() - tft.textWidth(nodeCountText)) / 2;
  int16_t nodeCount_y = deviceID_y - tft.fontHeight() - 10 - 3; // Added a 3-pixel margin
  tft.fillRect(0, nodeCount_y, tft.width(), tft.fontHeight() + 4, TFT_BLACK);
  tft.setCursor(nodeCount_x, nodeCount_y);
  tft.print(nodeCountText);
  tft.setCursor(3, nodeCount_y);
  tft.print(scriptVersion);//show the script version 
  tft.setCursor(3, 220);// move to bottom of screen 
  tft.print(MESH_SSID);//write the name of the MESH-SSID network
  tft.setCursor(3, nodeCount_y);//move cursor back to node count
  if ((deviceConnected == true) && (OTA == "receiver")) { //check if device is connected to bluetooth 
    int16_t bluetoothIndicator = nodeCount_x + tft.textWidth(nodeCountText) + 3; //calculate osition to place BT indicator
    tft.setCursor(bluetoothIndicator, nodeCount_y);// set cursor to this calculated point
    tft.print("BT");// print the BT indicator
  } else if (OTA == "sender") {
    int16_t OTAIndicator = nodeCount_x + tft.textWidth(nodeCountText) + 3; //calculate osition to place BT indicator
    tft.setCursor(OTAIndicator, nodeCount_y);// set cursor to this calculated point
    tft.print("OTA");
  }
  
  // Set cursor for message display
  tft.setTextColor(TFT_WHITE); // change text color to black 
  tft.setCursor(5, deviceID_y + 40);

  // Calculate starting message index based on the current message count and display limit
  int startIndex = max(0, messageCount - maxDisplayedLines);

  for (int i = startIndex; i < messageCount; i++) {
    int index = (currentMessage + i) % maxMessages;
    tft.setTextSize(2);
    if (i == messageCount - 1) {// need to subtract 1 because array start at position zero
      tft.print("Last:" + messages[index]); // Use print instead of println
    } else {
      tft.print(messages[index]); // Use print instead of println
    }
    tft.setCursor(5, tft.getCursorY() + 2 * 11);

    // Check if the cursor position exceeds the screen height
    // If so, reset the cursor position and break out of the loop
    if (tft.getCursorY() >= tft.height() - tft.fontHeight()) {
      tft.setCursor(5, deviceID_y);
      break;
    }
  }
}

class MyCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        // Handle characteristic write event here
        characteristicChanged = true;  // Set the flag to true when the characteristic changes
    }
    
};
class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        Serial.println("Device connected");
        deviceConnected = true; // set this flag to true so program can know a ble device is connected
        updateDisplay();
    }

    void onDisconnect(BLEServer* pServer) {
        Serial.println("Device disconnected");
        // Handle disconnection and any additional actions here
        deviceConnected = false;
        pServer->getAdvertising()->start();// on doisconnect start advertising device again soit canbe used
        updateDisplay();
    }
};
void setupConfigVariables() {
  // Begin the preferences storage with a namespace
  preferences.begin("configNamespace", false);

  // Initialize and retrieve each configuration variable

  // Initialize and retrieve deviceID
  if (preferences.isKey("deviceID")) {
    deviceID = preferences.getString("deviceID", deviceID);
  } else {
    // Set the default value if OTA doesn't exist in preferences
    preferences.putString("deviceID", deviceID);
  }
  if (preferences.isKey("OTA")) {
    OTA = preferences.getString("OTA", OTA);
  } else {
    // Set the default value if OTA doesn't exist in preferences
    preferences.putString("OTA", OTA);
  }
  // Initialize and retrieve MESH_SSID
  if (preferences.isKey("MESH_SSID")) {
    MESH_SSID = preferences.getString("MESH_SSID", MESH_SSID);
  } else {
    // Set the default value if MESH_SSID doesn't exist in preferences
    preferences.putString("MESH_SSID", MESH_SSID);
  }
  if (preferences.isKey("bleNetwork")) {
    bleNetwork = preferences.getString("bleNetwork", bleNetwork);
  } else {
    // Set the default value if deviceID doesn't exist in preferences
    preferences.putString("bleNetwork", bleNetwork);
  }
  // Initialize and retrieve MESH_PASSWORD
  if (preferences.isKey("MESH_PASSWORD")) {
    MESH_PASSWORD = preferences.getString("MESH_PASSWORD", MESH_PASSWORD);
  } else {
    // Set the default value if MESH_PASSWORD doesn't exist in preferences
    preferences.putString("MESH_PASSWORD", MESH_PASSWORD);
  }

  // Initialize and retrieve MESH_PORT
  if (preferences.isKey("MESH_PORT")) {
    MESH_PORT = preferences.getInt("MESH_PORT", MESH_PORT);
  } else {
    // Set the default value if MESH_PORT doesn't exist in preferences
    preferences.putInt("MESH_PORT", MESH_PORT);
  }
  if (preferences.isKey("fileName")) {
    fileName = preferences.getString("fileName", fileName);
  } else {
    // Set the default value if MESH_PORT doesn't exist in preferences
    preferences.getString("fileName", fileName);
  }
  // Initialize and retrieve username
  if (preferences.isKey("username")) {
    username = preferences.getString("username", username);
  } else {
    // Set the default value if username doesn't exist in preferences
    preferences.putString("username", username);
  }

  // Initialize and retrieve password
  if (preferences.isKey("password")) {
    password = preferences.getString("password", password);
  } else {
    // Set the default value if password doesn't exist in preferences
    preferences.putString("password", password);
  }
}



void setup() {
  
  Serial.begin( 115200 );
  
  //usbHost.begin();
  pinMode(5, OUTPUT);
  digitalWrite(5, HIGH); 
  pinMode(34, OUTPUT);
  digitalWrite(18, LOW);
  digitalWrite(34, HIGH);
  tft.init();
  tft.setRotation(3);
  setupConfigVariables();
  deviceID = preferences.getString("deviceID", deviceID);

  MESH_SSID = String("whateverYouLike" + deviceID);//allows for creation of multiple networks based on device ID
  
  //generate unique service and characteristic UUID to prevent communication issues with large numbers/multiple devices.
  int randomNum = 10000 + random(90000);
  String stringNum = String(randomNum);
  String SERVICE_UUID = deviceID + stringNum + "-1fb5-459e-8fcc-c5c9c331914b";
  String CHARACTERISTIC_UUID = deviceID + stringNum + "-36e1-4688-b7f5-ea07361b26a8";
  ///ble setup 
  Serial.println("Starting BLE work!");
  String storedNetworkID = "";
  storedNetworkID = preferences.getString("bleNetwork", bleNetwork);
  String bleNetID = String(storedNetworkID + " " + deviceID);
  // networkID = bleNetID;

  BLEDevice::init(bleNetID.c_str());
  Serial.println("Starting BLE work 2 !");
  BLEServer *pServer = BLEDevice::createServer();//sertup bluetooth server
  pServer->setCallbacks(new MyServerCallbacks());//setup callbacks to handle bluetooth disconnection
  BLEService *pService = pServer->createService(SERVICE_UUID.c_str());
  pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID.c_str(),
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setValue("Hello World says Vinny");
  MyCallbacks *pCallbacks = new MyCallbacks();//set callbacks for main service
  pCharacteristic->setCallbacks(pCallbacks); // Set the callbacks for the characteristic
    
  pService->start();
  
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID.c_str());
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Characteristic defined! Now you can read it in your phone!");
  
  
  // Set up backlight control pin
  pinMode(GPIO_NUM_9, OUTPUT); // Configure GPIO9 as an output pin
  //digitalWrite(TFT_CS, LOW);
  digitalWrite(GPIO_NUM_9, HIGH); // Turn on backlight (Set to HIGH
  tft.fillScreen(TFT_BLACK); // Fill the screen with black color

  // Set the font and text size for the deviceID
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  updateDisplay();
  //Sut up and down button to high or == 1 
  pinMode(14, INPUT_PULLUP);
  pinMode(pin10, INPUT_PULLUP);
  pinMode(pin11, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  pinMode(16, OUTPUT);
  
  mesh.setDebugMsgTypes(ERROR | DEBUG);  // set before init() so that you can see error messages
  mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.onNodeDelayReceived(&delayReceivedCallback);
  String OTAstatus = preferences.getString("OTA", OTA);
  
  if (OTAstatus == "receiver") {
    mesh.initOTAReceive("otareceiver"); // establish that device is an OTA reciever. This allows remote updating
  }
  
  userScheduler.addTask( taskSendMessage );
  taskSendMessage.enable();

  blinkNoNodes.set(BLINK_PERIOD, (mesh.getNodeList().size() + 1) * 2, []() {
      // If on, switch off, else switch on
      if (mesh.getNodeList().size() == 0){
        
      }
      if (onFlag)
        onFlag = false;
      else
        onFlag = true;
      blinkNoNodes.delay(BLINK_DURATION);

      if (blinkNoNodes.isLastIteration()) {
        // Finished blinking. Reset task for next run 
        // blink number of nodes (including this node) times
        blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
        // Calculate delay based on current mesh time and BLINK_PERIOD
        // This results in blinks between nodes being synced
        blinkNoNodes.enableDelayed(BLINK_PERIOD - 
            (mesh.getNodeTime() % (BLINK_PERIOD*1000))/1000);
      }
  });
  userScheduler.addTask(blinkNoNodes);
  blinkNoNodes.enable();
  randomSeed(analogRead(A0));  
  Keyboard.begin();
  USB.begin();
  delay(100);
  if (!SD.begin(34)) {
    //rebootEspWithReason("Could not mount SD card, restarting");
    Serial.println("opening SD");
    Serial.println("could not open SD card");
    return;
  }
  if (OTAstatus == "sender")  {
    
  
  //File dir = SD.open("/OTA/firmware_ESP32_otareceiver.bin");
  //while (true) {
    File entry = SD.open("/OTA/firmware_ESP32_otareceiver.bin");
    //if (!entry) { //End of files
    //  TSTRING name = entry.name();
    //  Serial.println(name);
    //  Serial.println("could not open SD card");
      //rebootEspWithReason("Could not find valid firmware, please validate");
    //  return;
    //}
  
    //This block of code parses the file name to make sure it is valid.
    //It will also get the role and hardware the firmware is targeted at.
    
  
          //This is the important bit for OTA, up to now was just getting the file. 
          //If you are using some other way to upload firmware, possibly from 
          //mqtt or something, this is what needs to be changed.
          //This function could also be changed to support OTA of multiple files
          //at the same time, potentially through using the pkg.md5 as a key in
          //a map to determine which to send
      mesh.initOTASend([&entry](painlessmesh::plugin::ota::DataRequest pkg,char* buffer) {
            
            //fill the buffer with the requested data packet from the node.
            entry.seek(OTA_PART_SIZE * pkg.partNo);
            entry.readBytes(buffer, OTA_PART_SIZE);
            
            //The buffer can hold OTA_PART_SIZE bytes, but the last packet may
            //not be that long. Return the actual size of the packet.
            return min((unsigned)OTA_PART_SIZE,entry.size() - (OTA_PART_SIZE * pkg.partNo));},OTA_PART_SIZE);
  
      //Calculate the MD5 hash of the firmware we are trying to send. This will be used
      //to validate the firmware as well as tell if a node needs this firmware.
      MD5Builder md5;
      md5.begin();
      md5.addStream(entry, entry.size());
      md5.calculate(); 
  
      //Make it known to the network that there is OTA firmware available.
      //This will send a message every minute for an hour letting nodes know
      //that firmware is available.
      //This returns a task that allows you to do things on disable or more,
      //like closing your files or whatever.
      mesh.offerOTA("otareceiver", "ESP32", md5.toString(),ceil(((float)entry.size()) / OTA_PART_SIZE), false);
  
      while (true) {
        //This program will not reach loop() so we dont have to worry about
        //file scope.
        mesh.update();
        usleep(1000);
      }
  }
};


#define MAX_DATA_LENGTH 300
void loop() {
  Serial.print("freeheap is: ");
  Serial.println(ESP.getFreeHeap());
  if (millis() - lastConnectionTime >= connectionInterval) {
    Serial.println("Re-establishing USB Connection...");
    USB.begin();
    Keyboard.begin();
    lastConnectionTime = millis();  // Update the last connection time
  }
  
  int pin14 = digitalRead(14);
  if (pin14 == 0) {
    if (configMode == true) {
    // Toggle configuration mode
      configMode = false;
      updateDisplay();
      delay(500);
    } else {
      changeConfig(deviceID);
    }
  }
  mesh.update();

  digitalWrite(LED, !onFlag);
  // Clear the screen
  tft.setTextColor(TFT_WHITE);
  if (Serial.available() > 0) {
    
    char receivedData[MAX_DATA_LENGTH]; // Character array to store received data
    int bytesRead = Serial.readBytes(receivedData, MAX_DATA_LENGTH - 1); // Read data into the array
    receivedData[bytesRead] = '\0'; // Null-terminate the character array

    // Create a String object from the character array
    String receivedString = String(receivedData);
    receivedString = processSerialMessages(receivedString);
    // Split the received message based on commas
    int startIndex = 0;
    int commaIndex = receivedString.indexOf(',');

    while (commaIndex != -1) {
        String individualMessage = receivedString.substring(startIndex, commaIndex);
        mesh.sendBroadcast(individualMessage);
        Serial.printf("Broadcasting individual message: %s\n", individualMessage);

        startIndex = commaIndex + 1;
        commaIndex = receivedString.indexOf(',', startIndex);
    }

    // Broadcast the remaining portion of the message
    String remainingMessage = receivedString.substring(startIndex);
    if (remainingMessage != "") {
      mesh.sendBroadcast(remainingMessage);
      Serial.printf("Broadcasting remaining message: %s\n", remainingMessage);
  
      if (messageCount >= maxMessages) {
          for (int i = 0; i < messageCount - 1; i++) {
              messages[i] = messages[i + 1];
          }
      } else {
          messageCount++;
      }
  
      // Add the new message to the last position in the messages array
      messages[messageCount - 1] = receivedString;
      updateDisplay();
    }
  }
  //}
  int valuePin10 = digitalRead(pin10);
  int valuePin11 = digitalRead(pin11);
  int valuePin14 = digitalRead(14);
  int valuePin0 = digitalRead(0);
  if (millis() - lastButtonCheckTime >= buttonCheckDelay) {
    // Update the lastButtonCheckTime
    lastButtonCheckTime = millis();
    
    // Check the value of pin 10 after the 300 milliseconds delay
    if (valuePin10 == 0) {
      // Button 1 (GPIO 10) pressed (low level)
      
      if (valuePin14 == 0 || configMode == true) {
        deviceID = String(deviceID.toInt() + 1); // Increment the device ID
        if (deviceID.toInt() < 10) {
          deviceID = String("00" + deviceID);
        } else { 
          deviceID = String("0" + deviceID);
        }
        
        changeConfig(deviceID); // Update display to show new device ID and save in preferences
        configMode = true;
      } else {
        String msg = preferences.getString("username", "haelab\n");
        Serial.print("Broadcasting Username: ");
        Serial.println(msg);
        mesh.sendBroadcast(msg, true);
        // Non-blocking delay for 300 milliseconds
        lastButtonCheckTime = millis();  // Update lastButtonCheckTime after broadcasting
      }
    }
  }
  
  // Check if password broadcast delay has passed
  if (millis() - lastPasswordBroadcastTime >= passwordBroadcastDelay) {
    lastPasswordBroadcastTime = millis();
    
    // Check if Button 2 (GPIO 11) is pressed
    if (valuePin11 == 0) {
      if ((valuePin14 == 0 || configMode == true ) && (deviceID != "000")) {
        deviceID = String(deviceID.toInt() - 1); // Decrement the device ID
        if (deviceID.toInt() < 10) {
          deviceID = String("00" + deviceID);
        } else {
          deviceID = String("0" + deviceID);
        }
        changeConfig(deviceID);
        configMode = true;
      } else {
        String msg = preferences.getString("password", "teg6s234\n");
        Serial.print("Broadcasting Password: ");
        Serial.println(msg);
        mesh.sendBroadcast(msg, true);
      }
    }
  }
  if (valuePin0 == 0) {
    Serial.begin(115200);
  // Initialize SD card
    if (!SD.begin(34)) { // Use the correct CS pin for your setup
      Serial.println("SD Card initialization failed!");
      return;
    }
    dataFile = SD.open(fileName, FILE_READ);
    if (dataFile) {
    Serial.println("Reading from " + String(fileName) + "...");
    String patientID = dataFile.readString(); // Read the entire file as a String
    dataFile.close();

    Serial.println("Patient ID: " + patientID);
    mesh.sendBroadcast(patientID, true);
    delay(500);
    } else {
      Serial.println("Error opening " + String(fileName));
      delay(500);
    }
  }
  
  if (characteristicChanged == true) {
    // Broadcast the new value to the network
    std::string value = pCharacteristic->getValue();
    String msg(value.c_str()); // Convert std::string to String
    mesh.sendBroadcast(msg, true);
    characteristicChanged = false;  // Reset the flag
  }
  if (mesh.getNodeList().size() + 1 == 1) {
    timeUnconnected++;
    Serial.printf("unconnected %i", timeUnconnected);
    digitalWrite(16, 1); 
  } else
  if (mesh.getNodeList().size() + 1 > 1) {
    timeUnconnected = 0;
    digitalWrite(16, 0);
  } 
  if ((timeUnconnected == 10000) && (deviceConnected == true)) {
    updateDisplay();
  }
  if ((timeUnconnected == 10000) && (deviceConnected == false) && (deviceID != "000")) {
  // Add other tasks or code you need to run within the loop
    ESP.restart();
    
  } else 
  return;
}


void sendMessage() {
  if (Serial.available() > 0) {
    char receivedData[MAX_DATA_LENGTH]; // Character array to store received data
    int bytesRead = Serial.readBytes(receivedData, MAX_DATA_LENGTH - 1); // Read data into the array
    receivedData[bytesRead] = '\0'; // Null-terminate the character array
    
    // Create a String object from the character array
    String receivedString = String(receivedData);
    String msg = receivedString;
    mesh.sendBroadcast(msg);
    mesh.sendBroadcast(receivedString);
    Serial.printf("serial input is %s", receivedString);
  }
  else  {
  String msg = "Hello from node ";
  msg += mesh.getNodeId();
  msg += " myFreeMemory: " + String(ESP.getFreeHeap());
  //mesh.sendBroadcast(msg);

  if (calc_delay) {
    SimpleList<uint32_t>::iterator node = nodes.begin();
    while (node != nodes.end()) {
      mesh.startDelayMeas(*node);
      node++;
    }
    calc_delay = false;
  }
  //old code to randomly send message for device network testing
  //Serial.printf("Sending message: %s\n", msg.c_str());
  //taskSendMessage.setInterval( random(TASK_SECOND * 1, TASK_SECOND * 5));  // between 1 and 5 seconds
  }

}


void updateConfigVariable(const String& configName, const String& configValue) {
  // Remove leading and trailing whitespaces from the received value
  String trimmedValue = configValue;
  trimmedValue.trim();

  // Update configuration variables based on the received configName
  if (configName == "deviceID") {
      deviceID = trimmedValue;
      preferences.putString("deviceID", deviceID);
  } else if (configName == "MESH_SSID") {
      // Update MESH_SSID
      MESH_SSID = trimmedValue;
      preferences.putString("MESH_SSID", MESH_SSID);
  } else if (configName == "MESH_PASSWORD") {
      MESH_PASSWORD = trimmedValue;
      preferences.putString("MESH_PASSWORD", MESH_PASSWORD);
  } else if (configName == "MESH_PORT") {
      // Convert the received string to an integer
      MESH_PORT = configValue.toInt();
      preferences.putString("MESH_PORT", String(MESH_PORT));
  } else if (configName == "username") {
      username = trimmedValue;
      preferences.putString("username", username);
  } else if (configName == "password") {
      password = trimmedValue;
      preferences.putString("password", password);
  } else {
      // Handle unknown configName or provide an error message

  }
}
String processSerialMessages(const String& receivedMessage) {
  String unmatchedMessage = "";  // Initialize an empty string for unmatched messages

  // Check if the received message format matches "configName: configValue"
  int colonIndex = receivedMessage.indexOf(':');
  if (colonIndex != -1) {
    String configName = receivedMessage.substring(0, colonIndex);
    String configValue = receivedMessage.substring(colonIndex + 1);

    // Check if configName matches any of the keys in preferences
    bool configNameExists = false;
    for (const ConfigVariable& variable : configVariables) {
      if (variable.name == configName) {
        configNameExists = true;
        break;
      }
    }

    if (configNameExists) {
      // Update configuration variables based on the received message
      updateConfigVariable(configName, configValue);
      updateDisplay();
      // Print updated values (optional)
      Serial.println(configName + " updated to " + configValue);
    } else {
      // Handle messages that don't match any preference keys
      // You can add your custom logic here for unmatched messages.
      unmatchedMessage = receivedMessage;
    }
  } else {
    unmatchedMessage = receivedMessage;
  }

  // Return the unmatched message (empty string if all matched)
  return unmatchedMessage;
}


void receivedCallback(uint32_t from, String & msg) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
  char buffer[200]; // Adjust the buffer size as per your message requirement
  snprintf(buffer, sizeof(buffer), "%s", msg.c_str());
  String firstChars = msg.substring(0, deviceID.length());
  // Check if the message should be processed based on device ID
  if (msg.charAt(0) == '0') {
    if (firstChars == deviceID) {
      // Remove the device ID from the message
      msg.remove(0, deviceID.length());
    } else {
      return; // Ignore the message
    }
  }
  if (messageCount >= maxMessages) {
    for (int i = 0; i < messageCount - 1; i++) {
      messages[i] = messages[i + 1];
    }
  } else {
    messageCount++;
  }
  // Add the new message to the last position in the messages array
  messages[messageCount - 1] = msg;
  //display the message
  updateDisplay();

  // Write the message to the keyboard
  Keyboard.write((const uint8_t *)msg.c_str(), msg.length());
}

void newConnectionCallback(uint32_t nodeId) {
  // Reset blink task
  onFlag = false;
  blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
  blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD*1000))/1000);
 
  Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
  Serial.printf("--> startHere: New Connection, %s\n", mesh.subConnectionJson(true).c_str());
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
  // Reset blink task
  onFlag = false;
  blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
  blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD*1000))/1000);
 
  nodes = mesh.getNodeList();
  
  Serial.printf("Num nodes: %d\n", nodes.size());
  Serial.printf("Connection list:");
  
  SimpleList<uint32_t>::iterator node = nodes.begin();
  while (node != nodes.end()) {
    Serial.printf(" %u", *node);
    node++;
  }
  
  Serial.println();
  calc_delay = true;
  updateDisplay(); 
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void delayReceivedCallback(uint32_t from, int32_t delay) {
  Serial.printf("Delay to node %u is %d us\n", from, delay);
}
void changeConfig(const String& receivedMessage) { //show screen with updated device ID and save preferences 
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_WHITE);
      tft.setTextSize(2); //initial text size
      tft.setCursor(3, 10);//starting spot for cursor to draw
      tft.print("Use DW- and UP+\nbuttons to change");
      tft.setTextSize(3);//change size of text to belarger
      tft.setCursor(3, 70);//move cursor down
      tft.print("DeviceID:" + receivedMessage); //write the device ID
      configMode = true;//set config mode to true if not already 
      delay(225);//delay between increment on button press
      preferences.putString("deviceID", deviceID);//store the new device ID 
      //if (deviceID == "000") {
      //  if (OTA == "sender"){
      //    OTA = "receiver";
      //  } else {
      //    OTA = "sender";
      //  }
      ///  preferences.putString("OTA", OTA);   
      ///} else {
       // preferences.putString("deviceID", deviceID);//store the new device ID    
      //}
}
void keyboardMode(const String& receivedMessage) { //show screen with updated device ID and save preferences 
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_WHITE);
      tft.setTextSize(2); //initial text size
      tft.setCursor(3, 10);//starting spot for cursor to draw
      tft.print("Use Keyboard to type text\n Press enter to send to network");
      tft.setTextSize(3);//change size of text to belarger
      tft.setCursor(3, 70);//move cursor down
      tft.print(receivedMessage); //write the device ID
      //configMode = true;//set config mode to true if not already 
      delay(225);//delay between increment on button press
      //preferences.putString("deviceID", deviceID);//store the new device ID
}
