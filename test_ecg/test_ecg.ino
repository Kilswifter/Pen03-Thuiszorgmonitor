#include <ESP8266WiFi.h>

// WIFI //
const char* ssid = "ESP_reciever_1";
const char* password = "123456789";
const char* mqtt_server = "192.168.4.1";
int bits_per_int = 16;
int bits_per_measurement = 10;

WiFiClient client;

const uint8_t input_pin = A0;

const float ecg_samplefreq  = 250;
const int ecg_interval  = round(1e6/ecg_samplefreq);
unsigned long last_micros = micros();  

uint8_t sample_count_ecg = 0;
const uint8_t buffer_length = 12;
const uint8_t shifted_buffer_length = 8;
uint16_t ecg_samples[buffer_length];
uint16_t test1[12] = {1,2,3,4,5,6,7,8,9,10,11,12};
uint8_t test2[16] = {8,1,1,1,1,1,1,1,1,1,1,1,1,1,1,8};

void setup_wifi() {

  delay(10);
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

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("");
    if (client.connect(mqtt_server, 1883)) {
      Serial.println("reconnected");
    } else {
      Serial.println("failed");
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
  client.connect(mqtt_server, 1883);
}

void loop() {
  /*
  uint16_t *measurement_buffer = test1;
  int measurement_buffer_length = 12;

  uint16_t *data_to_shift = measurement_buffer;
  uint16_t shifted_data[8];
  bitShift(data_to_shift, 12, shifted_data, 8);

  uint16_t *data_to_split = shifted_data;
  uint16_t splitted_data[16];
  splitBuffer(data_to_split, 8, splitted_data);

  uint16_t *data_to_encrypt = splitted_data;
  uint16_t encrypted_data[16];
  encryptData(data_to_encrypt, encrypted_data);
  */
  
  client.write(0);
  client.write(test2, 17);
  delay(500);
  //client.write(1);
  //client.write(test2, 17);
  //delay(500);
  
}

void bitShift(uint16_t *measurement_buffer, int buffer_length, uint16_t *shifted_buffer, int shifted_buffer_length) {
  int current_buffer_index = buffer_length - 2;
  int current_shifted_buffer_index = shifted_buffer_length - 2;
  //print(measurement_buffer[current_buffer_index])
  shifted_buffer[current_shifted_buffer_index+1] = measurement_buffer[current_buffer_index+1];
  int current_bits_to_shift = bits_per_int - bits_per_measurement;
  
  while (true) {
    if (current_buffer_index < 0) {
        break;
    }
    
    int new_element_to_shift = measurement_buffer[current_buffer_index];
    
      
    if (current_bits_to_shift >= bits_per_int) { // als vorige shifted_buffer element volledig leeg is
      shifted_buffer[current_shifted_buffer_index+1] = new_element_to_shift;
      current_bits_to_shift = bits_per_int - bits_per_measurement;
    } else {
      // als er nog ruimte is om op te vullen door te shiften
      int previous_shifted_buffer_element = shifted_buffer[current_shifted_buffer_index+1];

      // vul vorig shifted_buffer element
      int bits_to_shift = uint16_t(new_element_to_shift << (bits_per_int - current_bits_to_shift));
      previous_shifted_buffer_element = previous_shifted_buffer_element + bits_to_shift;
      shifted_buffer[current_shifted_buffer_index+1] = previous_shifted_buffer_element;
      
      // plaats overblijvende bits in huidig shifted_buffer element
      if (current_shifted_buffer_index >= 0) {
          int leftover_bits = new_element_to_shift >> current_bits_to_shift;
          shifted_buffer[current_shifted_buffer_index] = leftover_bits;
          current_bits_to_shift = current_bits_to_shift + (bits_per_int - bits_per_measurement);
          current_shifted_buffer_index --;
      }
    }
    current_buffer_index --;
  
  }
}

void splitBuffer(uint16_t *before_buffer, int before_buffer_length, uint16_t *after_buffer) {
  for(int i=0; i<before_buffer_length; i++) {
    int element = before_buffer[i];
    uint8_t part_1 = element >> (before_buffer_length/2);
    uint8_t part_2 = (uint8_t(element << (before_buffer_length/2))) >> (before_buffer_length/2);
    after_buffer[2*i] = part_1;
    after_buffer[2*i+1] = part_2;
  }
}


void encryptData(uint16_t *data_to_decrypt, uint16_t *decrypted_data) {
  decrypted_data = data_to_decrypt;
}
