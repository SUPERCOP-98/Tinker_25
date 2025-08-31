void setup() {
  Serial.begin(9600);  // start serial at 9600 baud
}

void loop() {
  Serial.println("Hello from Arduino!");
  delay(1000);

  if (Serial.available() > 0) {
    char data = Serial.read();
    Serial.print("You typed: ");
    Serial.println(data);
  }
}
