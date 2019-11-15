l//#include

const uint8_t digitalPins[3] = {2,3,4}; // three digital outputs to control multiplexer
const uint8_t input_pin = A0;

const float samplefreq = 50;
const int pres_interval = round(1e6/samplefreq);
unsigned long last_micros = micros();
uint8_t sample_count;
uint16_t pressure_A[8];
uint16_t pressure_B[8];
uint16_t pressure_C[8];
uint16_t pressure_D[8];

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
  
  if (micros() - last_micros >= pres_interval) {
    last_micros += pres_interval;
    pinSelect(0);
    pressure_A[sample_count] = analogRead(A0);
    pinSelect(1);
    pressure_B[sample_count] = analogRead(A0);
    pinSelect(2);
    pressure_C[sample_count] = analogRead(A0);
    pinSelect(3);
    pressure_D[sample_count] = analogRead(A0);
    sample_count++;
    if (sample_count == 8){
      sample_count = 0;
      // send_data(data, sensor_type);
    }
  }
}

void pinSelect(uint8_t sensor_pin){   // pin is a number between 0 and 3
  for (uint8_t x=0; x<2; x++){
    byte state = bitRead(sensor_pin, x);    // bitRead reads the bit at position x of the number starting from the right, state can be 0 or 1
    digitalWrite(digitalPins[x], state);    
  }
}
