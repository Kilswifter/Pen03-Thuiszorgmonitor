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

// declarations
#define MAX_CLIENTS 10
#define MAX_LINE_LENGTH 16

// changeable parameters
bool DEBUG = false;
const int bits_per_int = 16;
const int bits_per_measurement = 10;
const int bits_per_encryption = 128;
const int bit_groups = bits_per_encryption / bits_per_int; // 8
const int measurement_groups = 12;

// configure WiFi server
WiFiServer server(1883);
WiFiClient *clients[MAX_CLIENTS] = { NULL };
uint8_t client_ids[MAX_CLIENTS] = { 0 };
uint8_t inputs[MAX_CLIENTS][MAX_LINE_LENGTH] = { 0 };
int topics[MAX_CLIENTS] = { -1 };
uint16_t buffer_index[MAX_CLIENTS] = { 0 } ;

// wifi AP config
char ssid[] = "ESP_reciever_1";
char pass[] = "123456789";
bool WiFiAP = true;

// functions pre-declarations
void startWiFiClient();
void startWiFiAP();

void receiveSerial();
void actOnNewSerialData();

void checkForNewConnections();
uint8_t receiveClientId(WiFiClient* client);
void checkForNewMessages();
void actOnMessage(uint16_t identifier, uint8_t *message_buffer);

void deUnicodeData(char *data_to_deUnicode, uint16_t *deUnicoded_data);
void deSplitData(uint8_t *data_to_desplit, uint16_t *desplitted_data, const int desplitted_data_length);
void decryptData(uint8_t *data_to_decrypt, uint8_t *decrypted_data);
void deshiftData(uint16_t *shifted_buffer, const int shifted_buffer_length, uint16_t *deshifted_buffer, int deshifted_buffer_length);

void addDataToBuffer(uint16_t *deshifted_data, uint16_t *measurement_buffer, const int buffer_length, int &buffer_index, bool &is_buffer_empty);
void sendBuffer(String topic, uint16_t *buffer, const int buffer_length, int &buffer_index, bool &is_buffer_empty);
void resetMeasurementBuffer(uint16_t *buffer, const int buffer_length, int &buffer_index, bool &is_buffer_empty);

void printIntArray(uint16_t *array, const int size_of_array);
void printInt8Array(uint8_t *array, const int size_of_array);
void printArray(char *array);
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
// 4 = voetdruk 1
const int measurement_4_buffer_length = 128;
int measurement_4_buffer_index = 0;
uint16_t measurement_4_buffer[measurement_4_buffer_length];
bool is_buffer_4_empty = true;
// 4 = voetdruk 2
const int measurement_5_buffer_length = 128;
int measurement_5_buffer_index = 0;
uint16_t measurement_5_buffer[measurement_5_buffer_length];
bool is_buffer_5_empty = true;
// 4 = voetdruk 3
const int measurement_6_buffer_length = 128;
int measurement_6_buffer_index = 0;
uint16_t measurement_6_buffer[measurement_6_buffer_length];
bool is_buffer_6_empty = true;
// 4 = voetdruk 4
const int measurement_7_buffer_length = 128;
int measurement_7_buffer_index = 0;
uint16_t measurement_7_buffer[measurement_7_buffer_length];
bool is_buffer_7_empty = true;

const int sensor_count = 7;

int measurement_n_buffer_length[sensor_count] = {measurement_1_buffer_length,
                                                 measurement_2_buffer_length,
                                                 measurement_3_buffer_length,
                                                 measurement_4_buffer_length,
                                                 measurement_5_buffer_length,
                                                 measurement_6_buffer_length,
                                                 measurement_7_buffer_length};
uint16_t* measurement_n_buffer[sensor_count] = {measurement_1_buffer,
                                                measurement_2_buffer,
                                                measurement_3_buffer,
                                                measurement_4_buffer,
                                                measurement_5_buffer,
                                                measurement_6_buffer,
                                                measurement_7_buffer};
int measurement_n_buffer_index[sensor_count] = {measurement_1_buffer_index,
                                                measurement_2_buffer_index,
                                                measurement_3_buffer_index,
                                                measurement_4_buffer_index,
                                                measurement_5_buffer_index,
                                                measurement_6_buffer_index,
                                                measurement_7_buffer_index};
bool is_buffer_n_empty[sensor_count] = {is_buffer_1_empty,
                                        is_buffer_2_empty,
                                        is_buffer_3_empty,
                                        is_buffer_4_empty,
                                        is_buffer_5_empty,
                                        is_buffer_6_empty,
                                        is_buffer_7_empty};





