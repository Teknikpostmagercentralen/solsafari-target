#include <Arduino.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>

#include <ArduinoJson.h>

#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

#include "SerialMP3Player.h"

#define MP3_TX 14
#define MP3_RX 12

#define POST_INTERVAL 2000 // Wait between

SerialMP3Player mp3(MP3_RX,MP3_TX);

// An IR detector/demodulator is connected to GPIO pin 14(D5 on a NodeMCU
// board).
// Note: GPIO 16 won't work on the ESP8266 as it does not have interrupts.
const uint16_t kRecvPin = 5;

IRrecv irrecv(kRecvPin);

decode_results results;

int myIndex = 0;

unsigned long timestamp = 0;

void setup() {
  Serial.begin(9600);
  irrecv.enableIRIn();  // Start the receiver
  while (!Serial)  // Wait for the serial connection to be establised.
    delay(50);
  
  WiFi.begin("SAFARIFI", "SolStorm");   //WiFi connection WIFI Admin: SolStorm
 
  while (WiFi.status() != WL_CONNECTED) {  //Wait for the WiFI connection completion
 
    delay(500);
    Serial.println("Waiting for connection");
 
  }

  Serial.println();
  Serial.print("IRrecvDemo is now running and waiting for IR message on Pin ");
  Serial.println(kRecvPin);

  //Start Mp3 lib
  mp3.begin(9600);        // start mp3-communication
  delay(500);             // wait for init
  mp3.sendCommand(CMD_SEL_DEV, 0, 2);   //select sd-card
  delay(500);             // wait for init
}

void loop() {
  if (irrecv.decode(&results)) {
    // print() & println() can't handle printing long longs. (uint64_t)
    uint32_t kommando = results.command;
    decode_type_t type = results.decode_type;
    if (kommando == 0x1D && type == RC5) {
      Serial.println(myIndex);
      myIndex++;
      mp3.play(1);
    }
    // How to print debug information
    //serialPrintUint64(results.command, HEX);
    irrecv.resume();  // Receive the next value
  }

  if (millis()-timestamp > 2000) {
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
 
      HTTPClient http;    //Declare object of class HTTPClient
  
      http.begin("http://192.168.0.198:5000/point/");      //Specify request destination
      http.addHeader("Content-Type", "application/json");  //Specify content-type header

      String json = "";
      StaticJsonDocument<16> doc;

      doc["hits"] = myIndex;
      myIndex = 0;

      serializeJson(doc, json);
  
      Serial.println(json);   //Print HTTP return code
  
      int httpCode = http.POST(json);   //Send the request
      String payload = http.getString();                  //Get the response payload

      Serial.println(httpCode);   //Print HTTP return code
      Serial.println(payload);    //Print request response payload
  
      http.end();  //Close connection
  
    } else {
  
      Serial.println("Error in WiFi connection");
  
    }
    timestamp = millis();
  }
}