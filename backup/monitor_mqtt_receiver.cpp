/**
  Besturingssoftware Thuiszorgmonitor
  Naam: monitor_receiver.cpp
  Purpose:

  @authors Thomas Sergeys
  @version v1.0
*/

// libraries to be included
#include <ESP8266WiFi.h>        // Include the Wi-Fi library
#include "uMQTTBroker.h"        // Include MQTT broker library

// pin declarations
#define LED_BUILTIN_PIN 2           // buildin led pin

// changeable parameters
bool DEBUG = true;
const int bits_per_int = 16;
const int bits_per_measurement = 10;
const int bits_per_encryption = 128;
const int bit_groups = bits_per_encryption / bits_per_int; // 8
const int measurement_groups = 12;

// wifi AP config
char ssid[] = "ESP_reciever_1";
char pass[] = "123456789";
bool WiFiAP = true;

// functions pre-declarations
void startWiFiClient();
void startWiFiAP();
void receiveSerial();
void actOnNewSerialData();
void printIntArray(uint16_t *array, const int size_of_array);
void printArray(char *array);
void deUnicodeData(char *data_to_deUnicode, uint16_t *deUnicoded_data);
void unSplitBuffer(uint16_t *before_buffer, uint16_t *after_buffer, const int after_buffer_length);
void decryptData(uint16_t *data_to_decrypt, uint16_t *decrypted_data);
void deshiftData(uint16_t *shifted_buffer, const int shifted_buffer_length, uint16_t *deshifted_buffer, int deshifted_buffer_length);
void addDataToBuffer(uint16_t *deshifted_data, uint16_t *measurement_buffer, const int buffer_length, int &buffer_index, bool &is_buffer_empty);
void sendBuffer(String topic, uint16_t *buffer, const int buffer_length, int &buffer_index, bool &is_buffer_empty);
void resetMeasurementBuffer(uint16_t *buffer, const int buffer_length, int &buffer_index, bool &is_buffer_empty);
void debugPrintLn(String str);
void debugPrint(String str);



// measurement buffers
//  1 = ECG
const int measurement_1_buffer_length = 128;
int measurement_1_buffer_index = 0;
uint16_t measurement_1_buffer[measurement_1_buffer_length];
bool is_buffer_1_empty = true;
// 2 = PPG
const int measurement_2_buffer_length = 128;
int measurement_2_buffer_index = 0;
uint16_t measurement_2_buffer[measurement_2_buffer_length];
bool is_buffer_2_empty = true;
// 3 = zweet
const int measurement_3_buffer_length = 128;
int measurement_3_buffer_index = 0;
uint16_t measurement_3_buffer[measurement_3_buffer_length];
bool is_buffer_3_empty = true;
// 4 = voetdruk
const int measurement_4_buffer_length = 128;
int measurement_4_buffer_index = 0;
uint16_t measurement_4_buffer[measurement_4_buffer_length];
bool is_buffer_4_empty = true;

const int sensor_count = 4;

int measurement_n_buffer_length[sensor_count] = {measurement_1_buffer_length,
                                                 measurement_2_buffer_length,
                                                 measurement_3_buffer_length,
                                                 measurement_4_buffer_length};
uint16_t* measurement_n_buffer[sensor_count] = {measurement_1_buffer,
                                           measurement_2_buffer,
                                           measurement_3_buffer,
                                           measurement_4_buffer};
int measurement_n_buffer_index[sensor_count] = {measurement_1_buffer_index,
                                                measurement_2_buffer_index,
                                                measurement_3_buffer_index,
                                                measurement_4_buffer_index};
bool is_buffer_n_empty[sensor_count] = {is_buffer_1_empty,
                                        is_buffer_2_empty,
                                        is_buffer_3_empty,
                                        is_buffer_4_empty};





// communication buffers
const byte serial_buffer_length = 32;          // predefined max buffer size
char serial_buffer[serial_buffer_length];     // buffer for received messages
boolean serial_new_data = false;           // bool to check if message is complete

//String serial_received_request[4];         // max amount of requests


// global run variables
bool send_live_data = false;


// mqtt broker class
class myMQTTBroker: public uMQTTBroker
{
public:
    virtual bool onConnect(IPAddress addr, uint16_t client_count) {
      debugPrintLn(addr.toString() + " connected");
      return true;
    }

    /*
    virtual bool onAuth(String username, String password) {  // not used
      debugPrintLn("Username/Password: " + username + "/" + password);
      return true;
    }
    */

