#include <DHT.h>

// Pin definitions
#define DHTPIN 27        // DHT11 sensor pin
#define DHTTYPE DHT11
#define RXD2 16          // UART receive pin
#define TXD2 17          // UART transmit pin
#define STATUS_LED 2     // Power/status indicator LED
#define TRANSMIT_LED 4   // Data transmission indicator LED

// Sensor analog pins
#define MQ7_PIN 35       // Carbon Monoxide sensor
#define MQ5_PIN 34       // Methane sensor  
#define MQ135_PIN 32     // Air quality/H2S sensor
#define O2_PIN 33        // Oxygen sensor

// Calibration constants
#define O2_CALIBRATION_FACTOR 0.15  // Adjust based on sensor datasheet
#define O2_ZERO_OFFSET 0            // Baseline offset for O2 sensor
#define WARMUP_TIME_MS 60000        // 60 seconds warmup for MQ sensors

// Global variables
DHT dht(DHTPIN, DHTTYPE);
unsigned long startTime;
int transmissionCount = 0;
bool sensorsWarmedUp = false;

void setup() {
  Serial.begin(115200);   
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);  
  
  // Initialize pins
  pinMode(STATUS_LED, OUTPUT);
  pinMode(TRANSMIT_LED, OUTPUT);
  
  // Initialize sensors
  dht.begin();
  startTime = millis();
  
  digitalWrite(STATUS_LED, HIGH);  // Power indicator
  
  Serial.println("=== Gas Sensor Transmitter Started ===");
  Serial.println("Role: Data collection and transmission only");
  Serial.println("Warming up MQ sensors for 60 seconds...");
}

void loop() {
  // Check if sensors are still warming up
  if (!sensorsWarmedUp) {
    if (millis() - startTime < WARMUP_TIME_MS) {
      unsigned long remaining = (WARMUP_TIME_MS - (millis() - startTime)) / 1000;
      if (remaining % 10 == 0 || remaining < 10) {
        Serial.println("Sensors warming up... " + String(remaining) + "s remaining");
      }
      // Blink status LED during warmup
      digitalWrite(STATUS_LED, (millis() / 500) % 2);
      delay(1000);
      return;
    } else {
      sensorsWarmedUp = true;
      digitalWrite(STATUS_LED, HIGH);
      Serial.println("✓ Sensors ready! Starting data transmission...");
    }
  }
  
  // Read and transmit sensor data
  readAndTransmitData();
  
  delay(3000);
}

void readAndTransmitData() {
  // Read DHT sensor
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  
  // Handle DHT errors
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("DHT sensor error - sending error values");
    temperature = -999;
    humidity = -999;
  }
  
  // Read gas sensors (average of 3 readings for stability)
  int mq7 = getStableReading(MQ7_PIN);
  int mq5 = getStableReading(MQ5_PIN);
  int mq135 = getStableReading(MQ135_PIN);
  int o2raw = getStableReading(O2_PIN);  // Send RAW value only
  
  // Create data string - Send O2 raw value only, receiver will calculate percentage
  char dataBuffer[200];
  snprintf(dataBuffer, sizeof(dataBuffer),
           "Temp:%.1f,Humidity:%.1f,MQ7:%d,MQ5:%d,MQ135:%d,O2Raw:%d,O2:%.2f",
           temperature, humidity, mq7, mq5, mq135, o2raw, 0.0);  // O2 percentage = 0 (placeholder)
  
  // Transmit data via UART with stability delay
  delay(50);  // Small delay for UART stability and reduce transmission errors
  Serial.println("TX #" + String(++transmissionCount) + ": " + String(dataBuffer));
  Serial2.println(dataBuffer);
  
  // Show raw readings (no O2 percentage calculation)
  Serial.printf("Raw readings - T:%.1f H:%.1f MQ7:%d MQ5:%d MQ135:%d O2_Raw:%d\n", 
                temperature, humidity, mq7, mq5, mq135, o2raw);
}

// Get stable analog reading (average of 3 samples)
int getStableReading(int pin) {
  int sum = 0;
  for (int i = 0; i < 3; i++) {
    sum += analogRead(pin);
    delay(10);
  }
  return sum / 3;
}

// Diagnostic function for troubleshooting
void runDiagnostics() {
  Serial.println("\n=== SENSOR DIAGNOSTICS ===");
  
  // Test DHT11
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  Serial.printf("DHT11 (pin %d): T=%.1f H=%.1f %s\n", DHTPIN, t, h,
                (isnan(t) || isnan(h)) ? "❌ FAILED" : "✓ OK");
  
  // Test analog sensors
  Serial.printf("MQ7 (pin %d): %d\n", MQ7_PIN, analogRead(MQ7_PIN));
  Serial.printf("MQ5 (pin %d): %d\n", MQ5_PIN, analogRead(MQ5_PIN));
  Serial.printf("MQ135 (pin %d): %d\n", MQ135_PIN, analogRead(MQ135_PIN));
  Serial.printf("O2 (pin %d): %d\n", O2_PIN, analogRead(O2_PIN));
  
  Serial.println("=== DIAGNOSTICS COMPLETE ===\n");
}
