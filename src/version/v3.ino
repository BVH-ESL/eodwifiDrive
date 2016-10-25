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
const char *ssid = "eodLoggerControll";
const char *password = "asdf1234";

ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(8081);

#define USE_SERIAL Serial
String data_in = "";

int speed_a_now = 0;
int speed_b_now = 0;
int speed_a_new = 0;
int speed_b_new = 0;

// connect motor controller pins to Arduino digital pins
// motor one
#define enA     5       //D1
#define in1     4       //D2
#define in2     15       //D4

// motor two
#define in3     13      //D7
#define in4     12      //D6
#define enB     14      //D5

//#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

long client_time = millis();
long count_time = millis();
int state = 0;
int counter_micro_second = 0;
int is_change_dir = false;
int state_dir = 0;
int wifiDev;

void changeDir(int s_a, int s_b) {
  if (s_a > 0 && s_b > 0) {
    if (state_dir != 1) {
      is_change_dir = false;
      state_dir = 1;
    }
  } else if (s_a < 0 && s_b < 0) {
    if (state_dir != 2) {
      is_change_dir = false;
      state_dir = 2;
    }
  } else if (s_a > 0 && s_b < 0) {
    if (state_dir != 3) {
      is_change_dir = false;
      state_dir = 3;
    }
  } else if (s_a < 0 && s_b > 0) {
    if (state_dir != 4) {
      is_change_dir = false;
      state_dir = 4;
    }
  }
}

void checkInput(String input) {
//  Serial.println((input));
  String inString;
  // int s_a, s_b;
  for (int i = 1; i < input.length(); i++) {
    // USE_SERIAL.printf("%c", input[i]);
    if (input[0] == 'm') {
      if (input[i] == ':' || input[i] == '\n') {
        changeDir(speed_a_new, speed_b_new);
        inString = "";
        if (speed_a_new > 75) {
          speed_a_new = 75;
        }
        if (speed_b_new > 75) {
          speed_b_new = 75;
        }
      } else {
        if (input[i] != ',' && input[i] != '(' && input[i] != ')') {
          inString += input[i];
        } else {
          if (input[i] == ',') {
            speed_a_new = inString.toInt();
            inString = "";
          } else if (input[i] == ')') {
            speed_b_new = inString.toInt();
            inString = "";
          }
        }
      }
    } else {
      if (input[i] == '0') {
        webSocket.sendTXT(wifiDev, "stop");
      } else if (input[i] == '1') {
        webSocket.sendTXT(wifiDev, "start");
      }
    }
  }
  // USE_SERIAL.printf("\n");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {
  switch (type) {
    case WStype_DISCONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        if (ip[3] != 2) {
          state = 0;
          analogWrite(enA, 0);
          analogWrite(enB, 0);
          digitalWrite(in1, LOW);
          digitalWrite(in2, LOW);
          digitalWrite(in3, LOW);
          digitalWrite(in4, LOW);
        }
        // drive(0, 0);
        USE_SERIAL.printf("[%u] Disconnected!\n", num);
      }
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        USE_SERIAL.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        if (ip[3] == 2) {
          wifiDev = num;
        } else {
          count_time = millis();
          client_time = millis();
          state = 1;
        }
        // send message to client
        webSocket.sendTXT(num, "1234");
      }
      break;
    case WStype_TEXT:
//      client_time = millis();
      data_in += (char *)payload;
//      USE_SERIAL.println(data_in);
      if (data_in == "esp") {
        wifiDev = num;
//        USE_SERIAL.println(wifiDev);
      } else {
        count_time = millis();
        client_time = millis();
//        Serial.println(client_time);
        checkInput(data_in);
      }
      data_in = "";
      break;
    case WStype_BIN:
      // USE_SERIAL.printf("[%u] get binary lenght: %u\n", num, lenght);
      hexdump(payload, lenght);
      break;
  }
}