    virtual void onData(String topic, const char *data, uint32_t length) {
      // topic = onderwerp van mqtt bericht
      // data = bericht, ingelezen als een char buffer
      // length = lengte van bericht

      //scanf
      /*
      for (int i=0; i<length; i++) {
        Serial.println((int)data[i]);
      }
      */

      // converteren van bericht
      char data_str[length+1];
      os_memcpy(data_str, data, length);
      data_str[length] = '\0';  // final received message

      debugPrintLn("Received topic '" + topic + "' with data '" + (String)data_str + "'");

      if (send_live_data == true) {
        Serial.println((String)data_str);
      }

      if (topic == "test") {
        Serial.println(data_str);
      }

      int n_index;
      // 1
      if (topic == "ECG") {
        debugPrintLn("ECG received");
        n_index = 0;
      }
      // 2
      if (topic == "PPG") {
        debugPrintLn("ECG received");
        n_index = 1;
      }

      // setting variables according to topic
      uint16_t *measurement_buffer = measurement_n_buffer[n_index];
      int measurement_buffer_length = measurement_n_buffer_length[n_index];
      int measurement_buffer_index = measurement_n_buffer_index[n_index];
      bool is_buffer_empty = is_buffer_n_empty[n_index];

      // convert Unicode string to 16x8bit integers
      char *data_to_deUnicode = data_str;
      uint16_t deUnicoded_data[bit_groups*2];
      deUnicodeData(data_to_deUnicode, deUnicoded_data);

      // convert 16x8bit integers to 8x16bit integers
      uint16_t *data_to_unsplit = deUnicoded_data;
      uint16_t desplitted_data[bit_groups];
      unSplitBuffer(data_to_unsplit, desplitted_data, bit_groups);

      // decrypt 8x16bit integers to 8x16bit integers
      uint16_t *data_to_decrypt = desplitted_data;  // copy of received data
      uint16_t decrypted_data[bit_groups];  // empty array for decrypted message
      decryptData(data_to_decrypt, decrypted_data);  // decrypting message

      // deshift 8x16bit integers to 12x16(10)bit integers
      uint16_t *data_to_deshift = decrypted_data;
      uint16_t deshifted_data[measurement_groups]; // empty array for deshifted message
      deshiftData(data_to_deshift, bit_groups, deshifted_data, measurement_groups);

      // put data in its buffer for later use
      addDataToBuffer(deshifted_data, measurement_buffer, measurement_1_buffer_length, measurement_1_buffer_index, is_buffer_1_empty);
      debugPrintLn("");
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

  // Start WiFi AP
  debugPrintLn("Starting network connection");
  if (WiFiAP) {
    startWiFiAP();
  } else {
    startWiFiClient();
  }

  // filling buffers with zeros
  for (int i=0; i<sensor_count; i++) {
    resetMeasurementBuffer(measurement_n_buffer[i],
                           measurement_n_buffer_length[i],
                           measurement_n_buffer_index[i],
                           is_buffer_n_empty[i]);
  }

  // Start the broker
  debugPrintLn("Starting MQTT broker");
  myBroker.init();

  // subscribe to all topics
  myBroker.subscribe("#");
}



void loop()
{

  //myBroker.publish("broker/counter", (String)counter++);
  receiveSerial(); // schijf binnekomend bericht naar buffer
  actOnNewSerialData(); // interpreteer en reageer

  if (send_live_data && is_buffer_1_empty == false) {
    sendBuffer("ECG", measurement_1_buffer, measurement_1_buffer_length, measurement_1_buffer_index, is_buffer_1_empty);
  }


  delay(1000);
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
  debugPrintLn("ssid: " + (String)ssid);
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
      //serial_received_request[current_index] = part_of_message;
      debugPrintLn(part_of_message);

      if (String(part_of_message) == "test") {
        digitalWrite(LED_BUILTIN_PIN, LOW);  //temp test
      }

      if (String(part_of_message) == "REQUESTLIVEDATA") {
        send_live_data = true;
      }

      if (String(part_of_message) == "REQUESTBULKDATA") {

      }



      current_index ++;
    }
  serial_new_data = false;
  }
}


void deUnicodeData(char *data_to_deUnicode, uint16_t *deUnicoded_data) {
  debugPrint("DeUnicoding data : [");
  debugPrint(data_to_deUnicode);
  debugPrintLn("]");

  for (int i=0; i<bit_groups; i++) {
    uint16_t element = uint16_t(data_to_deUnicode[i]);
    deUnicoded_data[i] = element;
  }

  debugPrint("DeUnicoded data : [");
  printIntArray(deUnicoded_data, bit_groups*2);
  debugPrintLn("]");
}


