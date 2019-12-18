#include <ESP8266WiFi.h>
#include <map>
#include "AEGIS.h";
#include "AESround.h";
#include "decryption.h";
#include "encryption.h";
#include "mixColumnsAes.h";
#include "preparing.h";
#include "shiftRowsAes.h";
#include "stateUpdate.h";
#include "subBytesAes.h";
#include "tag.h";
#include "AEGIS.hpp";

/*
Shifting data : [0,1,2,3,4,5,6,7,8,9,10,11,]
Shifted data : [0,1026,48,4101,96,7176,144,10251,]
Splitting data : [0,1026,48,4101,96,7176,144,10251,]
Splitted data : [0,0,4,2,0,48,16,5,0,96,28,8,0,144,40,11,]
Encrypted data : [219,224,165,242,160,176,116,58,66,47,194,245,208,206,156,91,]
encryption tag : [144,50,181,148,141,121,222,115,142,210,47,0,178,161,207,171,]
 */

int client_id = 1;

// WIFI //
const char* ssid = "ESP_reciever_1";
const char* password = "123456789";
const char* IP = "192.168.4.1";
uint16_t PORT = 1883;

WiFiClient client;

// INPUT/OUTPUT //
bool multiplexer_combinations[4][2] = {{0,0},{0,1},{1,0},{1,1}}; 
const uint8_t multiplexer_pins[2] = {4,5};
const uint8_t input_pin = A0;
int led_pin = 12;
int IR_pin = 13;
int interval_nb = 1;

// SAMPLE //
float ecg_samplefreq  = 250;
float ppg_samplefreq  = 50;
float zweet_samplefreq = 50;
int ecg_interval  = round(1e6/ecg_samplefreq);
unsigned long last_micros;              
const uint8_t counter_ppg_max = round(ecg_samplefreq/ppg_samplefreq);
uint8_t counter_ppg = counter_ppg_max-1;
const uint8_t counter_zweet_max = round(ecg_samplefreq/zweet_samplefreq);
uint8_t counter_zweet = counter_zweet_max-1;

uint8_t sample_count_ecg = 0;
uint8_t sample_count_ppg = 0;
uint8_t sample_count_zweet = 0;
uint8_t sample_count_serial = 0;
const int sensor_count = 4;
uint8_t sample_count_n[sensor_count] = {sample_count_ecg, 
                                        sample_count_ppg, 
                                        sample_count_zweet, 
                                        sample_count_serial};


// BUFFERS //
const uint8_t buffer_length = 12;
const uint8_t shifted_buffer_length = 8;

uint16_t ecg_samples[buffer_length] = {0};
uint16_t ppg_samples[buffer_length] = {0};
uint16_t zweet_samples[buffer_length] = {0};
uint16_t serial_samples[buffer_length] = {0};
uint16_t *n_samples[sensor_count] = {ecg_samples, 
                                     ppg_samples, 
                                     zweet_samples,
                                     serial_samples};

uint8_t bits_per_encryption = 128;
uint8_t bits_per_measurement = 10;
uint8_t bits_per_int = 16;

// COMMUNICATION //
const byte serial_buffer_length = 32;          // predefined max buffer size
char serial_buffer[serial_buffer_length];     // buffer for received messages
boolean serial_new_data = false;           // bool to check if message is complete

// RUN VARIABLES //
bool send_live_data = true;

bool DEBUG = false;
bool FIXEDSENSOR = false;
int FIXEDSENSORID = 0;
bool NORMALMODE = true;
bool TESTDATA = false;
bool SINGLEVALUE = false;
bool PRINTDATA = true;
bool ENCRYPT = false;

std::map<String, bool*> bool_map = {
    { "DEBUG", &DEBUG },
    { "FIXEDSENSOR", &FIXEDSENSOR },
    { "NORMALMODE", &NORMALMODE },
    { "TESTDATA", &TESTDATA },
    { "SINGLEVALUE", &SINGLEVALUE },
    { "PRINTDATA", &PRINTDATA },
    { "ENCRYPT", &ENCRYPT },
};


