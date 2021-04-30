
void httppost (String server, String uri, String data) {
  String cmd_tcp = "AT+CIPSTART=\"TCP\",\"" + server + "\",80";

  String postRequest =
    "POST " + uri + " HTTP/1.1\r\n" +
    "Host: " + server + "\r\n" +
    "Content-Length: " + data.length() + "\r\n" +
    "Content-Type: application/x-www-form-urlencoded\r\n" +
    "\r\n" + data;

  String sendCmd = "AT+CIPSEND=";//determine the number of characters to be sent.

  Serial.println("[Connecting to Server]");
  Serial2.println(cmd_tcp);//start a TCP connection.
  delay(1000);
  if ( Serial2.find("OK")) {
    Serial.println("[TCP connection ready]");
  }

  Serial2.print(sendCmd);
  Serial2.println(postRequest.length() );

  delay(500);

  if (Serial2.find(">")) {
    Serial.println("[Sending..]"); Serial2.print(postRequest);
    //"Voltage=" + voltage + "; Current=" + current + "; Power=" + power + "; Energy=" + energy;
    Serial.println(data);

    if ( Serial2.find("SEND OK")) {
      Serial.println("[Packet sent]");

      while (Serial2.available()) {
        String tmpResp = esp.readString();
        //Serial.println(tmpResp);
      }

      // close the connection
      Serial2.println("AT+CIPCLOSE");
    }
  }
}
