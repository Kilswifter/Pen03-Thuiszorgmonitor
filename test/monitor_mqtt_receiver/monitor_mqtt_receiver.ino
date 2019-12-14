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
#include <map>
#include "AEGIS.h"
#include "AEGIS.hpp"
#include "AESround.h"
#include "tag.h"
#include "decryption.h"
#include "encryption.h"
#include "mixColumnsAes.h"
#include "shiftRowsAes.h"
#include "preparing.h"
#include "stateUpdate.h"
#include "subBytesAes.h"

// declarations
#define MAX_CLIENTS 10          // max aantal clients die zonder reboot na elkaar kunnen verbinden (niet persé tegelijk)
#define MAX_LINE_LENGTH 16        // max berichtlengte

// changeable parameters
const int bits_per_int = 16;
const int bits_per_measurement = 10;
const int bits_per_encryption = 128;
const int bit_groups = bits_per_encryption / bits_per_int; // 8
const int measurement_groups = int(bits_per_encryption / bits_per_measurement); //12;
float loop_frequency = 1000;
int loop_interval = round(1e6/loop_frequency);
unsigned long last_micros;

// configure WiFi server
WiFiServer server(1883);                              // TCP/IP server op poort 1883
WiFiClient *clients[MAX_CLIENTS] = { NULL };          // buffer voor client verbindingen
uint8_t client_ids[MAX_CLIENTS] = { 0 };              // buffer voor client ID's
uint8_t inputs[MAX_CLIENTS][MAX_LINE_LENGTH] = { 0 }; // buffers voor binnenkomende data
uint8_t tags[MAX_CLIENTS][MAX_LINE_LENGTH] = { 0 };   // buffers voor binnenkomende data
int topics[MAX_CLIENTS] = { -1 };                     // buffer voor onderwerp van data
uint16_t buffer_index[MAX_CLIENTS] = { 0 } ;          // bufferposities

// wifi AP config
char ssid[] = "ESP_reciever_1";
char pass[] = "123456789";

// functions pre-declarations
void startWiFiClient();
void startWiFiAP();

void receiveSerial();
void actOnNewSerialData();

void checkForNewConnections();
uint8_t receiveClientId(WiFiClient* client);
void checkForNewMessages();
void actOnMessage(uint16_t identifier, uint8_t *message_buffer, uint8_t *tag_buffer);

void deUnicodeData(char *data_to_deUnicode, uint16_t *deUnicoded_data);
void deSplitData(uint8_t *data_to_desplit, uint16_t *desplitted_data, const int desplitted_data_length);
void decryptData(uint8_t *data_to_decrypt, uint8_t *decrypted_data, uint8_t *tag_buffer);
void deshiftData(uint16_t *shifted_buffer, const int shifted_buffer_length, uint16_t *deshifted_buffer, int deshifted_buffer_length);

void addDataToBuffer(uint8_t buffer_id, uint16_t *deshifted_data);
void sendBuffer(uint8_t buffer_id);
void sendBufferVisual();
void resetMeasurementBuffer(int buffer_id);

void printIntArray(uint16_t *array, const int size_of_array);
void printInt8Array(uint8_t *array, const int size_of_array);
void printArray(char *array);
void debugPrintLn(String str);
void debugPrint(String str);


// measurement buffers
//  0 = EXTRA
const int measurement_0_buffer_length = 128;
int measurement_0_buffer_index = 0;
uint16_t measurement_0_buffer[measurement_0_buffer_length];
bool is_buffer_0_empty = true;
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
// 4 = voetdruk 1
const int measurement_4_buffer_length = 128;
int measurement_4_buffer_index = 0;
uint16_t measurement_4_buffer[measurement_4_buffer_length];
bool is_buffer_4_empty = true;
// 5 = voetdruk 2
const int measurement_5_buffer_length = 128;
int measurement_5_buffer_index = 0;
uint16_t measurement_5_buffer[measurement_5_buffer_length];
bool is_buffer_5_empty = true;
// 6 = voetdruk 3
const int measurement_6_buffer_length = 128;
int measurement_6_buffer_index = 0;
uint16_t measurement_6_buffer[measurement_6_buffer_length];
bool is_buffer_6_empty = true;
// 7 = voetdruk 4
const int measurement_7_buffer_length = 128;
int measurement_7_buffer_index = 0;
uint16_t measurement_7_buffer[measurement_7_buffer_length];
bool is_buffer_7_empty = true;

const int sensor_count = 8;

int measurement_n_buffer_length[sensor_count] = {measurement_0_buffer_length,
                                                 measurement_1_buffer_length,
                                                 measurement_2_buffer_length,
                                                 measurement_3_buffer_length,
                                                 measurement_4_buffer_length,
                                                 measurement_5_buffer_length,
                                                 measurement_6_buffer_length,
                                                 measurement_7_buffer_length};
