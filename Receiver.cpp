#define RXD2 16   // Choose ESP32 GPIO for RX
#define TXD2 17   // Choose ESP32 GPIO for TX

void setup() {
  Serial.begin(115200);                            // For Serial Monitor
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);     // Define pins + baud rate
  Serial.println("UART Receiver Ready....");
}

void loop() {
  if (Serial2.available()) {
    String data = Serial2.readStringUntil('\n');   // Read until newline
    Serial.println("Received: " + data);
  }
}
