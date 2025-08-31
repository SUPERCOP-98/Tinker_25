#include <SoftwareSerial.h>

SoftwareSerial BT(10, 11); // RX, TX

void setup() {
  Serial.begin(9600);
  BT.begin(9600);
  Serial.println("Bluetooth Ready!");
}

void loop() {
  if (BT.available()) {
    char c = BT.read();
    Serial.print("From Bluetooth: ");
    Serial.println(c);
  }

  if (Serial.available()) {
    char c = Serial.read();
    BT.print(c);
  }
}