void setup() {
  WiFi.disconnect();
  WiFi.softAP(ssid, password);

  // set all the motor control pins to outputs
  analogWrite(enA, 0);
  analogWrite(enB, 0);
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
  pinMode(enA, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);

  pinMode(enB, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  analogWriteRange(1000);
  analogWriteFreq(10000);

  // USE_SERIAL.begin(921600);
  USE_SERIAL.begin(115200);

  //Serial.setDebugOutput(true);
  // USE_SERIAL.setDebugOutput(true);
  USE_SERIAL.println();
  USE_SERIAL.println();
  USE_SERIAL.println();

  for (uint8_t t = 4; t > 0; t--) {
    USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
    USE_SERIAL.flush();
    delay(1000);
  }

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  if (state) {
//     Serial.println(millis() - count_time);
    if (millis() - count_time == 10) {
       count_time = millis();
//      Serial.println(millis() - client_time);
      if (speed_a_new == 0 && speed_b_new == 0) { //m0,0: case
        if (speed_a_now > 0 && speed_b_now > 0) {
          speed_a_now -= 5;
          speed_b_now -= 5;
        } else {
          digitalWrite(in1, LOW);
          digitalWrite(in2, LOW);
          digitalWrite(in3, LOW);
          digitalWrite(in4, LOW);
          is_change_dir = false;
        }
      }
      else if (speed_a_new > 0 && speed_b_new > 0) { //m+,+:
        if (!is_change_dir) {
          if (speed_a_now > 0 && speed_b_now > 0) {
            speed_a_now -= 5;
            speed_b_now -= 5;
          } else {
            digitalWrite(in1, LOW);
            digitalWrite(in2, HIGH);
            digitalWrite(in3, LOW);
            digitalWrite(in4, HIGH);
            is_change_dir = true;
          }
        } else {
          if (speed_a_now < speed_a_new && speed_b_now < speed_b_new) {
            speed_a_now += 5;
            speed_b_now += 5;
          } else if (speed_a_now > speed_a_new && speed_b_now > speed_b_new) {
            speed_a_now -= 5;
            speed_b_now -= 5;
          }
        }
      }
      else if (speed_a_new < 0 && speed_b_new < 0) { //m-,-:
        if (!is_change_dir) {
          if (speed_a_now > 0 && speed_b_now > 0) {
            speed_a_now -= 5;
            speed_b_now -= 5;
          } else {
            digitalWrite(in1, HIGH);
            digitalWrite(in2, LOW);
            digitalWrite(in3, HIGH);
            digitalWrite(in4, LOW);
            is_change_dir = true;
          }
        } else {
          if (speed_a_now < -speed_a_new && speed_b_now < -speed_b_new) {
            speed_a_now += 5;
            speed_b_now += 5;
          } else if (speed_a_now > -speed_a_new && speed_b_now > -speed_b_new) {
            speed_a_now -= 5;
            speed_b_now -= 5;
          }
        }
      }
      else if (speed_a_new > 0 && speed_b_new < 0) { //m+,-:
        if (!is_change_dir) {
          if (speed_a_now > 0 && speed_b_now > 0) {
            speed_a_now -= 5;
            speed_b_now -= 5;
          } else {
            digitalWrite(in1, LOW);
            digitalWrite(in2, HIGH);
            digitalWrite(in3, HIGH);
            digitalWrite(in4, LOW);
            is_change_dir = true;
          }
        } else {
          if (speed_a_now < speed_a_new && speed_b_now < -speed_b_new) {
            speed_a_now += 5;
            speed_b_now += 5;
          } else if (speed_a_now > speed_a_new && speed_b_now > -speed_b_new) {
            speed_a_now -= 5;
            speed_b_now -= 5;
          }
        }
      }
      else if (speed_a_new < 0 && speed_b_new > 0) { //m-,+:
        if (!is_change_dir) {
          if (speed_a_now > 0 && speed_b_now > 0) {
            speed_a_now -= 5;
            speed_b_now -= 5;
          } else {
            digitalWrite(in1, HIGH);
            digitalWrite(in2, LOW);
            digitalWrite(in3, LOW);
            digitalWrite(in4, HIGH);
            is_change_dir = true;
          }
        } else {
          if (speed_a_now < -speed_a_new && speed_b_now < speed_b_new) {
            speed_a_now += 5;
            speed_b_now += 5;
          } else if (speed_a_now > -speed_a_new && speed_b_now > speed_b_new) {
            speed_a_now -= 5;
            speed_b_now -= 5;
          }
        }
      }
      analogWrite(enA, map(speed_a_now, 0, 100, 0, 1000));
      analogWrite(enB, map(speed_b_now, 0, 100, 0, 1000));
//      Serial.println("client in");
    }
    if(millis() - client_time > 2000){
//      Serial.println("client out");
      analogWrite(enA, 0);
      analogWrite(enB, 0);
      digitalWrite(in1, LOW);
      digitalWrite(in2, LOW);
      digitalWrite(in3, LOW);
      digitalWrite(in4, LOW);
//      state = 0;
//      break;
    }
  }
  webSocket.loop();
}
