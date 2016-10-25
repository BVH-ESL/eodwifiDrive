#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer.h>
#include <Hash.h>
extern "C" {
#include<user_interface.h>
}

/* Set these to your desired credentials. */
const char *ssid = "testWScommunication";
const char *password = "asdf1234";

ESP8266WebServer server(80);

ESP8266WiFiMulti WiFiMulti;

WebSocketsServer webSocket = WebSocketsServer(8081);

#define USE_SERIAL Serial
String data_in = "";

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {
  switch (type) {
    case WStype_DISCONNECTED:
      {
        USE_SERIAL.printf("[%u] Disconnected!\n", num);
      }
      break;
    case WStype_CONNECTED:
    {
        IPAddress ip = webSocket.remoteIP(num);
        USE_SERIAL.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

        // send message to client
        webSocket.sendTXT(num, "1234");
      }
      break;
    case WStype_TEXT:
      // client_time = millis();
      // USE_SERIAL.println(client_time);
       USE_SERIAL.printf("[%u] get Text: %s\n", num, payload);
      data_in += (char *)payload;
      // checkInput(data_in);
      data_in = "";
      break;
    case WStype_BIN:
      USE_SERIAL.printf("[%u] get binary lenght: %u\n", num, lenght);
      hexdump(payload, lenght);
      break;
  }
}

void setup() {
  USE_SERIAL.begin(115200);

  //Serial.setDebugOutput(true);
  USE_SERIAL.setDebugOutput(true);
  USE_SERIAL.println();
  USE_SERIAL.println();
  USE_SERIAL.println();
//  WiFi.disconnect(true);
//  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid, password);
//  IPAddress myIP = WiFi.softAPIP();
//  USE_SERIAL.print("AP IP address: ");
//  USE_SERIAL.println(myIP);
//  server.on("/", handleRoot);
//  server.begin();
//  USE_SERIAL.println("HTTP server started");
  for (uint8_t t = 4; t > 0; t--) {
    USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
    USE_SERIAL.flush();
    delay(1000);
  }


  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop(){
  webSocket.loop();
}
