int bits_per_encryption = 128;
int bits_per_measurement = 10;
int bits_per_int = 16;


const int measurement_1_buffer_length = 12; //bits_per_encryption / bits_per_measurement;
int measurement_1_buffer_index;
unsigned int measurement_1_buffer[measurement_1_buffer_length];
bool is_buffer_1_empty;

const int measurement_1_shifted_buffer_length = 8; //bits_per_encryption / bits_per_int
unsigned int measurement_1_shifted_buffer[measurement_1_shifted_buffer_length];


void setup() {
  Serial.begin(115200);
  delay(1000);
}

void loop() {
  Serial.println("Original list");
  fillBufferWithTestData(measurement_1_buffer, 12);
  Serial.println(measurement_1_buffer[1]);
  printBuffer(measurement_1_buffer, 12);
  delay(1000);
  
  Serial.println("Shifted list");
  bitShift(measurement_1_buffer, 12, measurement_1_shifted_buffer, measurement_1_shifted_buffer_length); 
  printBuffer(measurement_1_shifted_buffer, measurement_1_shifted_buffer_length);
  delay(1000);
  
  Serial.println("Unshifted list");
  unsigned int result_buffer[12];
  fillBufferWithZeros(result_buffer, 12);
  unBitShift(result_buffer, 12, measurement_1_shifted_buffer, measurement_1_shifted_buffer_length); 
  printBuffer(result_buffer, 12);
  delay(4000);
}

void fillBufferWithTestData(unsigned int *measurement_buffer, int buffer_length) {
  for(uint16_t i=0; i<buffer_length; i++) {
    measurement_buffer[i] = i;
  }
  
  printBuffer(measurement_buffer, buffer_length);
}

void fillBufferWithZeros(unsigned int *measurement_buffer, int buffer_length) {
  for(uint16_t i=0; i<buffer_length; i++) {
    measurement_buffer[i] = 0;
  }
}

uint16_t correctInt(int element_to_correct) {
  
}


void printBuffer(unsigned int *measurement_buffer, int buffer_length) {
  Serial.print("[ ");
  for(int i=0; i<buffer_length; i++) {
    Serial.print(measurement_buffer[i]);
    Serial.print(", ");
  }
  Serial.println(" ]");
}

void bitShift(unsigned int *measurement_buffer, int buffer_length, unsigned int *shifted_buffer, int shifted_buffer_length) {
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
      int bits_to_shift = scaleInt(new_element_to_shift << (bits_per_int - current_bits_to_shift));
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

void unBitShift(unsigned int *measurement_buffer, int buffer_length, unsigned int *shifted_buffer, int shifted_buffer_length) {
  int current_buffer_index = 12 - 1;
  int current_shifted_buffer_index = 8 - 1;
  int current_bits_to_shift = bits_per_int - bits_per_measurement;

  while (true) {
    printBuffer(measurement_buffer, 12);
    if (current_buffer_index < 0) {
        break;
    }
    
    int new_element_to_shift = shifted_buffer[current_shifted_buffer_index];
    
    if (current_bits_to_shift >= bits_per_int) {
        current_bits_to_shift = bits_per_int - bits_per_measurement;
    } else {
        int current_buffer_element = measurement_buffer[current_buffer_index];
        int bits_to_stay = scaleInt(new_element_to_shift << (current_bits_to_shift)) >> (bits_per_int - bits_per_measurement);
        current_buffer_element = current_buffer_element + bits_to_stay;
        measurement_buffer[current_buffer_index] = current_buffer_element;
        

        int bits_to_shift = scaleInt(new_element_to_shift >> (bits_per_int - current_bits_to_shift));
        measurement_buffer[current_buffer_index-1] = bits_to_shift;

        
        current_bits_to_shift = current_bits_to_shift + (bits_per_int - bits_per_measurement);
        current_shifted_buffer_index --;
    }
    current_buffer_index --;
  }
}

int scaleInt(int integer) {
  return uint16_t(integer);
}
