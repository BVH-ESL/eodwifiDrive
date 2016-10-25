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
const char *ssid = "espedotestdrive";
const char *password = "asdf1234";

ESP8266WebServer server(80);

ESP8266WiFiMulti WiFiMulti;

WebSocketsServer webSocket = WebSocketsServer(81);

#define USE_SERIAL Serial
String data_in = "";

int speed_a;
int speed_b;

// connect motor controller pins to Arduino digital pins
// motor one
#define enA     5       //D1
#define in1     4       //D2
#define in2     2       //D4

// motor two
#define in3     13      //D7
#define in4     12      //D6
#define enB     14      //D5

//#include <Adafruit_NeoPixel.h>

#ifdef __AVR__
#include <avr/power.h>
#endif

#define LED_PIN  15      //D8
//Adafruit_NeoPixel pixels = Adafruit_NeoPixel(1, LED_PIN, NEO_GRB + NEO_KHZ800);
int state = 0;
int toggle = 0;

long client_time = millis();

void drive(int speed_a, int speed_b) {
  if (speed_a > 0 && speed_b > 0) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);

    digitalWrite(in3, HIGH);
    digitalWrite(in4, LOW);
  } else if (speed_a < 0 && speed_b < 0) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);

    digitalWrite(in3, LOW);
    digitalWrite(in4, HIGH);
    speed_a *= -1;
    speed_b *= -1;
  } else if (speed_a > 0 && speed_b < 0) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);

    digitalWrite(in3, LOW);
    digitalWrite(in4, HIGH);
    speed_b *= -1;
  } else if (speed_a < 0 && speed_b > 0) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);

    digitalWrite(in3, HIGH);
    digitalWrite(in4, LOW);
    speed_a *= -1;
  }

  if (speed_a == 0) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
  }

  if (speed_b == 0) {
    digitalWrite(in3, LOW);
    digitalWrite(in4, LOW);
  }

  float s_a = speed_a * 1023 / 100.0;
  float s_b = speed_b * 1023 / 100.0;
  analogWrite(enA, s_a);
  analogWrite(enB, s_b);
}

void checkInput(String input) {
  //  Serial.println(sizeof(input));
  String inString;
  for (int i = 1; i < input.length(); i++) {
    USE_SERIAL.printf("%c", input[i]);
    if (input[0] == 'm') {
      if (input[i] == ':' || input[i] == '\n') {
        drive(speed_a, speed_b);
        inString = "";
      } else {
        if (input[i] != ',' && input[i] != '(' && input[i] != ')') {
          inString += input[i];
        } else {
          if (input[i] == ',') {
            speed_a = inString.toInt();
            inString = "";
          } else if (input[i] == ')') {
            speed_b = inString.toInt();
            inString = "";
          }
        }
      }
    }
  }
  USE_SERIAL.printf("\n");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {

  switch (type) {
    case WStype_DISCONNECTED:
      {
        state = 0;
        drive(0, 0);
        USE_SERIAL.printf("[%u] Disconnected!\n", num);
      }
      break;
    case WStype_CONNECTED:
      {
        state = 1;
        IPAddress ip = webSocket.remoteIP(num);
        USE_SERIAL.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

        // send message to client
        webSocket.sendTXT(num, "1234");
      }
      break;
    case WStype_TEXT:
      client_time = millis();
      USE_SERIAL.println(client_time);
      data_in += (char *)payload;
      checkInput(data_in);
      data_in = "";
      break;
    case WStype_BIN:
      USE_SERIAL.printf("[%u] get binary lenght: %u\n", num, lenght);
      hexdump(payload, lenght);
      break;
  }

}

void setup() {
//  pixels.begin();
  // set all the motor control pins to outputs
  pinMode(enA, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);

  pinMode(enB, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  analogWriteFreq(10000);
  // USE_SERIAL.begin(921600);
  USE_SERIAL.begin(115200);

  //Serial.setDebugOutput(true);
  USE_SERIAL.setDebugOutput(true);

  USE_SERIAL.println();
  USE_SERIAL.println();
  USE_SERIAL.println();

  for (uint8_t t = 4; t > 0; t--) {
    USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
    USE_SERIAL.flush();
    delay(1000);
  }

  WiFi.softAP(ssid, password);
  //  WiFiMulti.addAP("ESL_Lab1", "wifi@esl");
  //
  //  while (WiFiMulti.run() != WL_CONNECTED) {
  //    delay(100);
  //  }

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  unsigned char number_client = wifi_softap_get_station_num(); 
  if (number_client == 0) {
    USE_SERIAL.println(number_client);
    drive(0,0);
  }
  if (state) {
    Serial.println(WiFi.status());
//    pixels.setPixelColor(0, pixels.Color(0, 50, 0));

//    pixels.show();
  } else {
    if (toggle) {
//      pixels.setPixelColor(0, pixels.Color(0, 50, 0));
//
//      pixels.show();
      delay(500);
      toggle = 0;
    } else {
//      pixels.setPixelColor(0, pixels.Color(0, 0, 0));
//
//      pixels.show();
      delay(500);
      toggle = 1;
    }
  }
  //  if(millis()-client_time > 500){
  //    drive(0,0);
  //  }
  webSocket.loop();
}
