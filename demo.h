#include <WiFi.h>
#include <PubSubClient.h>
#include <AM2302-Sensor.h>
#include <HardwareSerial.h>
#include <time.h>

// ===== NTP =====
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 25200;
const int   daylightOffset_sec = 0;

// ===== WiFi =====
const char* ssid = "wora";
const char* password = "BallAfu123?";

// ===== MQTT =====
const char* mqtt_server = "broker.hivemq.com";
WiFiClient espClient;
PubSubClient client(espClient);

// ===== Sensor =====
#define DHT_PIN 4
#define MQ2_PIN 34
AM2302::AM2302_Sensor am2302{DHT_PIN};
HardwareSerial pmSerial(1);

// ===== Timing =====
unsigned long lastRead = 0;
const long interval = 2000;

// ===== WiFi =====
void setup_wifi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nWiFi FAILED - continuing without WiFi");
  }
}

// ===== MQTT reconnect =====
void reconnect() {
  Serial.println("Attempting MQTT connection...");
  String clientId = "ESP32-" + String(random(0xffff), HEX);
  if (client.connect(clientId.c_str())) {
    Serial.println("MQTT connected!");
  } else {
    Serial.print("MQTT failed, rc=");
    Serial.println(client.state());
  }
}

// ===== อ่าน PM2.5 =====
int readPM25() {
  while (pmSerial.available() >= 32) {
    if (pmSerial.peek() == 0x42) {
      uint8_t buf[32];
      pmSerial.readBytes(buf, 32);
      if (buf[1] == 0x4D) {
        return (buf[12] << 8) | buf[13];
      }
    } else {
      pmSerial.read();
    }
  }
  return -1;
}

// ===== อ่าน MQ-2 =====
int readMQ2() {
  return analogRead(MQ2_PIN);  
}

// ===== setup =====
void setup() {
  Serial.begin(115200);
  delay(1000);

  setup_wifi();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("NTP synced");

  client.setServer(mqtt_server, 1883);
  client.setBufferSize(512);
  client.setKeepAlive(60);

  am2302.begin();
  pmSerial.begin(9600, SERIAL_8N1, 16, 17);

  Serial.println("MQ-2 warming up (20s)...");
  delay(20000);

  Serial.println("Setup done");
}

// ===== loop =====
void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) reconnect();
    client.loop();
  }

  unsigned long currentMillis = millis();
  if (currentMillis - lastRead >= interval) {
    lastRead = currentMillis;

    am2302.read();
    float temp  = am2302.get_Temperature();
    float hum   = am2302.get_Humidity();
    int   pm25  = readPM25();
    if (pm25 == -1) pm25 = 0;
    int   smoke = readMQ2(); 

    time_t now;
    time(&now);

    String payload = "{";
    payload += "\"pm25Value\":\""   + String(pm25)        + "\",";
    payload += "\"co2Value\":\""    + String(smoke)        + "\",";  
    payload += "\"humidValue\":\""  + String(hum)          + "\",";
    payload += "\"temperature\":\"" + String(temp)         + "\",";
    payload += "\"timestamp\":\""   + String((long)now)    + "\"";
    payload += "}";

    Serial.println(payload);
    Serial.println("----------------------");

    if (WiFi.status() == WL_CONNECTED && client.connected()) {
      client.publish("pmproj/sensor/outside", payload.c_str());
    }
  }
}
