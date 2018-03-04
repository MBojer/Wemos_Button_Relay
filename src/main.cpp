/*
Pins

  D0 = 16
  D1 = 5
  D2 = 4
  D3 = Not used
  D4 =  Not used (BUILTIN_LED)
  D5 = 14
  D6 = 12
  D7 = 13
  D8 = 15



WiFi.status() =
  0 : WL_IDLE_STATUS when Wi-Fi is in process of changing between statuses
  1 : WL_NO_SSID_AVAILin case configured SSID cannot be reached
  3 : WL_CONNECTED after successful connection is established
  4 : WL_CONNECT_FAILED if password is incorrect
  6 : WL_DISCONNECTED if module is not configured in station mode
*/

#include <Arduino.h>

// ---------------------------------------- WiFi ----------------------------------------
#include <ESP8266WiFi.h>

const char* WiFi_SSID = "NoInternetHereEither";
const char* WiFi_Password = "NoPassword1!";
String WiFi_Hostname = "BtnRel";

unsigned long WiFi_Timeout = 5000; // ms before marking connection as failed
unsigned long WiFi_Reconnect_At;
unsigned long WiFi_Reconnect_Delay = 30000; // ms before marking connection as failed


// ---------------------------------------- Button's ----------------------------------------
const int Button_Pin[] {D5, D6, D7, D2};
const int Button_Number_Of = 4;

unsigned long Button_Ignore_Input_Untill[4];
unsigned int Button_Ignore_Input_For = 750; // Time in ms before a butten can triggered again


// ---------------------------------------- Relay's ----------------------------------------
const int Relay_Number_Of = 1;
const int Relay_Pin[Relay_Number_Of] {D1};




// ---------------------------------------- Server ----------------------------------------
WiFiServer server(80);


// ---------------------------------------- Web_Server() ----------------------------------------
void Web_Server() {

  if (server.status() == false) {
    if (WiFi.status() == 3) {
      // Start the server
      server.begin();
      Serial.println("Server started");

      // Print the IP address
      Serial.print("Use this URL : ");
      Serial.print("http://");
      Serial.print(WiFi.localIP());
      Serial.println("/");
    }
  }

  else if (server.status() == true) {
    if (WiFi.status() != 3) {
        server.stop();
    }
  }
} // Web_Server()



// ---------------------------------------- WiFi_Reset() ----------------------------------------
void WiFi_Reset() {
  Serial.println("Erasing WiFi config");
  ESP.eraseConfig();
  delay(1500);

  Serial.println("Configuring WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.hostname(WiFi_Hostname);
}


// ---------------------------------------- WiFi_Connect() ----------------------------------------
bool WiFi_Connect(const char* SSID, const char* Password) {

  if (WiFi.status() == 3) {
    Serial.print("Disconnecting from ");
    Serial.println(WiFi.SSID());
    WiFi_Reset();
  }

  Serial.print("Connecting to ");
  Serial.print(SSID);
  Serial.print(" ");

  unsigned long WiFi_Connect_Start = millis() + WiFi_Timeout;

  WiFi.begin(SSID, Password);

  while (WiFi.status() != 3) {
    if (WiFi_Connect_Start < millis()) {
      Serial.println(" connection timeout.");
      return false;
    }
    Serial.print(".");
    delay (500);
  }

  Serial.println(" connected.");
  return true;

} // WiFi_Connect

// ---------------------------------------- WiFi_Check() ----------------------------------------

void WiFi_Check() {

  if (WiFi_Reconnect_At < millis()) {
    if (digitalRead(Relay_Pin[0]) == HIGH) {
      if (WiFi.status() != 3) {
        WiFi_Connect(WiFi_SSID, WiFi_Password);
        Web_Server();
        WiFi_Reconnect_At = millis() + WiFi_Reconnect_Delay;
      }
    }
  }

}





// ---------------------------------------- Button_Check() ----------------------------------------
byte Button_Check() {
  for (byte i = 0; i < Button_Number_Of; i++) {
    if (Button_Ignore_Input_Untill[i] < millis()) {
      if (digitalRead(Button_Pin[i]) == LOW) {
        Serial.println("Button " + String(i) + " pressed");
        Button_Ignore_Input_Untill[i] = millis() + Button_Ignore_Input_For;
        return i;
      }
    }
  }
  return 255;
}





// ---------------------------------------- setup() ----------------------------------------
void setup() {
  Serial.begin(112500);
  Serial.println();
  Serial.println("Booting");

  for (byte i = 0; i < Button_Number_Of; i++) {
    pinMode(Button_Pin[i], INPUT_PULLUP);
  }

  for (byte i = 0; i < Relay_Number_Of; i++) {
    pinMode(Relay_Pin[i], OUTPUT);
    digitalWrite(Relay_Pin[i], 0);
  }

  WiFi_Check();

  Serial.println("Boot done");
}


// ---------------------------------------- loop() ----------------------------------------
void loop() {

  WiFi_Check();

  byte Button_Pressed = Button_Check();

  if (Button_Pressed == 255) {
    // 255 = No button pressed
  }
  else if (Button_Pressed == 0) { // Main Light
    
    Serial.print("Main light set to 50");
  }
  else if (Button_Pressed == 1) { // ADD ME

  }
  else if (Button_Pressed == 2) { // Router OFF
    digitalWrite(Relay_Pin[0], !digitalRead(Relay_Pin[0]));
    Serial.print("Relay changed state to ");
    Serial.println(digitalRead(Relay_Pin[0]));
  }
  else if (Button_Pressed == 3) { // All OFF

  }

  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  // Wait until the client sends some data
  while(!client.available()){
    delay(1);
  }

  // Read the first line of the request
  String request = client.readStringUntil('\r');
  Serial.println(request);
  client.flush();

  // Match the request

  // remove "/light" and set light based on % number in get requist

  if (request.indexOf("GET /Relay_") != -1) {

    request.replace("GET /Relay_", "");
    request.replace(" HTTP/1.1", "");

    byte Selected_Relay = request.substring(0, request.indexOf("-")).toInt();

    byte Selected_State = request.substring(request.indexOf("-") + 1, request.length()).toInt();

    if (Selected_Relay == 99) {
      for (byte i = 0; i < Relay_Number_Of; i++) {
        digitalWrite(Relay_Pin[i], 0);
      }
      Serial.println("All OFF");
    }

    else {
      Selected_Relay = Selected_Relay - 1; // Done to compensate for the array number beaingg -1 in relation to relay number

      digitalWrite(Relay_Pin[Selected_Relay], Selected_Relay);

      Serial.print("Relay " + String(Selected_Relay + 1) + ": ");
      if (digitalRead(Selected_Relay) == 0) Serial.println("OFF");
      else Serial.println("ON");

    }


  }


  else Serial.println(request);

  // Return the response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); //  do not forget this one
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");

  client.println("Wemos Relay v0.1");
  client.println("<br><br>");

  for (byte i = 0; i < Relay_Number_Of; i++) {
    client.print("Relay " + String(i + 1) + ": ");
    if (digitalRead(Relay_Pin[i]) == LOW) client.print("OFF");
    else client.print("ON");
    client.println("<br>");
  }


  client.println("</html>");

  delay(1);

}
