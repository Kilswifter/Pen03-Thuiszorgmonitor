int bits_per_encryption = 128;
int bits_per_measurement = 10;
int bits_per_int = 16;


const int measurement_1_buffer_length = 12; //bits_per_encryption / bits_per_measurement;
int measurement_1_buffer_index;
unsigned int measurement_1_buffer[measurement_1_buffer_length];
bool is_buffer_1_empty;

const int measurement_1_shifted_buffer_length = 8; //bits_per_encryption / bits_per_int
uint16_t measurement_1_shifted_buffer[measurement_1_shifted_buffer_length];






void setup() {
  Serial.begin(115200);
  // put your setup code here, to run once:

}



void loop() {
  fillBufferWithTestData(measurement_1_shifted_buffer, measurement_1_shifted_buffer_length);
  printBuffer16(measurement_1_shifted_buffer, measurement_1_shifted_buffer_length);

  uint8_t splitted_buffer[16];
  splitBuffer(measurement_1_shifted_buffer, measurement_1_shifted_buffer_length, splitted_buffer);
  printBuffer8(splitted_buffer, 16);

  uint16_t unsplitted_buffer[8];
  unSplitBuffer(splitted_buffer, measurement_1_shifted_buffer_length*2, unsplitted_buffer);
  printBuffer16(unsplitted_buffer, measurement_1_shifted_buffer_length);

  /*
  String converted = intToUnicode(measurement_1_shifted_buffer, measurement_1_shifted_buffer_length);
  Serial.println(converted);
  
  const int measurement_1_shifted_buffer_length = 8; //bits_per_encryption / bits_per_int
  unsigned int measurement_1_shifted_buffer[measurement_1_shifted_buffer_length];
  unicodeToInt(converted, measurement_1_shifted_buffer, measurement_1_shifted_buffer_length);
  printBuffer(measurement_1_shifted_buffer, 8);
  */
  /*
  char chars[5];
  GetUnicodeChar(32767, chars);
  Serial.println(chars);
  delay(1000);

  char a = 'à';
  String b = "\u00B2"; 

  int someInt = a - '0';
  Serial.println(someInt);
  Serial.println(int(a));
  */
  delay(1000);
}

String intToUnicode(unsigned int *measurement_buffer, int buffer_length) {
  String return_string = "";
  for (uint16_t i=0; i<buffer_length; i++) {
    char converted[5] = "";
    GetUnicodeChar(measurement_buffer[i]+1, converted);  // plus 1 om 0 te encoden
    return_string = return_string + converted;
  }
  return return_string;
}

void unicodeToInt(String unicode_string, unsigned int *measurement_buffer, int buffer_length){
  for (uint16_t i=0; i<8; i++) {
    measurement_buffer[i] = (unsigned int)unicode_string[i]-1;
    const char *test = &unicode_string[i];
    String testt = (String)unicode_string[i];
    //Serial.println(utf8_to_codepoint(test)-1);
    //Serial.println(static_cast<uint32_t>(unicode_string[i]));
    //Serial.println(int(unicode_string[i]));
    byte buff[20];
    unicode_string.getBytes(buff, 20);
    for(uint16_t i=0; i<10; i++) {
      Serial.print(buff[i],BIN);
    }
    Serial.println("");
    
  }
}


void fillBufferWithTestData(uint16_t *measurement_buffer, int buffer_length) {
  for(uint16_t i=0; i<buffer_length; i++) {
    measurement_buffer[i] = i+125;
  }

  measurement_buffer[0] = 0;
  measurement_buffer[1] = 1026;
  measurement_buffer[2] = 48;
  measurement_buffer[3] = 4101;
  measurement_buffer[4] = 96;
  measurement_buffer[5] = 7176;
  measurement_buffer[6] = 144;
  measurement_buffer[7] = 10251; 
}

void fillBufferWithZeros(unsigned int *measurement_buffer, int buffer_length) {
  for(uint16_t i=0; i<buffer_length; i++) {
    measurement_buffer[i] = 0;
  }
}


void printBuffer8(uint8_t *measurement_buffer, int buffer_length) {
  Serial.print("[ ");
  for(int i=0; i<buffer_length; i++) {
    Serial.print(measurement_buffer[i]);
    Serial.print(", ");
  }
  Serial.println(" ]");
}