void setup_wifi() {
  // We start by connecting to a WiFi network
  //Serial.println();
  //Serial.print("Connecting to ");
  //Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    //Serial.print(".");
  }

  //Serial.println("");
  //Serial.println("WiFi connected");
  //Serial.println("IP address: ");
  //Serial.println(WiFi.localIP());
  //Serial.println("");
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting connection...");
    if (client.connect(IP, PORT)){
      Serial.println("Connected to server!");
      client.write(client_id);
    } else {
      Serial.println("failed, resetting wifi connection");
      setup_wifi();
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void ICACHE_RAM_ATTR onTimerISR(){
    timer1_write(125000000); // 2.50 s
    interval_nb++;
    if (interval_nb == 1){
      digitalWrite(led_pin, LOW); 
      digitalWrite(IR_pin, HIGH); 
      }
    if (interval_nb == 2){
      digitalWrite(led_pin, HIGH); //LOW
      digitalWrite(IR_pin, HIGH); // HIGH
    }
    if (interval_nb == 3){
      digitalWrite(led_pin, HIGH); // HIGH
      digitalWrite(IR_pin, LOW); // LOW
    }
    if (interval_nb == 4){
      digitalWrite(led_pin, HIGH); // HIGH
      digitalWrite(IR_pin, HIGH); // LOW
      interval_nb = 0;
    }
}



void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(1000);

  // WIFI //
  setup_wifi();

  // INPUT/OUTPUT //
  pinMode(input_pin, INPUT);
  pinMode(multiplexer_pins[0], OUTPUT);
  pinMode(multiplexer_pins[1], OUTPUT);
  digitalWrite(multiplexer_pins[0], LOW);
  digitalWrite(multiplexer_pins[1], LOW);
 
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, HIGH);
  pinMode(IR_pin, OUTPUT);
  digitalWrite(IR_pin, LOW);

  timer1_isr_init(); 
  timer1_attachInterrupt(onTimerISR);
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE); // 5MHZ timer gives 5 ticks per us (80MHz timer divided by 16)
  timer1_write(125000000);                             // number of ticks, 1250 ticks / 5 ticks per us = 250 us
  
  unsigned long last_micros = micros(); 
}

void loop() {
  
  // WIFI //
  if (!client.connected()) {
    reconnect();
    unsigned long last_micros = micros();
  }

  if (NORMALMODE == true) {
    // SAMPLE //
    unsigned long diff_micros = micros() - last_micros;
    if (diff_micros >= ecg_interval) {
       
      if (diff_micros > 4*ecg_interval) {
        last_micros = micros();
      } else { last_micros += ecg_interval; }
      
      // ECG sample [0]
      if (!FIXEDSENSOR or FIXEDSENSORID == 0) {
        multiplexerSelect(multiplexer_combinations[0]);
        debugPrint("Sampeling sensor ECG - ");
        n_samples[0][sample_count_n[0]] = analogRead(input_pin);
        if (PRINTDATA){Serial.print((String)n_samples[0][sample_count_n[0]] + " ");}

        sample_count_n[0] = sample_count_n[0] + 1; //12 voor één waarde
        if (SINGLEVALUE == true) { sample_count_n[0] = 12; }
      }
      
      // PPG sample [1]
      counter_ppg++;
      if (!FIXEDSENSOR or FIXEDSENSORID == 1) {
        if (counter_ppg == counter_ppg_max){
          multiplexerSelect(multiplexer_combinations[1]);
          debugPrint("Sampeling sensor PPG - ");
          n_samples[1][sample_count_n[1]] = analogRead(input_pin);
          if (PRINTDATA){Serial.print((String)n_samples[1][sample_count_n[1]] + " ");}
          sample_count_n[1] = sample_count_n[1] + 1;
          if (SINGLEVALUE == true) { sample_count_n[1] = 12; }
          counter_ppg = 0;
        } else { if (PRINTDATA){Serial.print((String)n_samples[1][max(sample_count_n[1]-1, 0)] + " ");}}
      }

      // ZWEET sample [2]
      counter_zweet++;
      if (!FIXEDSENSOR or FIXEDSENSORID == 2) {
        if (counter_zweet == counter_zweet_max){
          multiplexerSelect(multiplexer_combinations[2]);
          debugPrint("Sampeling sensor ZWEET - ");
          n_samples[2][sample_count_n[2]] = analogRead(input_pin);
          if (PRINTDATA){Serial.print((String)n_samples[2][sample_count_n[2]] + " ");}
          sample_count_n[2] = sample_count_n[2] + 1;
          if (SINGLEVALUE == true) { sample_count_n[2] = 12; }
          counter_zweet = 0;
        } else {if (PRINTDATA){Serial.print((String)n_samples[2][max(sample_count_n[2]-1, 0)] + " ");}}
      }
      Serial.println("");
      /*
      if (PRINTDATA == true) {
        Serial.print((String)n_samples[0][0]);
        Serial.print(" ");
        Serial.print((String)n_samples[1][0]);
        Serial.print(" ");
        Serial.println((String)n_samples[2][0]);
      }
      */
      
  
      // check if buffers are full 
      for (int q=0; q<sensor_count; q++){
        int n = 0;
        if (FIXEDSENSOR == true) {
          n = FIXEDSENSORID; 
        } else {
          n = q;
        }
        
        
        if (sample_count_n[n] == buffer_length){
          sample_count_n[n] = 0;
          if (TESTDATA == true) { fillBufferWithTestData(n_samples[n], buffer_length); }
          sendDataBuffer(n, n_samples[n]);
          n_samples[n][0] = n_samples[n][12];
          for (int s=1; s<12; s++) { n_samples[n][s] = 0; }
        }
      }
    } 
  }
  receiveSerial();
  actOnNewSerialData();
}


