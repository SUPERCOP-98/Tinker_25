#define gasPin A0

void setup() {
  Serial.begin(9600);
}

void loop() {
  int gasValue = analogRead(gasPin);
  Serial.print("Gas Sensor Value: ");
  Serial.println(gasValue);

  if (gasValue > 400) {
    Serial.println("⚠ Gas Detected!");
  }
  delay(500);
}
