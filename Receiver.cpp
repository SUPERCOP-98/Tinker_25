void setup() {
  Serial.begin(9600);   // Start UART at 9600 baud
}

void loop() {
  if (Serial.available() > 0) {       // Check if data is available
    char data = Serial.read();        // Read one byte (character)
    Serial.print("Received: ");
    Serial.println(data);             // Print received data
  }
}
