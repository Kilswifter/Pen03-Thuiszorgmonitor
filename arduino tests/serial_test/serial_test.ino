// COMMUNICATION //
const byte serial_buffer_length = 32;          // predefined max buffer size
char serial_buffer[serial_buffer_length];     // buffer for received messages
boolean serial_new_data = false;           // bool to check if message is complete

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("");
  Serial.println("Program started running");
}

void loop() {
  receiveSerial();
  actOnNewSerialData();
}

void receiveSerial() {
  static byte character_index = 0;
  char end_marker = '\n';
  char new_character;

  if (Serial.available() > 0) {
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
      Serial.println(part_of_message);
    }
  serial_new_data = false;
  }
}
