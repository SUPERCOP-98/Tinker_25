#include <DHT.h>

//PIN LAYOUT
//MQ7- 35
//MQ5 - 34
//MQ135 - 32
//AO03 - 33
//DHT11 - 27

#define DHTPIN 27
#define DHTTYPE DHT11

#define RXD2 16
#define TXD2 17

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);   
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);  
  dht.begin();
}

void loop() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  int mq7   = analogRead(35);
  int mq5   = analogRead(34);
  int mq135 = analogRead(32);
  int o2raw = analogRead(33);   // O2 sensor analog pin

  // --- Oxygen calibration ---
  // Assume 140 ADC counts ≈ 21% O2 (normal air at 1 atm, 30 °C)
  float O2_percent = (o2raw / 140.0) * 21.0;

  String data = "Temp:" + String(t) + 
                ",Humidity:" + String(h) + 
                ",MQ 7:" + String(mq7) + 
                ",MQ 5:" + String(mq5) + 
                ",MQ 135:" + String(mq135) + 
                ",O₂ Raw:" + String(o2raw) + 
                ",O₂ %:" + String(O2_percent, 2);

  Serial.println("Sending: " + data);
  Serial2.println(data);

  delay(3000);
}