// communication buffers
const byte serial_buffer_length = 32;          // predefined max buffer size
char serial_buffer[serial_buffer_length];     // buffer for received messages
boolean serial_new_data = false;           // bool to check if message is complete

//String serial_received_request[4];         // max amount of requests


// global run variables
bool send_live_data = false;












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
    resetMeasurementBuffer(measurement_n_buffer[i],
                           measurement_n_buffer_length[i],
                           measurement_n_buffer_index[i],
                           is_buffer_n_empty[i]);
  }
}



void loop()
{
  checkForNewConnections();
  checkForNewMessages();
  //myBroker.publish("broker/counter", (String)counter++);
  receiveSerial(); // schijf binnekomend bericht naar buffer
  actOnNewSerialData(); // interpreteer en reageer

  if (send_live_data && is_buffer_1_empty == false) {
    sendBuffer("ECG", measurement_1_buffer, measurement_1_buffer_length, measurement_1_buffer_index, is_buffer_1_empty);
  }


  //delay(1);
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

void checkForNewConnections() {
  // Check if a new client has connected
  WiFiClient newClient = server.available();
  if (newClient) {
    debugPrintLn("new client");
    // resetting buffers
    /*
    for(int client=0; client<MAX_CLIENTS; client++) {
      for(int r=0; r<MAX_LINE_LENGTH; r++) {
        inputs[client][r] = 0;
      }
      topics[client] = -1;
    }
    */


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
        if (buffer_index[l] < MAX_LINE_LENGTH) {
          inputs[l][buffer_index[l]] = newBit;
          buffer_index[l] = buffer_index[l] + 1;
        }
        // when buffer is full
        else{
          buffer_index[l] = 0;
          actOnMessage(topics[l], inputs[l]);
          topics[l] = -1;
        }
      }
    }
  }
}

void actOnMessage(uint16_t identifier, uint8_t *message_buffer) {

  String topic = "";
  if (identifier == 0) {
    topic = "ECG";
  }
  if (identifier == 1) {
    topic = "PPG";
  }
  if (identifier == 2) {
    topic = "ZWEET";
  }
  if (identifier == 3) {
    topic = "VOET1";
  }
  if (identifier == 4) {
    topic = "VOET2";
  }
  if (identifier == 5) {
    topic = "VOET3";
  }
  if (identifier == 6) {
    topic = "VOET4";
  }

  int n_index = identifier;


  debugPrint("Buffer from signal " + topic + " - ");
  printInt8Array(message_buffer, MAX_LINE_LENGTH);
  debugPrintLn("");

  // setting variables according to topic
  uint16_t *measurement_buffer = measurement_n_buffer[n_index];
  int measurement_buffer_length = measurement_n_buffer_length[n_index];
  int measurement_buffer_index = measurement_n_buffer_index[n_index];
  bool is_buffer_empty = is_buffer_n_empty[n_index];

  // decrypt 16x8bit integers to 16x8bit integers
  //uint8_t *data_to_decrypt = message_buffer;  // copy of received data
  //uint8_t decrypted_data[bit_groups*2];  // empty array for decrypted message
  //decryptData(data_to_decrypt, decrypted_data);  // decrypting message

  // convert 16x8bit integers to 8x16bit integers
  uint8_t *data_to_desplit = message_buffer; // decrypted_data;
  uint16_t desplitted_data[bit_groups];
  deSplitData(data_to_desplit, desplitted_data, bit_groups);

  // deshift 8x16bit integers to 12x16(10)bit integers
  uint16_t *data_to_deshift = desplitted_data;
  uint16_t deshifted_data[measurement_groups]; // empty array for deshifted message
  for (int t=0; t<12; t++) {
    deshifted_data[t] = 0;
  }
  deshiftData(data_to_deshift, bit_groups, deshifted_data, measurement_groups);

  // put data in its buffer for later use
  addDataToBuffer(deshifted_data, measurement_buffer, measurement_1_buffer_length, measurement_1_buffer_index, is_buffer_1_empty);
  debugPrintLn("");
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
    Serial.print("E ");
  }
  if (topic == "PPG") {
    Serial.print("P ");
  }
  if (topic == "ZWEET") {
    Serial.print("Z ");
  }
  if (topic == "VOETDRUK") {
    Serial.print("V ");
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
