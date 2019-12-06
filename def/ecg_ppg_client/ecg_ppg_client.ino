#include <ESP8266WiFi.h>
/*
Shifting data : [0,1,2,3,4,5,6,7,8,9,10,11,]
Shifted data : [0,1026,48,4101,96,7176,144,10251,]
Splitting data : [0,1026,48,4101,96,7176,144,10251,]
Splitted data : [0,0,64,2,3,48,0,5,6,96,192,8,9,144,128,11,]
 */
bool DEBUG = true;
int client_id = 1;

// WIFI //
const char* ssid = "ESP_reciever_1";
const char* password = "123456789";
const char* IP = "192.168.4.1";
uint16_t PORT = 1883;

WiFiClient client;

// INPUT/OUTPUT //
const uint8_t digitalPins[3] = {2,3,4}; // three digital outputs to control multiplexer !!!! numbers are wrong !!!!!
const uint8_t input_pin = A0;

// SAMPLE //
const float ecg_samplefreq  = 250;
const float ppg_samplefreq  = 50;
const int ecg_interval  = round(1e6/ecg_samplefreq);
unsigned long last_micros;              
const uint8_t counter_ppg_max = round(ecg_samplefreq/ppg_samplefreq);
uint8_t counter_ppg = counter_ppg_max-1;

uint8_t sample_count_ecg = 0;
uint8_t sample_count_ppg = 0;
uint8_t sample_count_serial = 0;
uint8_t sample_count_n[3] = {sample_count_ecg, sample_count_ppg, sample_count_serial};

const uint8_t ecg_pin = 0;
const uint8_t ppg_pin = 1;
const uint8_t n_pin[2] = {ecg_pin, ppg_pin};

// BUFFERS //
const uint8_t buffer_length = 12;
const uint8_t shifted_buffer_length = 8;

uint16_t ecg_samples[buffer_length];
uint16_t ppg_samples[buffer_length];
uint16_t serial_samples[buffer_length];
uint16_t *n_samples[3] = {ecg_samples, ppg_samples, serial_samples};

//uint16_t shifted_buffer_ecg[shifted_buffer_length];
//uint16_t shifted_buffer_ppg[shifted_buffer_length];
//uint16_t *shifted_buffer_n[2] = {shifted_buffer_ecg, shifted_buffer_ppg};

//uint8_t encoded_message_ecg[shifted_buffer_length*2];
//uint8_t encoded_message_ppg[shifted_buffer_length*2];
//uint8_t *encoded_message_n[2] = {encoded_message_ecg, encoded_message_ppg};

uint8_t bits_per_encryption = 128;
uint8_t bits_per_measurement = 10;
uint8_t bits_per_int = 16;

// communication buffers
const byte serial_buffer_length = 32;          // predefined max buffer size
char serial_buffer[serial_buffer_length];     // buffer for received messages
boolean serial_new_data = false;           // bool to check if message is complete

// global run variables
bool send_live_data = true;

bool FIXEDSENSOR = false;
int FIXEDSENSORID = 0;
bool NORMALMODE = true;




void setup_wifi() {
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("");
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


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(2000);

  // WIFI //
  setup_wifi();

  // INPUT/OUTPUT //
  pinMode(input_pin, INPUT);
  for (uint8_t i=0; i<3; i++){
    pinMode(digitalPins[i], OUTPUT);
    digitalWrite(digitalPins[i], LOW);
  } 

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
    if (micros() - last_micros >= ecg_interval) {


      
      last_micros += ecg_interval;
      
      // ecg sample [0]
      Serial.println("ECG: " + (String)sample_count_n[0]);
      n_samples[0][sample_count_n[0]] = analogRead(input_pin);  
      sample_count_n[0] = sample_count_n[0] + 1;
      
  
      // ppg sample [1]
      counter_ppg++;
      if (counter_ppg == counter_ppg_max){
        Serial.println("PPG: " + (String)sample_count_n[1]);
        pinSelect(n_pin[1]);
        n_samples[1][sample_count_n[1]] = analogRead(input_pin);
        sample_count_n[1] = sample_count_n[1] + 1;
        counter_ppg = 0;
  
      }
  
      
  
      // check if buffers are full 
      for (int q=0; q<2; q++){
        int n = 0;
        if (FIXEDSENSOR == true) {
          n = FIXEDSENSORID; 
        } else {
          n = q;
        }
        
        uint8_t &sample_count = sample_count_n[n];
        uint16_t *samples = n_samples[n];
        //uint16_t *shifted_buffer = shifted_buffer_n[n];
        //uint8_t *encoded_message = encoded_message_n[n];
        
        if (sample_count == buffer_length){
        
          sample_count = 0;
  
          sendDataBuffer(n, samples);

          /*          
          fillBufferWithTestData(samples, buffer_length);
          
          uint16_t *data_to_shift = samples;
          uint16_t shifted_data[shifted_buffer_length];
          bitShift(data_to_shift, 12, shifted_data, 8);
  
          
          uint16_t *data_to_split = shifted_data;
          uint8_t splitted_data[shifted_buffer_length*2];
          splitBuffer(data_to_split, shifted_buffer_length, splitted_data);
          
          uint8_t *data_to_encrypt = splitted_data;
          //printInt8Array(data_to_encrypt, 16);
          //uint8_t encrypted_data[shifted_buffer_length*2];
          //encryptData(data_to_encrypt, encrypted_data);
          //printInt8Array(encrypted_data, 16);
  
  
          printInt8Array(data_to_encrypt, 16);
  
          client.write(n);
          client.write(data_to_encrypt, 17);
          */

        }
      }

     
    } 
    
  }
  

}


