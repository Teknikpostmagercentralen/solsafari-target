#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRutils.h>
#include <DFPlayerMini_Fast.h>
#include <SoftwareSerial.h>

#define NO_SERIALMP3_DELAY //disable delays in mp3 SerialMP3Player library

// MP3 Player
#define MP3_TX 12
#define MP3_RX 13

#define POST_INTERVAL 2000  // Wait between

#define kRecvPin2 14

#define RED_PIN 0
#define GREEN_PIN 4
#define BLUE_PIN 5

#define ORANGE_CODE 0x1F
#define BLUE_CODE 0x1D

#define BLINK_DELAY 500

//SerialMP3Player mp3(MP3_RX, MP3_TX);
DFPlayerMini_Fast mp3;
SoftwareSerial mp3Serial(MP3_RX, MP3_TX); // RX, TX



// An IR detector/demodulator is connected to GPIO pin 14(D5 on a NodeMCU
// board).
// Note: GPIO 16 won't work on the ESP8266 as it does not have interrupts.

IRrecv irrecv2(kRecvPin2);

decode_results results;
decode_results results2;

int orangeHits = 0;
int blueHits = 0;

unsigned long timestamp = 0;

long redTimestamp = 0;
long greenTimestamp = 0;

void setup() {
    Serial.begin(9600);

    irrecv2.enableIRIn();  // Start the receiver
    pinMode(kRecvPin2, INPUT);
    pinMode(RED_PIN, OUTPUT);
    pinMode(BLUE_PIN, OUTPUT);
    pinMode(GREEN_PIN, OUTPUT);

    while (!Serial)  // Wait for the serial connection to be establised.
        delay(50);

    WiFi.begin("SAFARIFI", "SolStorm");  // WiFi connection WIFI Admin: SolStorm

    checkWifiConnectionBlocking();

    Serial.println();
    Serial.print(
        "IRrecv Target is now running and waiting for IR message on Pin ");

    Serial.print(kRecvPin2);

    // Start Mp3 lib
    //mp3.begin(9600);                     // start mp3-communication
    mp3Serial.begin(9600);
    mp3.begin(mp3Serial, true);

    delay(500);                          // wait for init
    mp3.volume(30); //set volume max
    //mp3.sendCommand(CMD_SEL_DEV, 0, 2);  // select sd-card
    delay(500);                          // wait for init
}

void checkWifiConnectionBlocking() {
    while (WiFi.status() !=
           WL_CONNECTED) {  // Wait for the WiFI connection completion
        digitalWrite(BLUE_PIN, !digitalRead(BLUE_PIN));
        delay(500);
        Serial.println("Waiting for connection");
    }

    digitalWrite(
        BLUE_PIN,
        LOW);  // Set blue pin low so its disabled no matter where it ended
}

void bang() {
    mp3.play(1);
    blinkGreen();
}

void blinkRed() {}

void blinkGreen() {
    digitalWrite(GREEN_PIN, HIGH);
    greenTimestamp = millis();
}

void updateLED() {
    long current = millis();
    if ((current - greenTimestamp) > BLINK_DELAY) {
        digitalWrite(GREEN_PIN, LOW);
    }
}

void loop() {
    if (irrecv2.decode(&results2)) {
        uint32_t kommando2 = results2.command;
        decode_type_t type2 = results2.decode_type;
        if (type2 == RC5) {
            if (kommando2 == BLUE_CODE) {
                bang();
                blueHits++;
            }
            if (kommando2 == ORANGE_CODE) {
                bang();
                orangeHits++;
            }
        }
        irrecv2.resume();  // Receive the next value
    }

    if (millis() - timestamp > 10000) {
        if (WiFi.status() == WL_CONNECTED) {  // Check WiFi connection status

            HTTPClient http;  // Declare object of class HTTPClient
            WiFiClient client;

            http.begin(client,
                       "http://192.168.0.198:5000/point/");  // Specify request
                                                             // destination
            http.addHeader("Content-Type",
                           "application/json");  // Specify content-type header

            String json = "";
            StaticJsonDocument<200> doc;

            doc["orangeHits"] = orangeHits;
            doc["blueHits"] = blueHits;

            serializeJson(doc, json);

            // Serial.println(json);   //Print HTTP return code

            int httpCode = http.POST(json);     // Send the request
            String payload = http.getString();  // Get the response payload

            // Serial.println(httpCode);   //Print HTTP return code
            // Serial.println(payload);    //Print request response payload
            if(httpCode != 200) {
              toggleErrorLED();
            } else {
              digitalWrite(RED_PIN, LOW); // Turn off error LED
              orangeHits = 0;
              blueHits = 0;
            }

            http.end();  // Close connection
        } else {
            Serial.println("Error in WiFi connection");
            checkWifiConnectionBlocking();
        }
        timestamp = millis();
    }
    updateLED();
}

void toggleErrorLED() {
  digitalWrite(RED_PIN, !digitalRead(RED_PIN));
}