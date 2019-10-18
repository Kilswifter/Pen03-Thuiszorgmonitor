/**
  Besturingssoftware Thuiszorgmonitor
  Naam: monitor_receiver.cpp
  Purpose:

  @authors
  @version
*/

// libraries to be included
#include <ESP8266WiFi.h>        // Include the Wi-Fi library
#include "uMQTTBroker.h"        // Include MQTT broker library

// pin declarations
#define LED_BUILTIN_PIN 2           // buildin led pin

// changeable parameters
bool DEBUG = false;
const int bits_per_measure = 16;
const int bits_per_encryption = 128;
const int measurements_per_encryption = bits_per_encryption / bits_per_measure;

// wifi AP config
char ssid[] = "ESP_receiver_1";
char pass[] = "123456789";
bool WiFiAP = true;

// functions pre-declarations
void startWiFiClient();
void startWiFiAP();
void receiveSerial();
void actOnNewSerialData();
void printIntArray(int *array);
void printArray(char *array);
void decryptData(char *data_to_decrypt, int *decrypted_data);
void addDataToBuffer(int *decrypted_data, int *measurement_buffer, const byte buffer_length, int &buffer_index, bool &is_buffer_empty);
void sendBuffer(String topic, int *buffer, const byte buffer_length, int &buffer_index, bool &is_buffer_empty);
void resetMeasurementBuffer(int *buffer, const byte buffer_length, int &buffer_index);
void debugPrintLn(String str);
void debugPrint(String str);


/**
String received_data_backlog_1[1024];
int backlog_index_1 = 0;
char received_data_backlog_2[1024];
int backlog_index = 0;
**/

// measurement buffers
const byte measurement_1_buffer_length = 16;
int measurement_1_buffer_index = 0;
int measurement_1_buffer[measurement_1_buffer_length];
bool is_buffer_1_empty = true;

// communication buffers
const byte serial_buffer_length = 32;          // predefined max buffer size
char serial_buffer[serial_buffer_length];     // buffer for received messages

String serial_received_request[4];
boolean serial_new_data = false;           // bool to check if message is complete

// global run variables
bool send_live_data = true;


// mqtt broker class
class myMQTTBroker: public uMQTTBroker
{
public:
    virtual bool onConnect(IPAddress addr, uint16_t client_count) {
      debugPrintLn(addr.toString() + " connected");
      return true;
    }

    virtual bool onAuth(String username, String password) {  // not used
      debugPrintLn("Username/Password: " + username + "/" + password);
      return true;
    }

    virtual void onData(String topic, const char *data, uint32_t length) {
      char data_str[length+1];
      os_memcpy(data_str, data, length);
      data_str[length] = '\0';  // final received message

      debugPrintLn("Received topic '" + topic + "' with data '" + (String)data_str + "'");
      Serial.println((String)data_str);

      if (topic == "ECG") {
        debugPrintLn("ECG received");
        char *data_to_decrypt = data_str;  // copy of received data
        int decrypted_data[measurements_per_encryption];  // empty array for decrypted message
        decryptData(data_to_decrypt, decrypted_data);  // decrypting message
        printIntArray(decrypted_data);

        addDataToBuffer(decrypted_data, measurement_1_buffer, measurement_1_buffer_length, measurement_1_buffer_index, is_buffer_1_empty);
        printIntArray(measurement_1_buffer);
        //received_data_backlog_1[backlog_index_1] = (String)data;
        //backlog_index_1 ++;
      }
    }
};

myMQTTBroker myBroker;


void setup()
{
  Serial.begin(115200);
  delay(10);
  debugPrintLn("\n\n***************************************************\n");
  debugPrintLn("                ESP started running\n");
  debugPrintLn("***************************************************\n");

  // Start WiFi
  debugPrintLn("Starting network connection");
  if (WiFiAP)
    startWiFiAP();
  else
    startWiFiClient();

  resetMeasurementBuffer(measurement_1_buffer, measurement_1_buffer_length, measurement_1_buffer_index);

  // Start the broker
  debugPrintLn("Starting MQTT broker");
  myBroker.init();

  // subscribe to all topics
  myBroker.subscribe("#");
}



