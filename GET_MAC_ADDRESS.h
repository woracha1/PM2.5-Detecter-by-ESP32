#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  while (!Serial);

  WiFi.mode(WIFI_STA);
  delay(100);  

  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
}

void loop() {}
