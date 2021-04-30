
#define esp Serial2

void resetwifi() {
  esp.println("AT+RST");
  delay(1000);
  if (esp.find("OK") ) Serial.println("[Module Reset]");
}