uint16_t* measurement_n_buffer[sensor_count] = {measurement_0_buffer,
                                                measurement_1_buffer,
                                                measurement_2_buffer,
                                                measurement_3_buffer,
                                                measurement_4_buffer,
                                                measurement_5_buffer,
                                                measurement_6_buffer,
                                                measurement_7_buffer};
int measurement_n_buffer_index[sensor_count] = {measurement_0_buffer_index,
                                                measurement_1_buffer_index,
                                                measurement_2_buffer_index,
                                                measurement_3_buffer_index,
                                                measurement_4_buffer_index,
                                                measurement_5_buffer_index,
                                                measurement_6_buffer_index,
                                                measurement_7_buffer_index};
bool is_buffer_n_empty[sensor_count] = {is_buffer_0_empty,
                                        is_buffer_1_empty,
                                        is_buffer_2_empty,
                                        is_buffer_3_empty,
                                        is_buffer_4_empty,
                                        is_buffer_5_empty,
                                        is_buffer_6_empty,
                                        is_buffer_7_empty};

String topic_list[8] = {"EXTRA", "ECG", "PPG", "ZWEET", "VOET1", "VOET2", "VOET3", "VOET4"};

// communication buffers
const byte serial_buffer_length = 32;          // max aantal characters voor binnenkomend bericht
char serial_buffer[serial_buffer_length];      // buffer voor seriële berichten
boolean serial_new_data = false;               // bool to check if message is complete

// global run variables
bool tag_match = false;
bool DEBUG = false;
bool FIXEDSENSOR = false;
int FIXEDSENSORID = 1;
bool SINGLEVALUE = false;
bool SENDLIVEDATA = true;
bool SENDVISUAL = false;
bool DECRYPT = true;

std::map<String, bool*> bool_map = {
    { "DEBUG", &DEBUG },
    { "FIXEDSENSOR", &FIXEDSENSOR },
    { "SINGLEVALUE", &SINGLEVALUE },
    { "SENDLIVEDATA", &SENDLIVEDATA },
    { "SENDVISUAL", &SENDVISUAL },
    { "DECRYPT", &DECRYPT },
};


/**
################################################################################

                                  Program

################################################################################
**/


void setup()
{
  Serial.begin(115200);
  delay(10);
  debugPrintLn("\n\n***************************************************\n");
  debugPrintLn("                ESP started running\n");
  debugPrintLn("***************************************************\n");

  // Start WiFi AP
  debugPrintLn("Starting network connection");
  startWiFiAP();

  // Start Wifi Server
  server.begin();
  Serial.println("Server started");

  // filling buffers with zeros
  for (int i=0; i<sensor_count; i++) {
    resetMeasurementBuffer(i);
  }

  preparing(Key, IV, const0, const1);

  unsigned long last_micros = micros();
}


void loop()
{


  checkForNewConnections();   // controleer op nieuwe sensorclients
  checkForNewMessages();      // controleer op binnenkomende berichten & reageer

  receiveSerial();            // schijf binnekomend seriële bericht naar buffer
  actOnNewSerialData();       // interpreteer en reageer

  unsigned long diff_micros = micros() - last_micros;
  if (diff_micros >= loop_interval) {
    if (diff_micros > 4*loop_interval) {
      last_micros = micros();
    } else { last_micros += loop_interval; }

    // Send data buffers to serial port for Matlab
    if (SENDLIVEDATA == true) {
      for (uint8_t sensor_buffer_index=0; sensor_buffer_index<sensor_count; sensor_buffer_index++) {
        uint8_t q = 0;
        if (FIXEDSENSOR == true) { q = (uint8_t)FIXEDSENSORID; } else { q = sensor_buffer_index; }
        if (is_buffer_n_empty[q] == false) {
          sendBuffer(q);
          is_buffer_n_empty[q] = true;
        }
      }
    }
    if (SENDVISUAL == true) {
      if (is_buffer_n_empty[1] == false) {
        sendBufferVisual();
      }
    }
  }
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
            String command = (String)part_of_message;
      debugPrintLn("Message : " + command);

      if (command == "FIXEDSENSORID") {
        FIXEDSENSORID = ((String)strtok_r(p, "/", &p)).toInt();
      }
      if (command == "LOOPINTERVAL") {
        loop_interval = ((String)strtok_r(p, "/", &p)).toInt();
      }
      if (command == "LOOPFREQ") {
        loop_interval = round(1e6/(float)((String)strtok_r(p, "/", &p)).toInt());
      }

      if (bool_map.count(command) > 0) {
        *bool_map[command] = !*bool_map[command];
      }

      current_index ++;
    }
  serial_new_data = false;
  }
}