/**
################################################################################

                                Functions

################################################################################
**/


void receiveSerial() {
  static byte character_index = 0;
  char end_marker = '\n';
  char new_character;

  if (Serial.available() > 0) {
    Serial.println("Serial data available");
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


void actOnNewSerialData() {
  if (serial_new_data == true) {
    char *p = serial_buffer;
    char *part_of_message;
    int current_index = 0;
    while ((part_of_message = strtok_r(p, "/", &p)) != NULL) {
      Serial.print("Message : " + (String)part_of_message + " ");
      Serial.println(strtok_r(p, "/", &p));
      //serial_received_request[current_index] = part_of_message;
      debugPrintLn(part_of_message);

      // set multiplexer fixed to only one sensor
      if (String(part_of_message) == "FIXEDSENSORON") {
        FIXEDSENSOR = true;
      }
      if (String(part_of_message) == "FIXEDSENSOROFF") {
        FIXEDSENSOR = false;
      }

      // 
      if (String(part_of_message) == "SETSENSORECG") {
        FIXEDSENSORID = 0;
      }

      if (String(part_of_message) == "SETSENSORPPG") {
        FIXEDSENSORID = 0;
      }

      if (String(part_of_message) == "SETNORMALMODEOFF") {
        NORMALMODE = false;
      }

      if (String(part_of_message) == "SETNORMALMODEON") {
        NORMALMODE = true;
      }

      if (NORMALMODE == false) {
        n_samples[2][sample_count_n[2]] = (uint16_t)((String)part_of_message).toInt();  
        sample_count_n[2] = sample_count_n[2] + 1;

        uint8_t &sample_count = sample_count_n[2];
        uint16_t *samples = n_samples[2];

        if (sample_count == buffer_length) {
          sample_count = 0;
          sendDataBuffer(0, samples);
        }
      }

      
      current_index ++;
    }
  serial_new_data = false;
  }
}



void pinSelect(uint8_t sensor_pin){   // pin is a number between 0 and 3
  for (uint8_t x=0; x<2; x++){
    byte state = bitRead(sensor_pin, x);    // bitRead reads the bit at position x of the number starting from the right, state can be 0 or 1
    digitalWrite(digitalPins[x], state);    
  }
}


void sendDataBuffer(int topic_id, uint16_t *data_buffer) {
        
  uint16_t *data_to_shift = data_buffer;
  uint16_t shifted_data[shifted_buffer_length];
  bitShift(data_to_shift, 12, shifted_data, 8);
  
  uint16_t *data_to_split = shifted_data;
  uint8_t splitted_data[shifted_buffer_length*2];
  splitBuffer(data_to_split, shifted_buffer_length, splitted_data);
  
  uint8_t *data_to_encrypt = splitted_data;
  printInt8Array(data_to_encrypt, 16);
  
  client.write(topic_id);
  client.write(data_to_encrypt, 17);
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

/*

void bitShift(uint16_t *measurement_buffer, uint8_t buffer_length, uint16_t *shifted_buffer, uint8_t shifted_buffer_length) {
  debugPrint("Shifting data : [");
  printInt16Array(measurement_buffer, buffer_length);
  debugPrintLn("]");
  
  uint8_t current_buffer_index = buffer_length - 2;
  uint8_t current_shifted_buffer_index = shifted_buffer_length - 2;
  //print(measurement_buffer[current_buffer_index])
  shifted_buffer[current_shifted_buffer_index+1] = measurement_buffer[current_buffer_index+1];
  uint16_t current_bits_to_shift = bits_per_int - bits_per_measurement;
  
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
      uint16_t bits_to_shift = (uint16_t)(new_element_to_shift << (bits_per_int - current_bits_to_shift));
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
*/

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


void encryptData(uint8_t *data_to_encrypt, uint8_t *encrypted_data) {
  debugPrint("Ecrypting data : [");
  printInt8Array(data_to_encrypt, shifted_buffer_length*2);
  debugPrintLn("]");
  
  encrypted_data = data_to_encrypt;

  debugPrint("Encrypted data : [");
  printInt8Array(encrypted_data, shifted_buffer_length*2);
  debugPrintLn("]");
}


void fillBufferWithTestData(uint16_t *measurement_buffer, uint8_t buffer_length) {
  for(uint16_t i=0; i<buffer_length; i++) {
    measurement_buffer[i] = i;
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
