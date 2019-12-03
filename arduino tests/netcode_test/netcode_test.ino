#include <ESP8266WiFi.h>

/*
const char* ssid = "Avenue Belle Vue";
const char* password = "Westfield19";
*/
char ssid[] = "ESP_reciever_1";
char pass[] = "123456789";
bool WiFiAP = true;
int counter = 0;

#define MAX_CLIENTS 3
#define MAX_LINE_LENGTH 16

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(1883);
WiFiClient *clients[MAX_CLIENTS] = { NULL };
uint16_t inputs[MAX_CLIENTS][MAX_LINE_LENGTH] = { 0 };
uint16_t buffer_index[MAX_CLIENTS] = { 0 } ;

void setup() {
  Serial.begin(115200);
  delay(10);

  // Start wifi acces point
  startWiFiAP();
  
  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());  
}

void loop() {
  // Check if a new client has connected
  WiFiClient newClient = server.available();
  if (newClient) {
    Serial.println("new client");
    // Find the first unused space
    for (int i=0 ; i<MAX_CLIENTS ; ++i) {
        if (NULL == clients[i]) {
            clients[i] = new WiFiClient(newClient);
            break;
        }
     }
  }

  // Check whether each client has some data
  for (int i=0 ; i<MAX_CLIENTS ; ++i) {
    // If the client is in use, and has some data...
    if (NULL != clients[i] && clients[i]->available() ) {

      uint16_t newBit = clients[i]->read();

      // add to buffer if it still has space
      if (buffer_index[i] < MAX_LINE_LENGTH) {
        inputs[i][buffer_index[i]] = newBit;
        buffer_index[i] = buffer_index[i] + 1;
      } 

      // when buffer is full
      else {
        //Serial.print("Buffer from client" + (String)i);
        printIntArray(inputs[i], MAX_LINE_LENGTH);
        Serial.println("");
        counter+=1;
        buffer_index[i] = 0;
      }

      
      
      

      /*
      // Read the data 
      char newChar = clients[i]->read();
      char *newerChar = newChar;
      // If we have the end of a string
      // (Using the test your code uses)
      if ('\r' == newerChar) {
        // Blah blah, do whatever you want with inputs[i]

        // Empty the string for next time
        inputs[i][0] = NULL;

        // The flush that you had in your code - I'm not sure
        // why you want this, but here it is
        clients[i]->flush();

        // If you want to disconnect the client here, then do this:
        clients[i]->stop();
        delete clients[i];
        clients[i] = NULL;

      } else {
        // Add it to the string
        strcat(inputs[i][i], newerChar);
        // IMPORTANT: Nothing stops this from overrunning the string and
        //            trashing your memory. You SHOULD guard against this.
        //            But I'm not going to do all your work for you :-)
        
      }
      */
    }
  }

}

void startWiFiAP()
{
  //WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, pass);
  Serial.println("AP started");
  Serial.println("ssid: " + (String)ssid);
  Serial.println("IP address: " + WiFi.softAPIP().toString());
}

void printIntArray(uint16_t *array, const int size_of_array) {
  for(int j = 0; j < size_of_array; j++) {
    Serial.print((String)array[j]);
    Serial.print(",");
  }
}