void checkForNewConnections() {
  // Check if a new client has connected
  WiFiClient newClient = server.available();
  if (newClient) {
    debugPrintLn("new client");
    WiFiClient* cl = new WiFiClient(newClient);
    delay(500);

    uint8_t client_id = receiveClientId(cl);
    debugPrintLn("Client id :" + (String)client_id);

    // Find the first unused space
    for (int i=0 ; i<MAX_CLIENTS ; ++i) {

      if (client_id == client_ids[i]) {
        debugPrintLn("That's no new client!");
        // reset its buffer
        for(int r=0; r<MAX_LINE_LENGTH; r++) {
          inputs[i][r] = 0;
        }
        topics[i] = -1;

        clients[i] = cl;
        break;
      }

      if (NULL == clients[i]) {
          clients[i] = cl;//new WiFiClient(newClient);
          client_ids[i] = client_id;
          topics[i] = -1;
          break;
      }
    }
  }
}


uint8_t receiveClientId(WiFiClient* client) {
  uint8_t client_id = client->read();
  return client_id;
}


void checkForNewMessages() {
  // Check whether each client has some data
  for (int l=0 ; l<MAX_CLIENTS ; l++) {
    // If the client is in use, and has some data...
    if (NULL != clients[l] && clients[l]->available() ) {

      uint8_t newBit = clients[l]->read();

      if (topics[l] == -1) {
        debugPrintLn("Topic identifier received : " + (String)newBit);
        topics[l] = newBit;
      }

      else {
        // add to buffer if it still has space
        if (buffer_index[l] < MAX_LINE_LENGTH*2) {
          if (buffer_index[l] < MAX_LINE_LENGTH) {
            inputs[l][buffer_index[l]] = newBit;
            buffer_index[l] = buffer_index[l] + 1;
          } else {
            tags[l][buffer_index[l]-MAX_LINE_LENGTH] = newBit;
            buffer_index[l] = buffer_index[l] + 1;
          }
        }
        // when buffer is full
        else{
          buffer_index[l] = 0;
          actOnMessage(topics[l], inputs[l], tags[l]);
          topics[l] = -1;
        }
      }
    }
  }
}


void actOnMessage(uint16_t identifier, uint8_t *message_buffer, uint8_t *tag_buffer) {

  String topic = topic_list[identifier];
  int n_index = identifier;

  debugPrint("Buffer from signal " + topic + " - ");
  printInt8Array(message_buffer, MAX_LINE_LENGTH);
  debugPrint(" - ");
  printInt8Array(tag_buffer, MAX_LINE_LENGTH);
  debugPrintLn("");

  // setting variables according to topic
  uint16_t *measurement_buffer = measurement_n_buffer[n_index];
  int measurement_buffer_length = measurement_n_buffer_length[n_index];
  int measurement_buffer_index = measurement_n_buffer_index[n_index];
  bool is_buffer_empty = is_buffer_n_empty[n_index];

  // decrypt 16x8bit integers to 16x8bit integers
  uint8_t *data_to_decrypt = message_buffer;  // copy of received data
  uint8_t decrypted_data[bit_groups*2];  // empty array for decrypted message
  decryptData(data_to_decrypt, decrypted_data, tag_buffer);  // decrypting message

  if (tag_match == true) {
    // convert 16x8bit integers to 8x16bit integers
    uint8_t *data_to_desplit = message_buffer; // decrypted_data;
    uint16_t desplitted_data[bit_groups];
    deSplitData(data_to_desplit, desplitted_data, bit_groups);

    // deshift 8x16bit integers to 12x16(10)bit integers
    uint16_t *data_to_deshift = desplitted_data;
    uint16_t deshifted_data[measurement_groups]; // empty array for deshifted message
    for (int t=0; t<12; t++) { deshifted_data[t] = 0; }
    deshiftData(data_to_deshift, bit_groups, deshifted_data, measurement_groups);

    // put data in its buffer for later use
    uint8_t buffer_id = n_index;
    addDataToBuffer(buffer_id, deshifted_data);
    debugPrintLn("");

    tag_match = false;
  }
}


void deSplitData(uint8_t *data_to_desplit, uint16_t *desplitted_data, const int desplitted_data_length) {
  debugPrint("Desplitting data : [");
  printInt8Array(data_to_desplit, bit_groups*2);
  debugPrintLn("]");

  for(int i=0; i<desplitted_data_length; i++) {
    uint8_t part_1 = data_to_desplit[2*i];
    uint8_t part_2 = data_to_desplit[2*i+1];
    uint16_t element = (part_1 << (8)) + part_2;
    desplitted_data[i] = element;
  }

  debugPrint("Desplitted data : [");
  printIntArray(desplitted_data, bit_groups);
  debugPrintLn("]");
}


