#include <Preferences.h>
#include <painlessMesh.h>
#include <Arduino.h>
#include "USBHIDKeyboard.h"
#include <USBHID.h>
#include "driver/gpio.h"
#include <FS.h>
#include "SD.h"
#include "SPI.h"
#include <TFT_eSPI.h>
#include "usb/usb_host.h"
#include <elapsedMillis.h>
#include <usb/usb_host.h>
#include "show_desc.hpp"
#include "usbhhelp.hpp"

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
String deviceID = "023";
String MESH_SSID = "whateverYouLike";
String MESH_PASSWORD = "somethingSneaky";
int MESH_PORT = 5555;
String username = "haelab\n";
String password = "teg6s234\n";
String fileName = "/patientID.txt";

struct ConfigVariable {
  String name;
  String* variable;
};

// Create an array of configuration variables
ConfigVariable configVariables[] = {
  {"deviceID", &deviceID},
  {"MESH_SSID", &MESH_SSID},
  {"MESH_PASSWORD", &MESH_PASSWORD},
  {"MESH_PORT", (String*)&MESH_PORT}, // Convert int to String
  {"username", &username},
  {"password", &password},
  {"fileName", &fileName}
};
File dataFile;
// Prototypes
void sendMessage(); 
void receivedCallback(uint32_t from, String & msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback(); 
void nodeTimeAdjustedCallback(int32_t offset); 
void delayReceivedCallback(uint32_t from, int32_t delay);

TFT_eSPI tft = TFT_eSPI(); //instantiate instance or TFT display
USBHIDKeyboard Keyboard; //create keyboard object

Scheduler     userScheduler; // to control your personal task
painlessMesh  mesh;
Preferences preferences;

SimpleList<uint32_t> nodes;

void sendMessage() ; // Prototype
Task taskSendMessage( TASK_SECOND * 1, TASK_FOREVER, &sendMessage ); // start with a one second interval

// Task to blink the number of nodes
Task blinkNoNodes;
bool calc_delay = false;
bool onFlag = false;
bool configMode = false;
bool keyboardFlag = false;// Flag to indicate if the device is in configuration mode
bool isKeyboardPolling = false;
bool isKeyboard = false;
bool isKeyboardReady = false;


const int pin10 = 10;
const int pin11 = 11;
const int maxMessages = 4; // Maximum number of messages to buffer
String messages[maxMessages]; // Buffer to store received messages
int messageCount = 0; // Number of messages in the buffer
int currentMessage = 0; // Index of the message to print next
int timeUnconnected = 0;

const int GPIO_BUTTON_MENU = 14;

uint8_t KeyboardInterval;

elapsedMillis KeyboardTimer;

const size_t KEYBOARD_IN_BUFFER_SIZE = 8;
usb_transfer_t *KeyboardIn = NULL;


char receivedKeys[64]; // A buffer to store received keys
int receivedKeysLength = 0; // Length of received key string
#define KEYBOARD_ENTER_MAIN_CHAR    '\r'
/* When set to 1 pressing ENTER will be extending with LineFeed during serial debug output */
#define KEYBOARD_ENTER_LF_EXTEND    1
const uint8_t keycode2ascii [57][2] = {
    {0, 0}, /* HID_KEY_NO_PRESS        */
    {0, 0}, /* HID_KEY_ROLLOVER        */
    {0, 0}, /* HID_KEY_POST_FAIL       */
    {0, 0}, /* HID_KEY_ERROR_UNDEFINED */
    {'a', 'A'}, /* HID_KEY_A               */
    {'b', 'B'}, /* HID_KEY_B               */
    {'c', 'C'}, /* HID_KEY_C               */
    {'d', 'D'}, /* HID_KEY_D               */
    {'e', 'E'}, /* HID_KEY_E               */
    {'f', 'F'}, /* HID_KEY_F               */
    {'g', 'G'}, /* HID_KEY_G               */
    {'h', 'H'}, /* HID_KEY_H               */
    {'i', 'I'}, /* HID_KEY_I               */
    {'j', 'J'}, /* HID_KEY_J               */
    {'k', 'K'}, /* HID_KEY_K               */
    {'l', 'L'}, /* HID_KEY_L               */
    {'m', 'M'}, /* HID_KEY_M               */
    {'n', 'N'}, /* HID_KEY_N               */
    {'o', 'O'}, /* HID_KEY_O               */
    {'p', 'P'}, /* HID_KEY_P               */
    {'q', 'Q'}, /* HID_KEY_Q               */
    {'r', 'R'}, /* HID_KEY_R               */
    {'s', 'S'}, /* HID_KEY_S               */
    {'t', 'T'}, /* HID_KEY_T               */
    {'u', 'U'}, /* HID_KEY_U               */
    {'v', 'V'}, /* HID_KEY_V               */
    {'w', 'W'}, /* HID_KEY_W               */
    {'x', 'X'}, /* HID_KEY_X               */
    {'y', 'Y'}, /* HID_KEY_Y               */
    {'z', 'Z'}, /* HID_KEY_Z               */
    {'1', '!'}, /* HID_KEY_1               */
    {'2', '@'}, /* HID_KEY_2               */
    {'3', '#'}, /* HID_KEY_3               */
    {'4', '$'}, /* HID_KEY_4               */
    {'5', '%'}, /* HID_KEY_5               */
    {'6', '^'}, /* HID_KEY_6               */
    {'7', '&'}, /* HID_KEY_7               */
    {'8', '*'}, /* HID_KEY_8               */
    {'9', '('}, /* HID_KEY_9               */
    {'0', ')'}, /* HID_KEY_0               */
    {KEYBOARD_ENTER_MAIN_CHAR, KEYBOARD_ENTER_MAIN_CHAR}, /* HID_KEY_ENTER           */
    {0, 0}, /* HID_KEY_ESC             */
    {'\b', 0}, /* HID_KEY_DEL             */
    {0, 0}, /* HID_KEY_TAB             */
    {' ', ' '}, /* HID_KEY_SPACE           */
    {'-', '_'}, /* HID_KEY_MINUS           */
    {'=', '+'}, /* HID_KEY_EQUAL           */
    {'[', '{'}, /* HID_KEY_OPEN_BRACKET    */
    {']', '}'}, /* HID_KEY_CLOSE_BRACKET   */
    {'\\', '|'}, /* HID_KEY_BACK_SLASH      */
    {'\\', '|'}, /* HID_KEY_SHARP           */  // HOTFIX: for NonUS Keyboards repeat HID_KEY_BACK_SLASH
    {';', ':'}, /* HID_KEY_COLON           */
    {'\'', '"'}, /* HID_KEY_QUOTE           */
    {'`', '~'}, /* HID_KEY_TILDE           */
    {',', '<'}, /* HID_KEY_LESS            */
    {'.', '>'}, /* HID_KEY_GREATER         */
    {'/', '?'} /* HID_KEY_SLASH           */
};

char mapToUSKeyboard(uint8_t receivedKey) {
  // Check if the received key is within the valid range
  if (receivedKey >= 0 && receivedKey < 57) {
    char asciiChar = keycode2ascii[receivedKey][0];
    if (asciiChar != 0) {
      return asciiChar;
    }
  }
  // If the key is not found in the table or the mapping is 0, return 0 or handle it as needed.
  return 0; // You can choose to handle this differently if needed.
}

void keyboard_transfer_cb(usb_transfer_t *transfer)
{
  if (Device_Handle == transfer->device_handle) {
    isKeyboardPolling = false;
    if (transfer->status == 0) {
      if (transfer->actual_num_bytes == 8) {
        uint8_t *const p = transfer->data_buffer;
        for (int i = 0; i < 8; i++) {
          receivedKeys[receivedKeysLength] = p[i];
          receivedKeysLength++;
        }
      }
      else {
        Serial.println("Keyboard boot hid transfer too short or long");
      }
    }
    else {
      ESP_LOGI("", "transfer->status %d", transfer->status);
    }
  }
}

void check_interface_desc_boot_keyboard(const void *p)
{
  const usb_intf_desc_t *intf = (const usb_intf_desc_t *)p;

  if ((intf->bInterfaceClass == USB_CLASS_HID) &&
      (intf->bInterfaceSubClass == 1) &&
      (intf->bInterfaceProtocol == 1)) {
    isKeyboard = true;
    ESP_LOGI("", "Claiming a boot keyboard!");
    esp_err_t err = usb_host_interface_claim(Client_Handle, Device_Handle,
        intf->bInterfaceNumber, intf->bAlternateSetting);
    if (err != ESP_OK) ESP_LOGI("", "usb_host_interface_claim failed: %x", err);
  }
}

void prepare_endpoint(const void *p)
{
  const usb_ep_desc_t *endpoint = (const usb_ep_desc_t *)p;
  esp_err_t err;

  // must be interrupt for HID
  if ((endpoint->bmAttributes & USB_BM_ATTRIBUTES_XFERTYPE_MASK) != USB_BM_ATTRIBUTES_XFER_INT) {
    ESP_LOGI("", "Not interrupt endpoint: 0x%02x", endpoint->bmAttributes);
    return;
  }
  if (endpoint->bEndpointAddress & USB_B_ENDPOINT_ADDRESS_EP_DIR_MASK) {
    err = usb_host_transfer_alloc(KEYBOARD_IN_BUFFER_SIZE, 0, &KeyboardIn);
    if (err != ESP_OK) {
      KeyboardIn = NULL;
      ESP_LOGI("", "usb_host_transfer_alloc In fail: %x", err);
      return;
    }
    KeyboardIn->device_handle = Device_Handle;
    KeyboardIn->bEndpointAddress = endpoint->bEndpointAddress;
    KeyboardIn->callback = keyboard_transfer_cb;
    KeyboardIn->context = NULL;
    isKeyboardReady = true;
    KeyboardInterval = endpoint->bInterval;
    ESP_LOGI("", "USB boot keyboard ready");
  }
  else {
    ESP_LOGI("", "Ignoring interrupt Out endpoint");
  }
}

void show_config_desc_full(const usb_config_desc_t *config_desc)
{
  // Full decode of config desc.
  const uint8_t *p = &config_desc->val[0];
  static uint8_t USB_Class = 0;
  uint8_t bLength;
  for (int i = 0; i < config_desc->wTotalLength; i+=bLength, p+=bLength) {
    bLength = *p;
    if ((i + bLength) <= config_desc->wTotalLength) {
      const uint8_t bDescriptorType = *(p + 1);
      switch (bDescriptorType) {
        case USB_B_DESCRIPTOR_TYPE_DEVICE:
          ESP_LOGI("", "USB Device Descriptor should not appear in config");
          break;
        case USB_B_DESCRIPTOR_TYPE_CONFIGURATION:
          show_config_desc(p);
          break;
        case USB_B_DESCRIPTOR_TYPE_STRING:
          ESP_LOGI("", "USB string desc TBD");
          break;
        case USB_B_DESCRIPTOR_TYPE_INTERFACE:
          USB_Class = show_interface_desc(p);
          check_interface_desc_boot_keyboard(p);
          break;
        case USB_B_DESCRIPTOR_TYPE_ENDPOINT:
          show_endpoint_desc(p);
          if (isKeyboard && KeyboardIn == NULL) prepare_endpoint(p);
          break;
        case USB_B_DESCRIPTOR_TYPE_DEVICE_QUALIFIER:
          // Should not be config config?
          Serial.println("USB device qual desc TBD");
          break;
        case USB_B_DESCRIPTOR_TYPE_OTHER_SPEED_CONFIGURATION:
          // Should not be config config?
          Serial.println("USB Other Speed TBD");
          break;
        case USB_B_DESCRIPTOR_TYPE_INTERFACE_POWER:
          // Should not be config config?
          Serial.println("USB Interface Power TBD");
          break;
        case 0x21:
          if (USB_Class == USB_CLASS_HID) {
            show_hid_desc(p);
          }
          break;
        default:
          //Serial.println("Unknown USB Descriptor Type: %s", bDescriptorType);
          break;
      }
    }
    else {
      Serial.println("USB Descriptor invalid");
      return;
    }
  }
}

void setupConfigVariables() {
  // Begin the preferences storage with a namespace
  preferences.begin("configNamespace", false);

  // Initialize and retrieve each configuration variable

  // Initialize and retrieve deviceID
  if (preferences.isKey("deviceID")) {
    deviceID = preferences.getString("deviceID", deviceID);
  } else {
    // Set the default value if deviceID doesn't exist in preferences
    preferences.putString("deviceID", deviceID);
  }

  // Initialize and retrieve MESH_SSID
  if (preferences.isKey("MESH_SSID")) {
    MESH_SSID = preferences.getString("MESH_SSID", MESH_SSID);
  } else {
    // Set the default value if MESH_SSID doesn't exist in preferences
    preferences.putString("MESH_SSID", MESH_SSID);
  }

  // Initialize and retrieve MESH_PASSWORDhhjykhhujkhyukhkhukhuuuuuuuuu
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
String receivedString = "";
void processReceivedKeys(uint8_t receivedKeys[], int receivedKeysLength) {
  Serial.print("Received Keys: ");
  
  for (int i = 0; i < receivedKeysLength; i++) {
    uint8_t receivedCode = receivedKeys[i];
    char receivedChar = mapToUSKeyboard(receivedCode);
    
    if (receivedChar == '\0') {
      // Handle null characters or special modifiers as needed
      // You can add special logic here
    } else if (receivedChar == '\r') {
      // Handle Enter key: broadcast the received string
      mesh.sendBroadcast(receivedString);
      receivedString = ""; // Reset the received string
      keyboardMode(receivedString);
    } else if (receivedChar == '\b') {
      // Handle backspace: remove the last character from receivedString
      if (!receivedString.isEmpty()) {
        receivedString.remove(receivedString.length() - 1);
        keyboardMode(receivedString);
      }
    } else {
      // Add the received character to the string
      receivedString += receivedChar;
      keyboardMode(receivedString);
    }
  }// Print a newline after processing the received keys
}

void setup() {

  Serial.begin( 115200 );
  pinMode(5, OUTPUT);
  digitalWrite(5, HIGH);
  pinMode(34, OUTPUT);
  digitalWrite(34, HIGH);
  tft.init();
  tft.setRotation(3);

  setupConfigVariables();
  Serial.begin(9600);
  //tft.init();
  //tft.setRotation(3); // Adjust rotation if needed
  usbh_setup(show_config_desc_full);

  // Set up backlight control pin
  pinMode(TFT_BL, OUTPUT); // Configure GPIO9 as an output pin
  //digitalWrite(TFT_CS, LOW);
  digitalWrite(TFT_BL, HIGH); // Turn on backlight (Set to HIGH
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
}

#define MAX_DATA_LENGTH 500
void loop() {
  int pin14 = digitalRead(14);
  int pin0 = digitalRead(0);
  //if menu button pressed
  if (pin14 == 0) {
      if (configMode == true) {
      // Toggle configuration mode
        
        configMode = false;
        updateDisplay();
        keyboardFlag = false;
        delay(500);
      } else {
      changeConfig(deviceID);
      }
  }
  if (pin0 == 0 && pin14 == 0 ) {
    //if already in heyboard mode, exit keyboard mode
    if (keyboardFlag = true) {
      keyboardFlag=false;
      updateDisplay();
      pinMode(17, OUTPUT);
      digitalWrite(17, LOW);
      pinMode(12, OUTPUT);
      digitalWrite(12, LOW);
      pinMode(13, OUTPUT);
      digitalWrite(13, LOW);
      pinMode(18, OUTPUT);
      digitalWrite(18, LOW);
      delay(500);
    } 
    keyboardFlag = true;
    keyboardMode(receivedKeys);  

        
 // } else if (keyboardFlag == true && configMode == false) {
 //   keyboardMode(receivedKeys);
  } else {

  mesh.update();
  //usbHost.task(); 
  usbh_task();
  
  if (isKeyboardReady && !isKeyboardPolling && (KeyboardTimer > KeyboardInterval)) {
    KeyboardIn->num_bytes = 8;
    esp_err_t err = usb_host_transfer_submit(KeyboardIn);
    if (err != ESP_OK) {
      //Serial.println("usb_host_transfer_submit In fail: %c", err);
    }
    isKeyboardPolling = true;
    KeyboardTimer = 0;
  }

  if (keyboardFlag == true){
    processReceivedKeys((uint8_t*)receivedKeys, receivedKeysLength);
    Serial.print("Received Keys: ");
    for (int i = 0; i < receivedKeysLength; i++) {
      uint8_t receivedCode = receivedKeys[i];
      char receivedChar = mapToUSKeyboard(receivedCode);
      Serial.print(receivedChar); // Print the received character
      Serial.println();
    }
  }
  // Process the received keystrokes or perform other tasks
  // Here, we print the received keys to the Serial Monitor
  if (receivedKeysLength > 0) {
  receivedKeys[receivedKeysLength] = '\0'; // Null-terminate the string

  // For example, you can send the keys over the mesh network
  receivedKeysLength = 0; // Reset the received keys buffer
  }
    
    // Other tasks and mesh network handling
  
  digitalWrite(LED, !onFlag);
  //set text color
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
  if (valuePin10 == 0) { // Button 1 (GPIO 10) pressed (low level)
    if (valuePin14 == 0 || configMode == true) {
      deviceID = String(deviceID.toInt() + 1); // increment the device ID
      if (deviceID.toInt() < 10) {
        deviceID = String("00" + deviceID);
      } else {
      deviceID = String("0" + deviceID);
      }
      changeConfig(deviceID); //update display to show new device ID and save in preferences
      configMode = true;
    } else {
    
    String msg = preferences.getString("username", "haelab\n");
    mesh.sendBroadcast(msg, true);
    msg = "";
    delay(3000);
    
  }
  }
  if (valuePin11 == 0) { // Button 2 (GPIO 11) pressed (low level)

    if (valuePin14 == 0 || configMode == true) {
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
    mesh.sendBroadcast(msg, true);
    // Debounce delay
    msg = "";
    delay(3000);
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
  if (mesh.getNodeList().size() + 1 == 1) {
    timeUnconnected++;
    Serial.printf("unconnected %i", timeUnconnected);
    digitalWrite(16, 1); 
  } else
  if (mesh.getNodeList().size() + 1 > 1) {
    timeUnconnected = 0;
    digitalWrite(16, 0);
  } 
  if (timeUnconnected == 2500) {
  // Add other tasks or code you need to run within the loop
    ESP.restart();
    
  } else 
  return;
}
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
  char buffer[100]; // Adjust the buffer size as per your message requirement
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
  // Reset blink taskf
  onFlag = false;
  blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
  blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD*1000))/1000);
 
  nodes = mesh.getNodeList();
  
  Serial.printf("Num nodes: %d\n", nodes.size());
  Serial.printf("Connection list:");
  //write list of nodes to serial monitor 
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
      tft.setCursor(5, 70);//move cursor down
      tft.print("DeviceID:" + receivedMessage); //write the device ID
      configMode = true;//set config mode to true if not already 
      delay(225);//delay between increment on button press
      preferences.putString("deviceID", deviceID);//store the new device ID
}
// Include the USB HID library for ESP32


// Function to handle USB HID keyboard input


void keyboardMode(const String& receivedString) { //show screen with updated device ID and save preferences 
      pinMode(17, OUTPUT);
      digitalWrite(17, HIGH);
      pinMode(12, OUTPUT);
      digitalWrite(12, HIGH);
      pinMode(13, OUTPUT);
      digitalWrite(13, HIGH);
      pinMode(18, OUTPUT);
      digitalWrite(18, HIGH); 
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_WHITE);
      tft.setTextSize(2); //initial text size
      tft.setCursor(3, 10);//starting spot for cursor to draw
      tft.print("Use Keyboard to type text\n Press enter to send to network");
      tft.setTextSize(3);//change size of text to belarger
      tft.setCursor(3, 120);//move cursor down
      tft.print(receivedString); //write the string
      keyboardFlag = true;
      //configMode = true;//set config mode to true if not already 
      delay(225);//delay between increment on button press
      //preferences.putString("deviceID", deviceID);//store the new device ID
}
void updateDisplay() { //call this function to go back to the main viewport
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

  // Set cursor for message display
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(5, deviceID_y + 40);

  // Calculate starting message index based on the current message count and display limit
  int startIndex = max(0, messageCount - maxDisplayedLines);

  for (int i = startIndex; i < messageCount; i++) {
    int index = (currentMessage + i) % maxMessages;
    tft.setTextSize(2);
    if (i == messageCount - 1) {
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
