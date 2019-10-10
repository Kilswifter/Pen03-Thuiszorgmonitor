
// includes
#include <ESP8266WiFi.h>        // Include the Wi-Fi library
#define LED_BUILTIN 2

// acces point config
const char *ssid = "ESP8266 Access Point"; // The name of the Wi-Fi network that will be created
const char *password = "123456789";   // The password required to connect to it, leave blank for an open network

// TCP server
WiFiServer wifiServer(80);
const byte num_chars = 32;
char received_chars[num_chars];
boolean new_data = false;





void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println("\n");
  Serial.println("ESP started running");
  
  WiFi.softAP(ssid, password);             // Start the access point
  Serial.print("Access Point \"");
  Serial.print(ssid);
  Serial.println("\" started");
  Serial.print("IP address:\t");
  Serial.println(WiFi.softAPIP());         // Send the IP address of the ESP8266 to the computer

  wifiServer.begin();                      // Start TCP server

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

}

void loop() { 
//  exploreSurroundingReceivers
//  getClosedReceiver
//  buildNetwork

//  waitForDevices
//    getDataFromDivice
//    checkData

//    sendDataToRoot of decryptData

  WiFiClient client = wifiServer.available();  // make new connection
 
  if (client) {
    Serial.println("Client connected");
    while (client.connected()) {
      recvSerial();
      actOnNewData();




   
    }
 
    client.stop();
    Serial.println("Client disconnected");

  }
}





void recvSerial() {
  static byte ndx = 0;
  char endMarker = '\n';
  char rc;
  
  if (Serial.available() > 0) {
    digitalWrite(LED_BUILTIN, LOW);
    while (Serial.available() > 0 && new_data == false) {
      rc = Serial.read();
      if (rc != endMarker) {
        received_chars[ndx] = rc;
        ndx++;
        if (ndx >= num_chars) {
          ndx = num_chars - 1;
        }
      }
      else {
        received_chars[ndx] = '\0'; // terminate the string
        ndx = 0;
        new_data = true;
      }
    }
  }
}

void actOnNewData() {
  if (new_data == true) {
    Serial.println(received_chars);
    digitalWrite(LED_BUILTIN, LOW);
    delay(2000);
    digitalWrite(LED_BUILTIN, HIGH);
    
    

    
  new_data = false;
  }
}
