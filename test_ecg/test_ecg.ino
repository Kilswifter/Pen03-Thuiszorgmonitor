#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// WIFI //
const char* ssid = "ESP_reciever_1";
const char* password = "123456789";
const char* mqtt_server = "192.168.4.1";

WiFiClient client;

const uint8_t input_pin = A0;

const float ecg_samplefreq  = 250;
const int ecg_interval  = round(1e6/ecg_samplefreq);
unsigned long last_micros = micros();  

uint8_t sample_count_ecg = 0;
const uint8_t buffer_length = 12;
const uint8_t shifted_buffer_length = 8;
uint16_t ecg_samples[buffer_length];
uint8_t test1[16] = {5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5};
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
  // put your main code here, to run repeatedly:
  client.write(0);
  client.write(test1, 17);
  delay(500);
  client.write(1);
  client.write(test2, 17);
  delay(500);
  
}
