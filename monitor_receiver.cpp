/**
  Besturingssoftware Thuiszorgmonitor
  Naam: monitor_receiver.cpp
  Purpose:

  @authors
  @version
*/


// libraries to be included
#include <ESP8266WiFi.h>        // Include the Wi-Fi library


// program variables (constants) declaration
#define LED_BUILTIN_PIN 2           // buildin led pin

// acces point configuration
const char *ssid = "RECEIVER_1";    // name of wifi network to be created
const char *password = "123456789";           // password of network -> pasword generator?

// TCP server
WiFiServer wifiServer(80);


const byte serial_number_of_characters = 32;          // predefined max buffer size
char serial_received_characters[serial_number_of_characters];     // buffer for received messages
String serial_received_request[4];
boolean serial_new_data = false;           // bool to check if message is complete

const byte wireless_number_of_characters = 32;          // predefined max buffer size
char wireless_received_characters[serial_number_of_characters];     // buffer for received messages
String wireless_received_request[4];
boolean wireless_new_data = false;           // bool to check if message is complete

// initial function declarations
void receiveSerial();
void actOnNewSerialData();
void receiveWireless(WiFiClient client);
void actOnNewWirelessData();

// program operating variables





void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println("\n\n***************************************************\n");
  Serial.println("                ESP started running\n");
  Serial.println("***************************************************\n");

  WiFi.softAP(ssid, password);             // Start the access point
  Serial.print("Access Point \"");
  Serial.print(ssid);
  Serial.println("\" started");
  Serial.print("IP address:\t");
  Serial.println(WiFi.softAPIP());         // Send the IP address of the ESP8266 to the computer

  wifiServer.begin();                      // Start TCP server

  pinMode(LED_BUILTIN_PIN, OUTPUT);
  digitalWrite(LED_BUILTIN_PIN, HIGH);

}

void loop() {
//  exploreSurroundingReceivers
//  getClosedReceiver
//  buildNetwork

//  waitForDevices
//    getDataFromDivice
//    checkData

//    sendDataToRoot of decryptData










  receiveSerial();
  actOnNewSerialData();



  // wait for a new client:
  WiFiClient client = wifiServer.available();

  // when the client sends the first byte, say hello:
  if (client) {
    if (client.connected()) {
      Serial.printf("connected");
    }
  }


}




/**
  Function for handeling incomming characters from serial connection

  @param None
  @return None
*/
void receiveSerial() {
  static byte character_index = 0;
  char end_marker = '\n';
  char new_character;

  if (Serial.available() > 0) {
    while (Serial.available() > 0 && serial_new_data == false) {
      new_character = Serial.read();
      if (new_character != end_marker) {
        serial_received_characters[character_index] = new_character;
        character_index++;
        if (character_index >= serial_number_of_characters) {  // if buffer is full, last
          character_index = serial_number_of_characters - 1;  // character gets overwritten
        }
      }
      else {
        serial_received_characters[character_index] = '\0'; // terminate the string
        character_index = 0;
        serial_new_data = true;
      }
    }
  }
}


/**
  Function for handeling received serial message

  @param None
  @return None
*/
void actOnNewSerialData() {
  if (serial_new_data == true) {
    char *p = serial_received_characters;
    char *part_of_message;
    int current_index = 0;
    while ((part_of_message = strtok_r(p, "/", &p)) != NULL) {
      serial_received_request[current_index] = part_of_message;
      Serial.println(part_of_message);

      if (String(part_of_message) == "test") {
        digitalWrite(LED_BUILTIN_PIN, LOW);  //temp test
      }



      current_index ++;
    }
  serial_new_data = false;
  }
}


/**
  Function for handeling incomming characters from wireless connection

  @param None
  @return None
*/
void receiveWireless(WiFiClient client) {
  static byte character_index = 0;
  char end_marker = '\n';
  char new_character;

  if (client.available() > 0) {
    digitalWrite(LED_BUILTIN_PIN, LOW);  //temp test
    while (client.available() > 0 && wireless_new_data == false) {
      new_character = client.read();
      if (new_character != end_marker) {
        wireless_received_characters[character_index] = new_character;
        character_index++;
        if (character_index >= wireless_number_of_characters) {  // if buffer is full, last
          character_index = wireless_number_of_characters - 1;  // character gets overwritten
        }
      }
      else {
        wireless_received_characters[character_index] = '\0'; // terminate the string
        character_index = 0;
        wireless_new_data = true;
      }
    }
  }
}

/**
  Function for handeling received wireless message

  @param None
  @return None
*/
void actOnNewWirelessData() {
  if (wireless_new_data == true) {
    char *p = wireless_received_characters;
    char *part_of_message;
    int current_index = 0;
    while ((part_of_message = strtok_r(p, "/", &p)) != NULL) {
      wireless_received_request[current_index] = part_of_message;
      Serial.println(part_of_message);
      current_index ++;
    }
  wireless_new_data = false;
  }
}


void sendSerialMessage(String message) {
  Serial.print(message);
}

void sendWirelessMessage(String message) {

}