void decryptData(uint8_t *data_to_decrypt, uint8_t *decrypted_data) {
  debugPrint("Decrypting data : [");
  printInt8Array(data_to_decrypt, bit_groups*2);
  debugPrintLn("]");

  decrypted_data = data_to_decrypt;

  debugPrint("Decrypted data : [");
  printInt8Array(decrypted_data, bit_groups*2);
  debugPrintLn("]");
}

void decryptData(uint8_t *data_to_decrypt, uint8_t *decrypted_data, uint8_t *decryption_tag) {
  debugPrint("Decrypting data : [");
  printInt8Array(data_to_decrypt, bit_groups*2);
  debugPrintLn("]");

  //preparing(Key, IV, const0, const1);
  createTag(S0, S1, S2, S3, S4, msglen, adlen);
  if (checkTag(decryption_tag, tagsend) == true) {
    debugPrintLn("Tags match!");
    decryption(data_to_decrypt, S0, S1, S2, S3, S4);
    decrypted_data = resultSend;
    //encryption_tag = tagsend;

    tag_match = true;

    debugPrint("Decrypted data : [");
    printInt8Array(decrypted_data, bit_groups*2);
    debugPrintLn("]");

    debugPrint("Decryption tag : [");
    printInt8Array(decrypted_data, bit_groups*2);
    debugPrintLn("]");
  } else {
    debugPrintLn("Decryption failed!");
  }
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

    uint16_t new_element_to_shift = shifted_data[current_shifted_buffer_index];

    if (current_bits_to_shift >= bits_per_int) {
        current_bits_to_shift = bits_per_int - bits_per_measurement;
    } else {
        uint16_t current_buffer_element = deshifted_data[current_buffer_index];
        uint16_t bits_to_stay = uint16_t(new_element_to_shift << (current_bits_to_shift)) >> (bits_per_int - bits_per_measurement);
        current_buffer_element = current_buffer_element + bits_to_stay;
        deshifted_data[current_buffer_index] = current_buffer_element;


        uint16_t bits_to_shift = uint16_t(new_element_to_shift >> (bits_per_int - current_bits_to_shift));
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


void addDataToBuffer(uint8_t buffer_id, uint16_t *deshifted_data) {
  debugPrintLn("Adding data to buffer");
  uint16_t *measurement_buffer = measurement_n_buffer[buffer_id];
  const int buffer_length = measurement_n_buffer_length[buffer_id];
  int &buffer_index = measurement_n_buffer_index[buffer_id];
  bool &is_buffer_empty = is_buffer_n_empty[buffer_id];

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


void sendBuffer(uint8_t buffer_id) {
  uint16_t *measurement_buffer = measurement_n_buffer[buffer_id];
  int measurement_buffer_length = measurement_n_buffer_length[buffer_id];
  int measurement_buffer_index = measurement_n_buffer_index[buffer_id];
  bool is_buffer_empty = is_buffer_n_empty[buffer_id];

  if (is_buffer_empty) {
    return;
  }

  Serial.print(topic_list[buffer_id]);
  Serial.print(" ");

  int max_index = measurement_buffer_length;
  if (SINGLEVALUE) { max_index = 1; }

  for(int i = 0; i < max_index; i++) {
    if (measurement_buffer[i] != 0) {
      Serial.print(measurement_buffer[i]); // Serial.write
      if (i != measurement_buffer_length-1) {
        Serial.print(" "); // delimiter
      }
    }
  }
  Serial.print("\n");
  debugPrintLn("");

  resetMeasurementBuffer(buffer_id);
}


void sendBufferVisual() {
  for (uint8_t element_index=0; element_index<12; element_index++) {
    for (uint8_t sensor_buffer_index=1; sensor_buffer_index<sensor_count; sensor_buffer_index++) {
      Serial.print(measurement_n_buffer[sensor_buffer_index][element_index] + " ");
    }
    Serial.println("");
  }

  for (uint8_t sensor_buffer_index=0; sensor_buffer_index<sensor_count; sensor_buffer_index++) {
    resetMeasurementBuffer(sensor_buffer_index);
  }
}

void resetMeasurementBuffer(int buffer_id) {
  uint16_t *measurement_buffer = measurement_n_buffer[buffer_id];
  for(int i = 0; i < measurement_n_buffer_length[buffer_id]; i++) {
    measurement_buffer[i] = 0;
  }
  measurement_n_buffer[buffer_id] = measurement_buffer;
  measurement_n_buffer_index[buffer_id] = 0;
  is_buffer_n_empty[buffer_id] = true;
}


void printIntArray(uint16_t *array, const int size_of_array) {
  for(int i = 0; i < size_of_array; i++) {
    debugPrint((String)array[i]);
    debugPrint(",");
  }
}


void printInt8Array(uint8_t *array, const int size_of_array) {
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
