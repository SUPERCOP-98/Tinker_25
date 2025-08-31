#define ldrPin A0

void setup() {
  Serial.begin(9600);
}

void loop() {
  int lightVal = analogRead(ldrPin);
  Serial.print("Light Level: ");
  Serial.println(lightVal);
  delay(500);
}
