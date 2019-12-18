
byte sinetable[256]={
   128, 131, 134, 137, 140, 143, 146, 149, 152, 155, 158, 161, 164, 167, 170, 173,
   176, 179, 182, 185, 187, 190, 193, 195, 198, 201, 203, 206, 208, 210, 213, 215,
   217, 219, 222, 224, 226, 228, 230, 231, 233, 235, 236, 238, 240, 241, 242, 244,
   245, 246, 247, 248, 249, 250, 251, 251, 252, 253, 253, 254, 254, 254, 254, 254,
   255, 254, 254, 254, 254, 254, 253, 253, 252, 251, 251, 250, 249, 248, 247, 246,
   245, 244, 242, 241, 240, 238, 236, 235, 233, 231, 230, 228, 226, 224, 222, 219,
   217, 215, 213, 210, 208, 206, 203, 201, 198, 195, 193, 190, 187, 185, 182, 179,
   176, 173, 170, 167, 164, 161, 158, 155, 152, 149, 146, 143, 140, 137, 134, 131,
   128, 124, 121, 118, 115, 112, 109, 106, 103, 100, 97, 94, 91, 88, 85, 82,
   79, 76, 73, 70, 68, 65, 62, 60, 57, 54, 52, 49, 47, 45, 42, 40,
   38, 36, 33, 31, 29, 27, 25, 24, 22, 20, 19, 17, 15, 14, 13, 11,
   10, 9, 8, 7, 6, 5, 4, 4, 3, 2, 2, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 2, 2, 3, 4, 4, 5, 6, 7, 8, 9,
   10, 11, 13, 14, 15, 17, 19, 20, 22, 24, 25, 27, 29, 31, 33, 36,
   38, 40, 42, 45, 47, 49, 52, 54, 57, 60, 62, 65, 68, 70, 73, 76,
   79, 82, 85, 88, 91, 94, 97, 100, 103, 106, 109, 112, 115, 118, 121, 124
};

int pin = 5;
float frequency = 10;
long interval = round(1e6/frequency);
unsigned long last_micros; 

int counter = 0;

const byte serial_buffer_length = 32;          // predefined max buffer size
char serial_buffer[serial_buffer_length];     // buffer for received messages
boolean serial_new_data = false;           // bool to check if message is complete

bool DEBUG = false;
float amplitude_a_fraction = 0.8;
float amplitude_b_fraction = 1 - amplitude_a_fraction;
float max_amplitude = 190;
float frequency_a_fraction = 1;
float frequency_b_fraction = 4;

void setup() {
  Serial.begin(115200);
  pinMode(pin, OUTPUT);  // sets the pin as output
  unsigned long last_micros = micros(); 
}

void loop() {
  
  unsigned long diff_micros = micros() - last_micros;
  if (diff_micros >= interval) {
        
    if (diff_micros > 4*interval) {
      last_micros = micros();
    } else { last_micros += interval; }

    int pwmval= round(sinetable[counter]*0.75);
    /*
    int amplitude_a = round(max_amplitude*amplitude_a_fraction);
    int amplitude_b = round(max_amplitude*amplitude_b_fraction);
    int pwmval = calculateSignal(amplitude_a, frequency_a_fraction, amplitude_b, frequency_b_fraction, counter);
    */
    Serial.println(pwmval);
    analogWrite(pin, pwmval);
    counter++;

    if (counter == 256) { counter = 0; }
  }
  receiveSerial();
  actOnNewSerialData();
}

int calculateSignal(int amplitude_a, float frequency_a, int amplitude_b, float frequency_b, int t) {
  int signal_a = round(amplitude_a*sin(frequency_a*t/40));
  int signal_b = round(amplitude_b*sin(frequency_b*t/40 + M_PI/2));
  int signal_ab = signal_a + signal_b;
  return signal_ab;
}

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
      
      if (command == "DEBUG") {
        DEBUG = !DEBUG;
      } 
      if (command == "SIGNALINTERVAL") {
        interval = long(((String)strtok_r(p, "/", &p)).toInt());
        debugPrintLn((String)interval);
      } 
      if (command == "SIGNALFREQ") {
        /*
        int a = ((String)strtok_r(p, "/", &p)).toInt();
        int b = 1000000;
        Serial.println(a);
        Serial.println(b);
        int c = b/a; 
        Serial.println(c);
        Serial.println((10^6/((((String)strtok_r(p, "/", &p)).toInt()))));
        */
        interval = long(round(1e6/(float)((String)strtok_r(p, "/", &p)).toInt()));
        debugPrintLn((String)interval);
      } 
      
      current_index ++;
    }
  serial_new_data = false;
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
