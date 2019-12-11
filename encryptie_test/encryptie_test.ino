#include <ESP8266WiFi.h>
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
Splitted data : [0,0,64,2,3,48,0,5,6,96,192,8,9,144,128,11,]
*/

bool DEBUG = true;
int client_id = 1;

// SAMPLE //
const float samplefreq  = 12;
const int interval  = round(1e6/samplefreq);
unsigned long last_micros;              

uint8_t sample_count = 0;

// BUFFERS //
const uint8_t buffer_length = 12;
const uint8_t shifted_buffer_length = 8;

uint16_t samples[buffer_length] = {0};

uint8_t bits_per_encryption = 128;
uint8_t bits_per_measurement = 10;
uint8_t bits_per_int = 16;

// RUN VARIABLES //
bool send_live_data = true;

bool FIXEDSENSOR = true;
int FIXEDSENSORID = 0;
bool NORMALMODE = true;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(1000);
  Serial.println("test");

  unsigned long last_micros = micros(); 
}

void loop() {

  if (NORMALMODE == true) {
    // SAMPLE //
    if (micros() - last_micros >= interval) {
      
      last_micros += interval;
      sample_count++;
        
      if (sample_count == buffer_length){

        
        sample_count = 0;

        fillBufferWithTestData(samples, buffer_length);
        sendDataBuffer(0, samples);

        }
      }
    } 
}


/**
################################################################################

                                Functions

################################################################################
**/

/*
void pinSelect(uint8_t sensor_pin){   // pin is a number between 0 and 3
  for (uint8_t x=0; x<2; x++){
    byte state = bitRead(sensor_pin, x);    // bitRead reads the bit at position x of the number starting from the right, state can be 0 or 1
    digitalWrite(digitalPins[x], state);    
  }
}
*/

void sendDataBuffer(int topic_id, uint16_t *data_buffer) {
        
  uint16_t *data_to_shift = data_buffer;
  uint16_t shifted_data[shifted_buffer_length];
  bitShift(data_to_shift, 12, shifted_data, 8);
  
  uint16_t *data_to_split = shifted_data;
  uint8_t splitted_data[shifted_buffer_length*2];
  splitBuffer(data_to_split, shifted_buffer_length, splitted_data);
  
  uint8_t *data_to_encrypt = splitted_data;
  uint8_t data_to_send[shifted_buffer_length*2];
  uint8_t encryption_tag[shifted_buffer_length*2];
  encryptData(data_to_encrypt, data_to_send, encryption_tag);
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

  int plaintext[shifted_buffer_length*2];
  add_data_to_int_buffer(data_to_encrypt, plaintext, shifted_buffer_length*2);

  //encryption
  int time_start = micros();
  preparing(Key, IV, const0, const1);  // 3730 us
  //int time_start = micros();
  encryption(plaintext, S0, S1, S2, S3, S4); // 461 us
  //int time_start = micros();
  createTag(S0, S1, S2, S3, S4, msglen, adlen);
  int time_end = micros();
  int encryption_time = time_end - time_start;
  Serial.println(encryption_time);
  
  add_data_to_uint8_t_buffer(cipherTextBlocksend, encrypted_data, shifted_buffer_length*2);
  add_data_to_uint8_t_buffer(tagsend, encryption_tag, shifted_buffer_length*2);

  debugPrint("Encrypted data : [");
  printInt8Array(encrypted_data, shifted_buffer_length*2);
  debugPrintLn("]");

  debugPrint("encryption tag : [");
  printInt8Array(encryption_tag, shifted_buffer_length*2);
  debugPrintLn("]");

  // decryption
  //preparing(Key, IV, const0, const1);
  //decryption(cipherTextBlock, S0, S1, S2, S3, S4);
  //createTag(S0, S1, S2, S3, S4, msglen, adlen);
}

void fillBufferWithTestData(uint16_t *measurement_buffer, uint8_t buffer_length) {
  for(uint16_t i=0; i<buffer_length; i++) {
    measurement_buffer[i] = i;
  }
}

void printInt32Array(int *array, const int size_of_array) {
  for(int i = 0; i < size_of_array; i++) {
    debugPrint((String)array[i]);
    debugPrint(",");
  }
}

void printInt16Array(uint16_t *array, const int size_of_array) {
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

void add_data_to_uint8_t_buffer(int buffer_1[shifted_buffer_length*2], uint8_t *buffer_2, uint8_t buffer_length){

   for (int i=0; i<buffer_length; i++){
    buffer_2[i] = (uint8_t)buffer_1[i];
   }
}

void add_data_to_int_buffer(uint8_t *buffer_1, int buffer_2[shifted_buffer_length*2], uint8_t buffer_length){

   for (int i=0; i<buffer_length; i++){
    buffer_2[i] = (uint8_t)buffer_1[i];
   }
}
