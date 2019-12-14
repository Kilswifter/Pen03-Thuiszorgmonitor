#include <ESP8266WiFi.h>
#include <map>

// INPUT/OUTPUT //
const uint8_t input_pin = A0; // analoge pin voor metingen

// SAMPLE //
float sample_frequency  = 100; 
int sample_interval  = round(1e6/sample_frequency);
unsigned long last_micros;              

// BUFFERS //
const uint8_t buffer_length = 12;
const uint8_t shifted_buffer_length = 8;

uint16_t samples[buffer_length] = {0};                        
uint8_t sample_count = 0;

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
bool NORMALMODE = true;
bool TESTDATA = false;
bool SINGLEVALUE = false;

std::map<String, bool*> bool_map = {
    { "DEBUG", &DEBUG },
    { "NORMALMODE", &NORMALMODE },
    { "TESTDATA", &TESTDATA },
    { "SINGLEVALUE", &SINGLEVALUE },
};


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(1000);

  // INPUT/OUTPUT //
  pinMode(input_pin, INPUT); // pin voor analoge data

  unsigned long last_micros = micros(); 
}

void loop() {
  
  if (NORMALMODE == true) {
    // SAMPLE //
    unsigned long diff_micros = micros() - last_micros;
    if (diff_micros >= sample_interval) {
      
      if (diff_micros > 4*sample_interval) {
        last_micros = micros();
      } else { last_micros += sample_interval; }

      //debugPrint("Sampeling sensor - ");
      samples[sample_count] = analogRead(input_pin); // sampling
      if (samples[sample_count] != 0 ) { Serial.println((String)samples[sample_count]); }
      sample_count = sample_count + 1;
      if (SINGLEVALUE == true) { sample_count = 12; }
  
      if (sample_count == buffer_length){
        sample_count = 0;
        for (int s=0; s<12; s++) { samples[s] = 0; }
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
      debugPrintLn("Message : " + (String)part_of_message);
      
      String command = (String)part_of_message; 

      // set multiplexer fixed to only one sensor
      if (command == "SAMPLEINTERVAL") {
        sample_interval = ((String)strtok_r(p, "/", &p)).toInt();
      } 
      if (command == "SAMPLEFREQ") {
        sample_interval = round(1e6/(float)((String)strtok_r(p, "/", &p)).toInt());
      } 
      if (bool_map.count(command) > 0) {
        *bool_map[command] = !*bool_map[command];
      }

      

    /*
      if (NORMALMODE == false) {
        n_samples[2][sample_count_n[2]] = (uint16_t)((String)part_of_message).toInt();  
        sample_count_n[2] = sample_count_n[2] + 1;

        uint8_t &sample_count = sample_count_n[2];
        uint16_t *samples = n_samples[2];

        if (sample_count == buffer_length) {
          sample_count_n[2] = 0;
          sendDataBuffer(0, samples);
        }
      }
      */

      
      current_index ++;
    }
  serial_new_data = false;
  }
}


void printInt16Array(uint16_t *array, const int size_of_array) {
  for(int i = 0; i < size_of_array; i++) {
    debugPrint((String)array[i]);
    debugPrint(" ");
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
