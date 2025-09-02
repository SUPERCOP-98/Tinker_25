void setup() {
  Serial.begin(115200);   // For PC Serial Monitor
  Serial2.begin(9600);    // Must match transmitter baud rate
}

void loop() {
  if (Serial2.available()) {
    String data = Serial2.readStringUntil('\n');  // Read line from transmitter
    Serial.println("Received: " + data);          // Print on Serial Monitor
  }
}
