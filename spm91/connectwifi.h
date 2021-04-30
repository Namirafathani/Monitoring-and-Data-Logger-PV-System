//#include <Arduino.h>



void connectwifi(String ssid, String password) {
  String cmd = "AT+CWJAP=\"" + ssid + "\",\"" + password + "\"";
  Serial.println(ssid);
  Serial.print("[Connecting Wifi]");
  Serial2.println(cmd);
  delay(1000);
  while (!Serial2.find("OK")) {
    Serial.print(".");
    Serial2.println(cmd);
    delay(1000);
  }
  Serial.println("OK");
}
