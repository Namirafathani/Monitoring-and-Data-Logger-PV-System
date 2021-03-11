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
#include <PubSubClient.h>
#include <WiFi101.h>
#include <ArduinoJson.h>

// Update these with values suitable for your network.

/************************* WiFi Access Point *********************************/

const char* ssid = "GL 73";
const char* password = "gbglor73";

/**************************** MQTT Broker ************************************/

const char* mqtt_server = "192.168.100.206"; // example: "192.168.0.8"

const char* mqtt_topic_watt = "mkr-energy-01/watt";
const char* mqtt_topic_kwh = "mkr-energy-01/kwh";
const char* mqtt_topic_pulse = "mkr-energy-01/pulse";
const char* mkr1000 = "192.168.100.208";

#define DIGITAL_INPUT_SENSOR 6 // The digital input you attached S0+ D6 in Wemos D1 mini
#define PULSE_FACTOR 2000       // Nummber of pulses per kWh of your meeter
#define MAX_WATT 1000          // Max watt value to report. This filters outliers.

unsigned long SEND_FREQUENCY = 20000; // Minimum time between send (in milliseconds)
double ppwh = ((double)PULSE_FACTOR)/1000; // Pulses per watt hour
volatile unsigned long pulseCount = 0;
volatile unsigned long lastBlink = 0;
volatile unsigned long watt = 0;
unsigned long oldWatt = 0;
double oldKwh;
unsigned long lastSend;
double kwh;

WiFiClient mkrClient;
PubSubClient client(mkrClient);

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

// Setup a MQTT subscription
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("mkr1000Client")) {
      Serial.println("connected");
      const size_t Restart = JSON_OBJECT_SIZE(2);
      DynamicJsonBuffer jsonBuffer(Restart);
      JsonObject& root = jsonBuffer.createObject();

      root["id_energymeter"] = "New_System";
      root["flagstart"] = 1;

      char buffermessage[300];
      root.printTo(buffermessage, sizeof(buffermessage));

      Serial.println("Sending message to MQTT topic...");
      Serial.println(buffermessage);

      client.publish("EM/startcontroller", buffermessage);

      if(client.publish("EM/startcontroller", buffermessage) == true){
        Serial.println("Success sending message");
        Serial.println("---------------------------------------------");
        Serial.println("");
      }
      else{
        Serial.println("ERROR");
        Serial.println("---------------------------------------------");
        Serial.println("");
      }
    
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(2000);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  // Use the internal pullup to be able to hook up this sketch directly to an energy meter with S0 output
  // If no pullup is used, the reported usage will be too high because of the floating pin
  pinMode(DIGITAL_INPUT_SENSOR,INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(DIGITAL_INPUT_SENSOR), onPulse, RISING);
  lastSend=millis();
}

void loop()
{
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    unsigned long now = millis();
    // Only send values at a maximum frequency
    bool sendTime = now - lastSend > SEND_FREQUENCY;
    if (sendTime) {
        // New watt value has been calculated
        if (watt != oldWatt) {
            // Check that we dont get unresonable large watt value.
            // could hapen when long wraps or false interrupt triggered
            if (watt<((unsigned long)MAX_WATT)) {
                // convert to a string with 2 digits before the comma and 2 digits for precision
             //   dtostrf(watt, 4, 1, wattString);                  
                client.publish(mqtt_topic_watt,wattString);  // Publish watt to MQTT topic
            }
            //Serial.print("Watt:");
            //Serial.println(watt);
           // Serial.println(wattString);
            oldWatt = watt;
           // dtostrf(pulseCount, 4, 1, pulseCountString); // To Do: convert int to str, but not like this
            //client.publish(mqtt_topic_pulse,pulseCountString);  // Publish pulses to MQTT topic
            double kwh = ((double)pulseCount/((double)PULSE_FACTOR));
            // convert to a string with 2 digits before the comma and 2 digits for precision
//            dtostrf(kwh, 2, 2, kwhString);
            //client.publish(mqtt_topic_kwh,kwhString);  // Publish kwh to MQTT topic
            oldKwh = kwh;
            lastSend = now;  // once every thing is published we update the send time
            publishData();
        }
    }

}

void publishData(){
  Serial.print("Publish data");
  Serial.println("watt, pulseCount and kwh");
  Serial.print(watt);
  Serial.print(", ");
  Serial.print(pulseCount);
  Serial.print(", ");
  Serial.println(kwh);

  const size_t BUFFER_SIZE = JSON_OBJECT_SIZE(7);

  DynamicJsonBuffer jsonBuffer(BUFFER_SIZE);
    JsonObject& JSONencoder = jsonBuffer.createObject();

    /* Encode object in jsonBuffer */
    JSONencoder["id_EM"] = "EM01";
    JSONencoder["watt_reading"] = watt;
    JSONencoder["PulseCount_reading"] = pulseCount;
    JSONencoder["kwh_reading"] = kwh;
    JSONencoder["flagdata"] = 1;

    char JSONmessageBuffer[500];
    JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));

    Serial.println("Sending message to MQTT topic");
    Serial.println(JSONmessageBuffer);

    client.publish("EM/datacollector", JSONmessageBuffer);

    if(client.publish("EM/datacollector", JSONmessageBuffer)==true){
      Serial.println("SUCCESS PUBLISH PAYLOAD");
    } else {
      Serial.println("ERROR PUBLISH PAYLOAD");
    }
}

void onPulse()
{
    unsigned long newBlink = micros();
    unsigned long interval = newBlink-lastBlink;
    if (interval<10000L) { // Sometimes we get interrupt on RISING
            return;
    }
    watt = (3600000000.0 /interval) / ppwh;
    lastBlink = newBlink;
    pulseCount++;
}
