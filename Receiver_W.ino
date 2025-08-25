#include "SimpleDHT.h"

#define DHTPIN 27

#define MQ7_PIN 34
#define MQ5_PIN 35
#define MQ135_PIN 32

SimpleDHT11 dht11;

void setup() {
  Serial.begin(115200);                     // USB debug
  Serial2.begin(9600, SERIAL_8N1, 16, 17); // RX2=16, TX2=17 for wired UART

  delay(2000);                            // Sensor warm-up delay
}

void loop() {
  byte temperature = 0;
  byte humidity = 0;

  int err = dht11.read(DHTPIN, &temperature, &humidity, NULL);
  if (err != SimpleDHTErrSuccess) {
    Serial.println("Failed to read from DHT11 sensor!");
    delay(2000);
    return;
  }

  int mq7Value = analogRead(MQ7_PIN);
  int mq5Value = analogRead(MQ5_PIN);
  int mq135Value = analogRead(MQ135_PIN);

  // Format data as CSV: temp,humidity,mq7,mq5,mq135
  String data = "Temp:" + String(temperature) +
                ", Humidity:" + String(humidity) +
                ", MQ7:" + String(mq7Value) +
                ", MQ5:" + String(mq5Value) +
                ", MQ135:" + String(mq135Value);

  Serial.println("Sent: " + data);  // debug print
  Serial2.println(data);            // send over UART2

  delay(3000);
}