void unSplitBuffer(uint16_t *data_to_desplit, uint16_t *desplitted_data, const int desplitted_data_length) {
  debugPrint("Desplitting data : [");
  printIntArray(data_to_desplit, bit_groups*2);
  debugPrintLn("]");

  for(int i=0; i<desplitted_data_length; i++) {
    uint8_t part_1 = data_to_desplit[2*i];
    uint8_t part_2 = data_to_desplit[2*i+1];
    uint16_t element = (part_1 << (desplitted_data_length)) + part_2;
    desplitted_data[i] = element;
  }

  debugPrint("Desplitted data : [");
  printIntArray(desplitted_data, bit_groups);
  debugPrintLn("]");
}


void decryptData(uint16_t *data_to_decrypt, uint16_t *decrypted_data) {
  debugPrint("Decrypting data : [");
  printIntArray(data_to_decrypt, bit_groups);
  debugPrintLn("]");

  for (int i=0; i<8; i++) {
    decrypted_data[i] = i;
  }

  debugPrint("Decrypted data : [");
  printIntArray(decrypted_data, bit_groups);
  debugPrintLn("]");
}


void deshiftData(uint16_t *shifted_data, const int shifted_data_length, uint16_t *deshifted_data, const int deshifted_data_length) {
  debugPrint("Deshifting data : [");
  printIntArray(shifted_data, bit_groups);
  debugPrintLn("]");

  int current_buffer_index = 12 - 1;
  int current_shifted_buffer_index = 8 - 1;
  int current_bits_to_shift = bits_per_int - bits_per_measurement;

  while (true) {
    if (current_buffer_index < 0) {
        break;
    }

    int new_element_to_shift = shifted_data[current_shifted_buffer_index];

    if (current_bits_to_shift >= bits_per_int) {
        current_bits_to_shift = bits_per_int - bits_per_measurement;
    } else {
        int current_buffer_element = deshifted_data[current_buffer_index];
        int bits_to_stay = uint16_t(new_element_to_shift << (current_bits_to_shift)) >> (bits_per_int - bits_per_measurement);
        current_buffer_element = current_buffer_element + bits_to_stay;
        deshifted_data[current_buffer_index] = current_buffer_element;


        int bits_to_shift = uint16_t(new_element_to_shift >> (bits_per_int - current_bits_to_shift));
        deshifted_data[current_buffer_index-1] = bits_to_shift;


        current_bits_to_shift = current_bits_to_shift + (bits_per_int - bits_per_measurement);
        current_shifted_buffer_index --;
    }
    current_buffer_index --;
  }

  debugPrint("Deshifted data : [");
  printIntArray(deshifted_data, measurement_groups);
  debugPrintLn("]");
}


void addDataToBuffer(uint16_t *deshifted_data, uint16_t *measurement_buffer, const int buffer_length, int &buffer_index, bool &is_buffer_empty) {
  debugPrintLn("Adding data to buffer");

  // als er niet voldoende plaats over is in buffer
  if ((buffer_length - buffer_index) < measurement_groups) {
    debugPrintLn("Buffer out of space!");
    return;
  }

  for (int i=0; i<measurement_groups; i++) {
    measurement_buffer[buffer_index] = deshifted_data[i];
    buffer_index++;
  }

  debugPrintLn("Buffer filled with data");
  is_buffer_empty = false;
}


void sendBuffer(String topic, uint16_t *buffer, const int buffer_length, int &buffer_index, bool &is_buffer_empty) {

  if (is_buffer_empty) {
    return;
  }

  if (topic == "ECG") {
    Serial.print("E");
  }
  if (topic == "PPG") {
    Serial.print("P");
  }
  if (topic == "ZWEET") {
    Serial.print("Z");
  }
  if (topic == "VOETDRUK") {
    Serial.print("V");
  }


  for(int i = 0; i < buffer_length; i++) {
    if (buffer[i] != 0) {  // uitgaande dat 0 nooit voorkomt
      Serial.print(buffer[i]); // Serial.write
      if (i != buffer_length-1) {
        Serial.print(" "); // delimiter
      }

    }
  }
  Serial.print("\n");
  debugPrintLn("");

  resetMeasurementBuffer(buffer, buffer_length, buffer_index, is_buffer_empty);
  is_buffer_empty = true;
}

void resetMeasurementBuffer(uint16_t *buffer, const int buffer_length, int &buffer_index, bool &is_buffer_empty) {
  for(int i = 0; i < buffer_length; i++) {
    buffer[i] = 0;
  }
  buffer_index = 0;
  is_buffer_empty = true;
}


void printIntArray(uint16_t *array, const int size_of_array) {
  for(int i = 0; i < size_of_array; i++) {
    debugPrint((String)array[i]);
    debugPrint(",");
  }
}


void printArray(char *array) {
  int size_of_array = strlen(array);
  for(int i = 0; i < size_of_array; i++) {
    debugPrint((String)array[i]);
  }
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
