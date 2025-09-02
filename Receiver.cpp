void setup() {
  Serial.begin(9600);     // USB Serial for Monitor
  Serial2.begin(9600);    // Hardware UART2 (ESP32) OR SoftwareSerial for Arduino
  // On Arduino UNO, replace Serial2 with SoftwareSerial if needed
  Serial.println("UART Receiver Ready...");
}

void loop() {
  if (Serial2.available() > 0) {
    char c = Serial2.read();       // Read data from UART
    Serial.print("Received: ");
    Serial.println(c);             // Print to Serial Monitor
  }
}
