/**
 * This script is based on these sources:
 *
 * To count pulses using interruptions:  https://github.com/mysensors/MySensors/blob/master/examples/EnergyMeterPulseSensor/EnergyMeterPulseSensor.ino
 * To connect to Wifi and publish MQTT messages:  https://github.com/knolleary/pubsubclient/blob/master/examples/mqtt_esp8266/mqtt_esp8266.ino
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * DESCRIPTION
 * Use this sensor to measure kWh and Watt of your house meter.
 * You need to set the correct pulsefactor of your meter (pulses per kWh).
 * Reports every SEND_FREQUENCY miliseconds: pulses counting, kWh and Watt to different MQTT topics.
 *
 */

#include <SPI.h>
#include <WiFi101.h>

// Update these with values suitable for your network.

/************************* WiFi Access Point *********************************/

const char* ssid = "GL 73";
const char* password = "gbglor73";

/**************************** MQTT Broker ************************************/

char server[] = "192.168.100.206"; // example: "192.168.0.8"
IPAddress ip(192,168,100,208);
WiFiClient client;

#define DIGITAL_INPUT_SENSOR 6 // The digital input you attached S0+ D6 in Wemos D1 mini
#define PULSE_FACTOR 2000       // Nummber of pulses per kWh of your meeter
#define MAX_WATT 1000          // Max watt value to report. This filters outliers.

unsigned long SEND_FREQUENCY = 20000; // Minimum time between send (in milliseconds)
double ppwh = ((double)PULSE_FACTOR)/1000; // Pulses per watt hour
volatile unsigned long pulseCount = 0;
volatile unsigned long lastBlink = 0;
volatile unsigned long dataEnergi = 0;
unsigned long oldWatt = 0;
double oldKwh;
unsigned long lastSend;
double kwh;
bool printWebData = true;
unsigned long byteCount = 0;



long lastMsg = 0;
char msg[50];
char wattString[6];
char kwhString[6];
char pulseCountString[6];

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup()
{
  Serial.begin(9600);
  //setup_wifi();
  // Use the internal pullup to be able to hook up this sketch directly to an energy meter with S0 output
  // If no pullup is used, the reported usage will be too high because of the floating pin
  pinMode(DIGITAL_INPUT_SENSOR,INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(DIGITAL_INPUT_SENSOR), onPulse, RISING);
  lastSend=millis();
}

void loop()
{
    unsigned long now = millis();
    // Only send values at a maximum frequency
    bool sendTime = now - lastSend > SEND_FREQUENCY;
    if (sendTime) {
        // New watt value has been calculated
        if (dataEnergi != oldWatt) {
            // Check that we dont get unresonable large watt value.
            // could hapen when long wraps or false interrupt triggered
            if (dataEnergi<((unsigned long)MAX_WATT)) {
                // convert to a string with 2 digits before the comma and 2 digits for precision
             //   dtostrf(watt, 4, 1, wattString);                  
                //client.publish(mqtt_topic_watt,wattString);  // Publish watt to MQTT topic
            }
            Serial.print("Watt:");
            Serial.println(dataEnergi);
            //SendtoDB();


            
           // Serial.println(wattString);
            oldWatt = dataEnergi;
           // dtostrf(pulseCount, 4, 1, pulseCountString); // To Do: convert int to str, but not like this
            //client.publish(mqtt_topic_pulse,pulseCountString);  // Publish pulses to MQTT topic
            double kwh = ((double)pulseCount/((double)PULSE_FACTOR));
            // convert to a string with 2 digits before the comma and 2 digits for precision
//            dtostrf(kwh, 2, 2, kwhString);
            //client.publish(mqtt_topic_kwh,kwhString);  // Publish kwh to MQTT topic
            oldKwh = kwh;
            lastSend = now;  // once every thing is published we update the send time

            int len = client.available();
            if (len > 0){
              byte buffer[80];
              if (len > 80) len = 80;
              client.read(buffer, len);
              if (printWebData) {
                Serial.write(buffer, len);
              }
              byteCount = byteCount + len;
            }
        
        }
    }

}

//// insert data to DB via control.php
//void SendtoDB(){
//
//  if (client.connect(server, 80)){
//    Serial.println("");
//    Serial.println("connected");
//    // Make a HTTP request:
//    Serial.print("GET /arduino_mysql/control.php?dataEnergi=");
//    Serial.print(dataEnergi);
//    Serial.println("");
//
//    client.print("GET /arduino_mysql/control.php?dataEnergi=");
//    client.print(dataEnergi);
//    client.print(" ");
//    client.print("HTTP/1.1");
//    client.println();
//    client.println("Host: 192.168.100.208");
//    client.println("Connection: close");
//    client.println();
//  } else {
//    // if you didn't get a connection to the server
//    Serial.println("connection failed");
//  }
//}



void onPulse()
{
    unsigned long newBlink = micros();
    unsigned long interval = newBlink-lastBlink;
    if (interval<10000L) { // Sometimes we get interrupt on RISING
            return;
    }
    dataEnergi = (3600000000.0 /interval) / ppwh;
    lastBlink = newBlink;
    pulseCount++;
}
