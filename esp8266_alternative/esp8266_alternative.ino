#define SAMPLE_COUNT // print amount of samples per second 
#define SAMPLE_ECG
#define SAMPLE_PPG
#include "data_processing.h";

const uint8_t digitalPins[3] = {2,3,4}; // S1 = 12, S0 = 14, S2 = ??, three digital pins to control the multiplexer
const uint8_t input_pin = A0;

#ifdef SAMPLE_ECG
const float ecg_samplefreq  = 250;
const int ecg_interval  = round(1e6/ecg_samplefreq);
unsigned long ecg_last_micros = micros();
uint8_t sample_count_ecg = 0;
uint16_t ecg_samples[8];
const uint8_t ecg_pin = 0;
#endif

#ifdef SAMPLE_PPG
const float ppg_samplefreq  = 50;
const int ppg_interval  = round(1e6/ppg_samplefreq);
unsigned long ppg_last_micros = micros();
uint8_t sample_count_ppg = 0;
uint16_t ppg_samples[8];
const uint8_t ppg_pin = 1;
#endif

#ifdef SAMPLE_COUNT
uint16_t sample_count = 0;
unsigned long last_micros = micros();
#endif

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(input_pin, INPUT);
  for (uint8_t i=0; i<3; i++){
    pinMode(digitalPins[i], OUTPUT);
    digitalWrite(digitalPins[i], LOW);
  }
  delay(2000);
}

void loop() {
  // put your main code here, to run repeatedly:

  #ifdef SAMPLE_ECG
  if (micros() - ecg_last_micros >= ecg_interval) {
    ecg_last_micros += ecg_interval;
    pinSelect(ecg_pin);
    ecg_samples[sample_count_ecg] = sample();
    sample_count_ecg++;
    if (sample_count_ecg == 8){
      send_data();
      delay(3);
      sample_count_ecg = 0;
    }
  }
  #endif

  #ifdef SAMPLE_PPG
  if (micros() - ppg_last_micros >= ppg_interval) {
    ppg_last_micros += ppg_interval;
    pinSelect(ppg_pin);
    ppg_samples[sample_count_ppg] = sample();
    sample_count_ppg++;
    if (sample_count_ppg == 8){
      send_data();
      delay(3);
      sample_count_ppg = 0;
    }
  }
  #endif
  
  #ifdef SAMPLE_COUNT
  if (micros() - last_micros >= 1e6){
    Serial.println(sample_count);
    sample_count = 0;
    last_micros += 1e6;
  }
  #endif 
}

uint16_t sample(){
  #ifdef SAMPLE_COUNT
  sample_count++;
  #endif
  //return system_adc_read();
  return analogRead(input_pin);
}

void pinSelect(uint8_t sensor_pin){   // sensor_pin is a number between 0 and 3
  for (uint8_t x=0; x<2;x++){
    byte state = bitRead(sensor_pin, x);    // bitRead reads the bit at position x of the number starting from the right, state can be 0 or 1
    digitalWrite(digitalPins[x], state);    
  }
}
