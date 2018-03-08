/*
Pins

  D0 = 16
  D1 = 5 - Relay 1
  D2 = 4 - Button 4
  D3 = Not used
  D4 =  Not used (BUILTIN_LED)
  D5 = 14 - Button 1
  D6 = 12 - Button 2
  D7 = 13 - Button 3
  D8 = 15



WiFi.status() =
  0 : WL_IDLE_STATUS when Wi-Fi is in process of changing between statuses
  1 : WL_NO_SSID_AVAILin case configured SSID cannot be reached
  3 : WL_CONNECTED after successful connection is established
  4 : WL_CONNECT_FAILED if password is incorrect
  6 : WL_DISCONNECTED if module is not configured in station mode
*/

#include <Arduino.h>

// ---------------------------------------- MQTT Broker ----------------------------------------
#include "uMQTTBroker.h"

unsigned int MQTT_Broker_Port = 1883;       // the standard MQTT broker port
unsigned int MQTT_Broker_Max_Subscriptions = 30;
unsigned int MQTT_Broker_Max_Retained_Topics = 30;


// ---------------------------------------- MQTT Client ----------------------------------------
unsigned long MQTT_KeepAlive_Delay = 60000; // 60000
unsigned long MQTT_KeepAlive_At = MQTT_KeepAlive_Delay;


// ---------------------------------------- WiFi ----------------------------------------
#include <ESP8266WiFi.h>

WiFiClient client;

const char* WiFi_SSID = "NoInternetHere";
const char* WiFi_Password = "NoPassword1!";
String WiFi_Hostname = "BackupAP";

unsigned long WiFi_Timeout = 4500; // ms before marking connection as failed
unsigned long WiFi_Reconnect_At = 0;
unsigned long WiFi_Reconnect_Delay = 20000; // ms before marking connection as failed


// ---------------------------------------- WiFi_Backup_AP ----------------------------------------
IPAddress AP_IP(192,168,0,2);
IPAddress AP_Gateway(192,168,0,2);
IPAddress AP_Subnet(255,255,255,0);

byte Connected_Clients_Last = 0;


// ---------------------------------------- Button's ----------------------------------------
const int Button_Pin[] {D5, D6, D7, D2};
const int Button_Number_Of = 4;

unsigned long Button_Ignore_Input_Untill[4];
unsigned int Button_Ignore_Input_For = 750; // Time in ms before a butten can triggered again


// ---------------------------------------- Relay's ----------------------------------------
const int Relay_Number_Of = 1;
const int Relay_Pin[Relay_Number_Of] {D1};
bool Relay_On_State = LOW;


// ---------------------------------------- Web Server ----------------------------------------
WiFiServer server(80);


// ---------------------------------------- HTTP_GET() ----------------------------------------
String HTTP_GET(String Server, int Port, String URL) {

  // Attempt to make a connection to the remote server
  if ( !client.connect(Server, Port) ) {
    return "Connection failed";
  }

  // This will send the request to the server
  client.print(String("GET /") + URL + " HTTP/1.1\r\n" +
               "Host: " + Server + "\r\n" +
               "Connection: close\r\n\r\n");
  delay(50);

  // Read all the lines of the reply from server and print them to Serial
  String Reply;
  while(client.available()){
    Reply += client.readStringUntil('\r');
  }

  return Reply;
}



// ---------------------------------------- WiFi_Reset() ----------------------------------------
void WiFi_Reset(bool Backup_AP_Active) {

  if (Backup_AP_Active == true) {
    Serial.println("Configuring WiFi as Backup AP");
    WiFi.mode(WIFI_AP);
    WiFi.enableAP(true);
  }

  else {
    Serial.println("Configuring WiFi as client");
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.enableAP(false);
  }

  WiFi.hostname(WiFi_Hostname);

} // WiFi_Reset()


// ---------------------------------------- WiFi_Connect() ----------------------------------------
void WiFi_Client() {

  WiFi_Reset(false);

  Serial.print("Connecting to: ");
  Serial.print(WiFi_SSID);
  Serial.print(" ");

  Serial.print("Starting WiFi Client ... ");
  Serial.println(WiFi.begin(WiFi_SSID, WiFi_Password) ? "OK" : "Failed!");

} // WiFi_Connect