/**
################################################################################

                                Functions

################################################################################
**/


void receiveSerial() {
  static byte character_index = 0;
  char end_marker_1 = '\n';
  char end_marker_2 = '*';
  char new_character;

  if (Serial.available() > 0) {
    while (Serial.available() > 0 && serial_new_data == false) {
      new_character = Serial.read();
      if (new_character != end_marker_1 && new_character != end_marker_2) {
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


void actOnNewSerialData() {
  if (serial_new_data == true) {
    char *p = serial_buffer;
    char *part_of_message;
    int current_index = 0;
    while ((part_of_message = strtok_r(p, "/", &p)) != NULL) {
      String command = (String)part_of_message;
      debugPrintLn("Message : " + command + " ");
      // set multiplexer fixed to only one sensor
      if (command == "FIXEDSENSORID") {
        FIXEDSENSORID = ((String)strtok_r(p, "/", &p)).toInt();
      }
      if (command == "SAMPLEINTERVAL") {
        ecg_interval = ((String)strtok_r(p, "/", &p)).toInt();
      } 
      if (command == "SAMPLEFREQ") {
        ecg_interval = round(1e6/(float)((String)strtok_r(p, "/", &p)).toInt());
      }
      if (bool_map.count(command) > 0) {
        *bool_map[command] = !*bool_map[command];
      }

      if (NORMALMODE == false) {
        n_samples[sensor_count-1][sample_count_n[sensor_count-1]] = (uint16_t)((String)part_of_message).toInt();  
        sample_count_n[sensor_count-1] = sample_count_n[sensor_count-1] + 1;

        if (sample_count_n[sensor_count-1] == buffer_length) {
          sample_count_n[sensor_count-1] = 0;
          sendDataBuffer(0, n_samples[sensor_count-1]);
        }
      }

      current_index ++;
    }
  serial_new_data = false;
  }
}


void multiplexerSelect(bool *combination){   // pin is a number between 0 and 3
  digitalWrite(multiplexer_pins[0], combination[0]);
  digitalWrite(multiplexer_pins[1], combination[1]);
}


void sendDataBuffer(int topic_id, uint16_t *data_buffer) {
        
  uint16_t *data_to_shift = data_buffer;
  uint16_t shifted_data[shifted_buffer_length];
  bitShift(data_to_shift, 12, shifted_data, 8);
  
  uint16_t *data_to_split = shifted_data;
  uint8_t splitted_data[shifted_buffer_length*2];
  splitBuffer(data_to_split, shifted_buffer_length, splitted_data);
  
  uint8_t *data_to_encrypt = splitted_data;

  if (ENCRYPT == true) {
    uint8_t data_to_send[shifted_buffer_length*2];
    uint8_t encryption_tag[shifted_buffer_length*2];
    debugPrintLn("");
    printInt8Array(encryption_tag, 16);
    debugPrintLn("");

    encryptData(data_to_encrypt, data_to_send, encryption_tag);

    
    debugPrintLn(String(topic_id+1));
    printInt8Array(data_to_send, 16);
    debugPrintLn("");
    printInt8Array(encryption_tag, 16);
    debugPrintLn("");
    
    client.write(topic_id+1);
    client.write(cipherTextBlocksend, 16);
    client.write(tagsend, 17);
        
  } else {
    client.write(topic_id+1);
    client.write(data_to_encrypt, 17);
  }
}



void bitShift(uint16_t *measurement_buffer, uint8_t buffer_length, uint16_t *shifted_buffer, uint8_t shifted_buffer_length) {
  debugPrint("Shifting data : [");
  printInt16Array(measurement_buffer, buffer_length);
  debugPrintLn("]");
  
  int current_buffer_index = buffer_length - 2;
  int current_shifted_buffer_index = shifted_buffer_length - 2;
  //print(measurement_buffer[current_buffer_index])
  shifted_buffer[current_shifted_buffer_index+1] = measurement_buffer[current_buffer_index+1];
  int current_bits_to_shift = bits_per_int - bits_per_measurement;
  
  while (true) {
    if (current_buffer_index < 0) {
        break;
    }
    
    uint16_t new_element_to_shift = measurement_buffer[current_buffer_index];
    
      
    if (current_bits_to_shift >= bits_per_int) { // als vorige shifted_buffer element volledig leeg is
      shifted_buffer[current_shifted_buffer_index+1] = new_element_to_shift;
      current_bits_to_shift = bits_per_int - bits_per_measurement;
    } else {
      // als er nog ruimte is om op te vullen door te shiften
      uint16_t previous_shifted_buffer_element = shifted_buffer[current_shifted_buffer_index+1];

      // vul vorig shifted_buffer element
      uint16_t bits_to_shift = uint16_t(new_element_to_shift << (bits_per_int - current_bits_to_shift));
      previous_shifted_buffer_element = previous_shifted_buffer_element + bits_to_shift;
      shifted_buffer[current_shifted_buffer_index+1] = previous_shifted_buffer_element;
      
      // plaats overblijvende bits in huidig shifted_buffer element
      if (current_shifted_buffer_index >= 0) {
          uint16_t leftover_bits = new_element_to_shift >> current_bits_to_shift;
          shifted_buffer[current_shifted_buffer_index] = leftover_bits;
          current_bits_to_shift = current_bits_to_shift + (bits_per_int - bits_per_measurement);
          current_shifted_buffer_index --;
      }
    }
    current_buffer_index --;
  
  }

  debugPrint("Shifted data : [");
  printInt16Array(shifted_buffer, shifted_buffer_length);
  debugPrintLn("]");
}


void splitBuffer(uint16_t *before_buffer, uint8_t before_buffer_length, uint8_t *after_buffer) {
  debugPrint("Splitting data : [");
  printInt16Array(before_buffer, before_buffer_length);
  debugPrintLn("]");
  
  for(int i=0; i<before_buffer_length; i++) {
    int element = before_buffer[i];
    uint8_t part_1 = element >> (before_buffer_length);
    uint8_t part_2 = (uint8_t)element;//((uint8_t)(element << (before_buffer_length))) >> (before_buffer_length);
    after_buffer[2*i] = part_1;
    after_buffer[2*i+1] = part_2;
  }

  debugPrint("Splitted data : [");
  printInt8Array(after_buffer, before_buffer_length*2);
  debugPrintLn("]");
}


void encryptData(uint8_t *data_to_encrypt, uint8_t *encrypted_data, uint8_t *encryption_tag) {
  debugPrint("Ecrypting data : [");
  printInt8Array(data_to_encrypt, shifted_buffer_length*2);
  debugPrintLn("]");

  preparing(Key, IV, const0, const1);
  encryption(data_to_encrypt, S0, S1, S2, S3, S4);
  createTag(S0, S1, S2, S3, S4, msglen, adlen);

  for (int t; t<16; t++) {
    encrypted_data[t] = cipherTextBlocksend[t];
    encryption_tag[t] = tagsend[t];
  }


  debugPrint("Encrypted data : [");
  printInt8Array(encrypted_data, shifted_buffer_length*2);
  debugPrintLn("]");

  debugPrint("Encryption tag : [");
  printInt8Array(encryption_tag, shifted_buffer_length*2);
  debugPrintLn("]");
}


void fillBufferWithTestData(uint16_t *measurement_buffer, uint8_t buffer_length) {
  for(uint16_t i=0; i<buffer_length; i++) {
    measurement_buffer[i] = i;
  }
}

void printInt16Array(uint16_t *array, const int size_of_array) {
  if (DEBUG){
    for(int i = 0; i < size_of_array; i++) {
      debugPrint((String)array[i]);
      debugPrint(",");
    }
  }
}

void printInt8Array(uint8_t *array, const int size_of_array) {
  if (DEBUG){
    for(int i = 0; i < size_of_array; i++) {
      debugPrint((String)array[i]);
      debugPrint(",");
    }
  }
}

void printArray(char *array) {
  if (DEBUG){
    int size_of_array = strlen(array);
    for(int i = 0; i < size_of_array; i++) {
      debugPrint((String)array[i]);
    }
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
