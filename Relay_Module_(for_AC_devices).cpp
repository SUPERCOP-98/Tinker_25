#define relayPin 7

void setup() {
  pinMode(relayPin, OUTPUT);
}

void loop() {
  digitalWrite(relayPin, HIGH); // Turn ON
  delay(2000);
  digitalWrite(relayPin, LOW);  // Turn OFF
  delay(2000);
}