// ---------------------------------------- WiFi_Backup_AP() ----------------------------------------
void WiFi_Backup_AP() {

  WiFi_Reset(true);

  Serial.print("Setting soft-AP configuration ... ");
  Serial.println(WiFi.softAPConfig(AP_IP, AP_Gateway, AP_Subnet) ? "OK" : "Failed!");

  Serial.print("Setting soft-AP ... ");
  Serial.println(WiFi.softAP(WiFi_SSID, WiFi_Password) ? "OK" : "Failed!");

} // AP()


// ---------------------------------------- WiFi_Reset() ----------------------------------------
void AP_Client_Check() {
  if (Connected_Clients_Last != WiFi.softAPgetStationNum()) {
    Serial.print("Number of clients conected to AP: ");
    Serial.println(WiFi.softAPgetStationNum());
    Connected_Clients_Last = WiFi.softAPgetStationNum();
  }
}

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

void Web_Server_Check() {

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
          digitalWrite(Relay_Pin[i], !Relay_On_State);
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

      if (Relay_On_State == true) {
        if (digitalRead(Relay_Pin[i]) == LOW) client.print("OFF");
        else client.print("ON");
      }

      else {
        if (digitalRead(Relay_Pin[i]) == HIGH) client.print("OFF");
        else client.print("ON");
      }


      client.println("<br>");
    }


    client.println("</html>");

}


// ---------------------------------------- Button_Check() ----------------------------------------
byte Button_Pressed_Check() {
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


// ---------------------------------------- Button_Check() ----------------------------------------
void Button_Check() {
  byte Button_Pressed = Button_Pressed_Check();

  if (Button_Pressed == 255) {
    // 255 = No button pressed
    return;
  }
  else if (Button_Pressed == 0) { // Main Light
    Serial.println("Button 1 - CHANGE ME");
    // Serial.println(HTTP_GET("192.168.0.111", 80, "Dimmer_1-10"));
    // Serial.print("Main light set to 10");
  }
  else if (Button_Pressed == 1) { // ADD ME
    Serial.println("Button 2 - CHANGE ME");

  }
  else if (Button_Pressed == 2) { // Router OFF
    digitalWrite(Relay_Pin[0], !digitalRead(Relay_Pin[0]));
    Serial.print("Relay changed state to: ");
    if (digitalRead(Relay_Pin[0]) == Relay_On_State) {
      Serial.println("ON");

      Serial.println("Switching from Backup AP to WiFi client");
      WiFi_Client();
    }
    else {
      Serial.println("OFF");

      Serial.println("Switching WiFi client to Backup AP");
      WiFi_Backup_AP();
    }

  }
  else if (Button_Pressed == 3) { // All OFF
    Serial.println("Button 4 - CHANGE ME");
  }
} // Button_Press_Check


void MQTT_Local_Publish(String Topic, String Message) {
  // MQTT_Local_Publish("Boat/System"; "Booting");

  MQTT_local_publish((unsigned char *)Topic.c_str(), (unsigned char *)Message.c_str(), Message.length(), 0, 0);

}

void MQTT_KeepAlive() {
  if (MQTT_KeepAlive_At < millis()) {
    MQTT_Local_Publish("Boat/KeepAlive", WiFi_Hostname);
    MQTT_KeepAlive_At = millis() + MQTT_KeepAlive_Delay;
  }
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
    digitalWrite(Relay_Pin[i], !Relay_On_State);
  }

  WiFi_Backup_AP();

  Serial.print("Starting MQTT Broker ... ");
  Serial.println(MQTT_server_start(MQTT_Broker_Port, MQTT_Broker_Max_Subscriptions, MQTT_Broker_Max_Retained_Topics) ? "OK" : "Failed!");

  MQTT_Local_Publish("Boat/System", "Booting");
  Serial.println("Boot done");
}


// ---------------------------------------- loop() ----------------------------------------
void loop() {

  Button_Check();

  MQTT_KeepAlive();

  AP_Client_Check();

} // loop()