void loop()
{

  //myBroker.publish("broker/counter", (String)counter++);
  receiveSerial();
  actOnNewSerialData();

  if (send_live_data) {

    sendBuffer("ECG", measurement_1_buffer, measurement_1_buffer_length, measurement_1_buffer_index, is_buffer_1_empty);
  }


  delay(10);
}



/**
################################################################################

                                Functions

################################################################################
**/



void startWiFiClient()
{
  debugPrintLn("Connecting to "+(String)ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    debugPrintLn(".");
  }
  debugPrintLn("");

  debugPrintLn("WiFi connected");
  debugPrintLn("IP address: " + WiFi.localIP().toString());
}

void startWiFiAP()
{
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, pass);
  debugPrintLn("AP started");
  debugPrintLn("IP address: " + WiFi.softAPIP().toString());
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
        serial_buffer[character_index] = new_character;
        character_index++;
        if (character_index >= serial_buffer_length) {  // if buffer is full, last
          character_index = serial_buffer_length - 1;  // character gets overwritten
        }
      }
      else {
        serial_buffer[character_index] = '\0'; // terminate the string
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
    char *p = serial_buffer;
    char *part_of_message;
    int current_index = 0;
    while ((part_of_message = strtok_r(p, "/", &p)) != NULL) {
      serial_received_request[current_index] = part_of_message;
      debugPrintLn(part_of_message);

      if (String(part_of_message) == "test") {
        digitalWrite(LED_BUILTIN_PIN, LOW);  //temp test
      }



      current_index ++;
    }
  serial_new_data = false;
  }
}


void decryptData(char *data_to_decrypt, int *decrypted_data) {
  debugPrint("Decrypting data : [");
  debugPrint(data_to_decrypt);
  debugPrintLn("]");

  for (int i=0; i<8; i++) {
    decrypted_data[i] = i;
  }

  //Serial.println("Decrypted message : [" + String(decrypted_data) + "]");
}



void addDataToBuffer(int *decrypted_data, int *measurement_buffer, const byte buffer_length, int &buffer_index, bool &is_buffer_empty) {
  debugPrintLn("Adding data to buffer");
  if ((buffer_length - buffer_index) < measurements_per_encryption) {
    debugPrintLn("Buffer out of space!");
    return;
  }
  int max_i = measurements_per_encryption;
  for (int i=0; i<max_i; i++) {
    measurement_buffer[buffer_index] = decrypted_data[i];
    buffer_index++;
  }
  is_buffer_empty = false;
}

void sendBuffer(String topic, int *buffer, const byte buffer_length, int &buffer_index, bool &is_buffer_empty) {
  if (is_buffer_empty) {
    return;
  }
  Serial.println(topic);
  for(int i = 0; i < buffer_length; i++) {
    if (buffer[i] != -1) {
      Serial.println(buffer[i]);
    }

  }
  Serial.println("END" + topic);
  resetMeasurementBuffer(buffer, buffer_length, buffer_index);
  is_buffer_empty = true;



}

void resetMeasurementBuffer(int *buffer, const byte buffer_length, int &buffer_index) {
  for(int i = 0; i < buffer_length; i++) {
    buffer[i] = -1;
  }
  buffer_index = 0;
}



void printIntArray(int *array) {
  int size_of_array = measurements_per_encryption; //sizeof(array)/sizeof(int);
  for(int i = 0; i < size_of_array; i++) {
    debugPrint("[");
    debugPrint((String)i);
    debugPrint("] - ");
    debugPrintLn((String)array[i]);
  }
  debugPrintLn("");
}


void printArray(char *array) {
  int size_of_array = strlen(array);
  for(int i = 0; i < size_of_array; i++) {
    debugPrint((String)array[i]);
  }
  debugPrintLn("");
}

void debugPrintLn(String str) {
  if (DEBUG) {
    Serial.println(str);
  }
}

void debugPrint(String str) {
  if (DEBUG) {
    Serial.print(str);
  }
}
