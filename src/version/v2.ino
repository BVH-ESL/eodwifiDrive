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
const char *ssid = "espedotestdrivev3";
const char *password = "asdf1234";

ESP8266WebServer server(80);

ESP8266WiFiMulti WiFiMulti;

WebSocketsServer webSocket = WebSocketsServer(81);
WebSocketsServer webSocketESP = WebSocketsServer(82);

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
#define in2     2       //D4

// motor two
#define in3     13      //D7
#define in4     12      //D6
#define enB     14      //D5

//#include <Adafruit_NeoPixel.h>

#ifdef __AVR__
#include <avr/power.h>
#endif

long client_time = millis();
int state = 0;
int counter_micro_second = 0;
int is_change_dir = false;
int state_dir = 0;

void changeDir(int s_a,int s_b){
  if (s_a > 0 && s_b > 0) {
    if(state_dir != 1){
      is_change_dir = false;
      state_dir = 1;
    }
  } else if (s_a < 0 && s_b < 0) {
    if(state_dir != 2){
      is_change_dir = false;
      state_dir = 2;
    }
  }else if (s_a > 0 && s_b < 0) {
    if(state_dir != 3){
      is_change_dir = false;
      state_dir = 3;
    }
  } else if (s_a < 0 && s_b > 0) {
    if(state_dir != 4){
      is_change_dir = false;
      state_dir = 4;
    }
  }
}

void checkInput(String input) {
  Serial.println((input));
  String inString;
  // int s_a, s_b;
  for (int i = 1; i < input.length(); i++) {
    // USE_SERIAL.printf("%c", input[i]);
    if (input[0] == 'm') {
      if (input[i] == ':' || input[i] == '\n') {
        changeDir(speed_a_new, speed_b_new);
        inString = "";
        if(speed_a_new > 75){
          speed_a_new = 75;
        }
        if(speed_b_new > 75){
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
    }
  }
  // USE_SERIAL.printf("\n");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {
  switch (type) {
    case WStype_DISCONNECTED:
      {
        state = 0;
        // drive(0, 0);
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
      // USE_SERIAL.println(client_time);
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

void countCounter()
{
  counter_micro_second++;
}

void setup() {
  // set all the motor control pins to outputs
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

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  webSocketESP.begin();


  timer1_isr_init();
  timer1_attachInterrupt(reinterpret_cast<timercallback>(countCounter));
  timer1_enable(TIM_DIV16, TIM_EDGE, 1);    //TIM_DIV16 -> 5MHz = 5 ticks/us, TIM_DIV1 -> 80MHz = 80 ticks/us
  timer1_write(500);                       //call interrupt after ... tick
}

void loop() {
  if (state) {
    // Serial.println(WiFi.status());
    if(millis() - client_time == 10){
      client_time = millis();
      USE_SERIAL.println(speed_a_now);
      if(speed_a_new == 0 && speed_b_new == 0){ //m0,0: case
        if(speed_a_now > 0 && speed_b_now > 0){
          speed_a_now -= 5;
          speed_b_now -= 5;
        }else{
          digitalWrite(in1, LOW);
          digitalWrite(in2, LOW);
          digitalWrite(in3, LOW);
          digitalWrite(in4, LOW);
          is_change_dir = false;
        }
      }
      else if (speed_a_new > 0 && speed_b_new > 0) { //m+,+:
        if(!is_change_dir){
          if(speed_a_now > 0 && speed_b_now > 0){
            speed_a_now -= 5;
            speed_b_now -= 5;
          }else{
            digitalWrite(in1, LOW);
            digitalWrite(in2, HIGH);
            digitalWrite(in3, LOW);
            digitalWrite(in4, HIGH);
            is_change_dir = true;
          }
        }else{
          if(speed_a_now < speed_a_new && speed_b_now < speed_b_new){
            speed_a_now += 5;
            speed_b_now += 5;
          }else if(speed_a_now > speed_a_new && speed_b_now > speed_b_new){
            speed_a_now -= 5;
            speed_b_now -= 5;
          }
        }
      }
      else if (speed_a_new < 0 && speed_b_new < 0) { //m-,-:
        if(!is_change_dir){
          if(speed_a_now > 0 && speed_b_now > 0){
            speed_a_now -= 5;
            speed_b_now -= 5;
          }else{
            digitalWrite(in1, HIGH);
            digitalWrite(in2, LOW);
            digitalWrite(in3, HIGH);
            digitalWrite(in4, LOW);
            is_change_dir = true;
          }
        }else{
          if(speed_a_now < -speed_a_new && speed_b_now < -speed_b_new){
            speed_a_now += 5;
            speed_b_now += 5;
          }else if(speed_a_now > -speed_a_new && speed_b_now > -speed_b_new){
            speed_a_now -= 5;
            speed_b_now -= 5;
          }
        }
      }
      else if (speed_a_new > 0 && speed_b_new < 0) { //m+,-:
        if(!is_change_dir){
          if(speed_a_now > 0 && speed_b_now > 0){
            speed_a_now -= 5;
            speed_b_now -= 5;
          }else{
            digitalWrite(in1, LOW);
            digitalWrite(in2, HIGH);
            digitalWrite(in3, HIGH);
            digitalWrite(in4, LOW);
            is_change_dir = true;
          }
        }else{
          if(speed_a_now < speed_a_new && speed_b_now < -speed_b_new){
            speed_a_now += 5;
            speed_b_now += 5;
          }else if(speed_a_now > speed_a_new && speed_b_now > -speed_b_new){
            speed_a_now -= 5;
            speed_b_now -= 5;
          }
        }
      }
      else if (speed_a_new < 0 && speed_b_new > 0) { //m-,+:
        if(!is_change_dir){
          if(speed_a_now > 0 && speed_b_now > 0){
            speed_a_now -= 5;
            speed_b_now -= 5;
          }else{
            digitalWrite(in1, HIGH);
            digitalWrite(in2, LOW);
            digitalWrite(in3, LOW);
            digitalWrite(in4, HIGH);
            is_change_dir = true;
          }
        }else{
          if(speed_a_now < -speed_a_new && speed_b_now < speed_b_new){
            speed_a_now += 5;
            speed_b_now += 5;
          }else if(speed_a_now > -speed_a_new && speed_b_now > speed_b_new){
            speed_a_now -= 5;
            speed_b_now -= 5;
          }
        }
      }
      analogWrite(enA, map(speed_a_now, 0, 100, 0, 1000));
      analogWrite(enB, map(speed_b_now, 0, 100, 0, 1000));
    }
  }
  webSocket.loop();
}
