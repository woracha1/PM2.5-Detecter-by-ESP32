#include <AM2302-Sensor.h>
#include <HardwareSerial.h>

// sensor AM2302 ใช้วัดและแสดง temperature และ humidity
#define DHT_PIN 4
AM2302::AM2302_Sensor am2302{DHT_PIN};

// sensor PMS5003 ใช้วัดและแสดงค่า PM2.5
HardwareSerial pmSerial(1);

// ฟังก์ชันอ่าน PM2.5 แบบ sync packet
int readPM25() {
  while (pmSerial.available() >= 32) {

    // หา header 0x42
    if (pmSerial.peek() == 0x42) {

      uint8_t buf[32];
      pmSerial.readBytes(buf, 32);

      // เช็ค header ตัวที่ 2
      if (buf[1] == 0x4D) {
        int pm25 = (buf[12] << 8) | buf[13];
        return pm25;
      }
    } 
    else {
      pmSerial.read(); // ทิ้ง byte ที่ไม่ใช่ header
    }
  }

  return -1; // ยังไม่มีข้อมูล
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Start System...");

  am2302.begin();
  pmSerial.begin(9600, SERIAL_8N1, 16, 17);
}

void loop() {

  // AM2302 
  auto status = am2302.read();

  Serial.print("Status: ");
  Serial.println(AM2302::AM2302_Sensor::get_sensorState(status));

  Serial.print("Temp: ");
  Serial.print(am2302.get_Temperature());
  Serial.println(" °C");

  Serial.print("Humidity: ");
  Serial.print(am2302.get_Humidity());
  Serial.println(" %");

  // PM2.5
  int pm25 = readPM25();

  Serial.print("PM2.5: ");
  if (pm25 != -1) {
    Serial.print(pm25);
    Serial.println(" ug/m3");
  } else {
    Serial.println("No data");
  }

  Serial.println("----------------------");

  delay(2000);
}