void printBuffer16(uint16_t *measurement_buffer, int buffer_length) {
  Serial.print("[ ");
  for(int i=0; i<buffer_length; i++) {
    Serial.print(measurement_buffer[i]);
    Serial.print(", ");
  }
  Serial.println(" ]");
}

void splitBuffer(uint16_t *before_buffer, int before_buffer_length, uint8_t *after_buffer) {
  for(int i=0; i<before_buffer_length; i++) {
    int element = before_buffer[i];
    uint8_t part_1 = element >> (before_buffer_length/2);
    uint8_t part_2 = (scaleInt8(element << (before_buffer_length/2))) >> (before_buffer_length/2);
    after_buffer[2*i] = part_1;
    after_buffer[2*i+1] = part_2;
  }
}

void unSplitBuffer(uint8_t *before_buffer, int before_buffer_length, uint16_t *after_buffer) {
  for(int i=0; i<before_buffer_length/2; i++) {
    uint8_t part_1 = before_buffer[2*i];
    uint8_t part_2 = before_buffer[2*i+1];
    uint16_t element = (part_1 << (before_buffer_length/2)) + part_2;
    after_buffer[i] = element;
  }
}

int scaleInt8(int integer) {
  return uint8_t(integer);
}

void GetUnicodeChar(unsigned int code, char chars[5]) {
    if (code <= 0x7F) {
        chars[0] = (code & 0x7F); chars[1] = '\0';
    } else if (code <= 0x7FF) {
        // one continuation byte
        chars[1] = 0x80 | (code & 0x3F); code = (code >> 6);
        chars[0] = 0xC0 | (code & 0x1F); chars[2] = '\0';
    } else if (code <= 0xFFFF) {
        // two continuation bytes
        chars[2] = 0x80 | (code & 0x3F); code = (code >> 6);
        chars[1] = 0x80 | (code & 0x3F); code = (code >> 6);
        chars[0] = 0xE0 | (code & 0xF); chars[3] = '\0';
    } else if (code <= 0x10FFFF) {
        // three continuation bytes
        chars[3] = 0x80 | (code & 0x3F); code = (code >> 6);
        chars[2] = 0x80 | (code & 0x3F); code = (code >> 6);
        chars[1] = 0x80 | (code & 0x3F); code = (code >> 6);
        chars[0] = 0xF0 | (code & 0x7); chars[4] = '\0';
    } else {
        // unicode replacement character
        chars[2] = 0xEF; chars[1] = 0xBF; chars[0] = 0xBD;
        chars[3] = '\0';
    }
}


unsigned utf8_to_codepoint(const char *ptr) {
    if( *ptr < 0x80) return *ptr;
    //if( *ptr < 0xC0) throw unicode_error("invalid utf8 lead byte");
    unsigned result=0;
    int shift=0;
    if( *ptr < 0xE0) {result=*ptr&0x1F; shift=1;}
    if( *ptr < 0xF0) {result=*ptr&0x0F; shift=2;}
    if( *ptr < 0xF8) {result=*ptr&0x07; shift=3;}
    for(; shift>0; --shift) {
        ++ptr;
        //if (*ptr<0x7F || *ptr>=0xC0) 
          //return;
            //throw unicode_error("invalid utf8 continuation byte");
        result <<= 6;
        result |= *ptr&0x6F;
    }
    return result;
}


int codepoint(String u)
{
    int l = u.length();
    if (l<1) return -1; unsigned char u0 = u[0]; if (u0>=0   && u0<=127) return u0;
    if (l<2) return -1; unsigned char u1 = u[1]; if (u0>=192 && u0<=223) return (u0-192)*64 + (u1-128);
    if (u[0]==0xed && (u[1] & 0xa0) == 0xa0) return -1; //code points, 0xd800 to 0xdfff
    if (l<3) return -1; unsigned char u2 = u[2]; if (u0>=224 && u0<=239) return (u0-224)*4096 + (u1-128)*64 + (u2-128);
    if (l<4) return -1; unsigned char u3 = u[3]; if (u0>=240 && u0<=247) return (u0-240)*262144 + (u1-128)*4096 + (u2-128)*64 + (u3-128);
    return -1;
}
