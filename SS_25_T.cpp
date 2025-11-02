// Pin definitions for Arduino Uno
#define TRANSMIT_LED 12  // Data transmission indicator LED
#define STATUS_LED 13    // Built-in LED for status

// Sensor analog pins
#define MQ7_PIN A0       // Carbon Monoxide sensor
#define MQ5_PIN A1       // Methane sensor  
#define MQ135_PIN A2     // Air quality/H2S sensor

// Calibration constants
#define WARMUP_TIME_MS 60000  // 60 seconds warmup for MQ sensors

// Global variables
unsigned long startTime;
int transmissionCount = 0;
bool sensorsWarmedUp = false;

void setup() {
  Serial.begin(9600);  // UART communication with ESP32 (TX/RX only - no debug!)
  
  // Initialize pins
  pinMode(STATUS_LED, OUTPUT);
  pinMode(TRANSMIT_LED, OUTPUT);
  
  startTime = millis();
  digitalWrite(STATUS_LED, HIGH);  // Power indicator
  
  // Wait for warmup silently (LED will blink)
}

void loop() {
  // Check if sensors are still warming up
  if (!sensorsWarmedUp) {
    if (millis() - startTime < WARMUP_TIME_MS) {
      // Blink status LED during warmup (500ms intervals)
      digitalWrite(STATUS_LED, (millis() / 500) % 2);
      delay(1000);
      return;
    } else {
      sensorsWarmedUp = true;
      digitalWrite(STATUS_LED, HIGH);  // Solid when ready
    }
  }
  
  // Read and transmit sensor data
  readAndTransmitData();
  
  delay(3000);  // Send data every 3 seconds
}

void readAndTransmitData() {
  // Read gas sensors (average of 3 readings for stability)
  int mq7 = getStableReading(MQ7_PIN);
  int mq5 = getStableReading(MQ5_PIN);
  int mq135 = getStableReading(MQ135_PIN);
  
  // Create data string
  char dataBuffer[100];
  snprintf(dataBuffer, sizeof(dataBuffer),
           "MQ7:%d,MQ5:%d,MQ135:%d",
           mq7, mq5, mq135);
  
  // Blink transmit LED briefly
  digitalWrite(TRANSMIT_LED, HIGH);
  
  // Transmit ONLY sensor data via UART (no debug text!)
  Serial.println(dataBuffer);
  
  // Small delay for UART stability
  delay(50);
  digitalWrite(TRANSMIT_LED, LOW);
  
  // Show transmission count on status LED (brief flash every 10 transmissions)
  transmissionCount++;
  if (transmissionCount % 10 == 0) {
    digitalWrite(STATUS_LED, LOW);
    delay(50);
    digitalWrite(STATUS_LED, HIGH);
  }
}

// Get stable analog reading (average of 3 samples)
int getStableReading(int pin) {
  long sum = 0;
  for (int i = 0; i < 3; i++) {
    sum += analogRead(pin);
    delay(10);
  }
  return sum / 3;
}
