#define RXD2 16
#define TXD2 17

// Gas sensor thresholds
#define MQ135_THRESHOLD 100   // Air quality/H2S
#define MQ7_THRESHOLD   300   // Carbon Monoxide
#define MQ5_THRESHOLD   500   // Methane

// O2 thresholds and calibration
#define O2_SAFE_THRESHOLD     19.5  // >19.5% = Green (Safe)
#define O2_WARNING_THRESHOLD  16.0  // 16-19.5% = Yellow (Warning)
                                    // <16% = Red (Danger)

// O2 Calibration constants - ADJUST THESE BASED ON YOUR SENSOR
#define O2_CALIBRATION_FACTOR 0.087  // Adjusted for more realistic readings (21% at ~241 raw)
#define O2_ZERO_OFFSET        0      // Baseline offset

// MQ135 pins
#define MQ135_BUZZER 27
#define MQ135_LED    4
#define MQ135_BUTTON 21
#define MQ135_STATUS 12

// MQ7 pins
#define MQ7_BUZZER   26
#define MQ7_LED      33
#define MQ7_BUTTON   19
#define MQ7_STATUS   13

// MQ5 pins
#define MQ5_BUZZER   25
#define MQ5_LED      32
#define MQ5_BUTTON   18
#define MQ5_STATUS   15

// RGB LED pins for O2 status
#define RGB_RED_PIN   2
#define RGB_GREEN_PIN 23  // Changed from GPIO 0 to GPIO 23
#define RGB_BLUE_PIN  5

// Data reception status LED
#define STATUS_LED_PIN 14
#define STATUS_BLINK_MS 300
#define DATA_TIMEOUT_MS 10000  // 10 seconds timeout

// Global variables to store current sensor values - MOVED TO TOP
int currentMQ135 = 0, currentMQ7 = 0, currentMQ5 = 0;

// Button states
bool alertsEnabled_MQ135 = true, alertsEnabled_MQ7 = true, alertsEnabled_MQ5 = true;
bool lastButtonState_MQ135 = HIGH, lastButtonState_MQ7 = HIGH, lastButtonState_MQ5 = HIGH;

// Independent timing for each sensor pattern
unsigned long statusLedOffTime = 0;
unsigned long prevMillis135 = 0, prevMillis7 = 0, prevMillis5 = 0;
unsigned long lastDataReceived = 0;
int mq7PatternStep = 0;   // for MQ7 double-beep sequence

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  // Initialize gas sensor pins
  pinMode(MQ135_BUZZER, OUTPUT); pinMode(MQ135_LED, OUTPUT); pinMode(MQ135_STATUS, OUTPUT); pinMode(MQ135_BUTTON, INPUT_PULLUP);
  pinMode(MQ7_BUZZER, OUTPUT);   pinMode(MQ7_LED, OUTPUT);   pinMode(MQ7_STATUS, OUTPUT);   pinMode(MQ7_BUTTON, INPUT_PULLUP);
  pinMode(MQ5_BUZZER, OUTPUT);   pinMode(MQ5_LED, OUTPUT);   pinMode(MQ5_STATUS, OUTPUT);   pinMode(MQ5_BUTTON, INPUT_PULLUP);

  // Initialize RGB LED pins
  pinMode(RGB_RED_PIN, OUTPUT);
  pinMode(RGB_GREEN_PIN, OUTPUT);
  pinMode(RGB_BLUE_PIN, OUTPUT);

  // Initialize status LED
  pinMode(STATUS_LED_PIN, OUTPUT);

  // Set initial status LED states
  digitalWrite(MQ135_STATUS, HIGH);
  digitalWrite(MQ7_STATUS, HIGH);
  digitalWrite(MQ5_STATUS, HIGH);

  // Set RGB to off initially (all pins HIGH for common anode)
  setRGBColor(0, 0, 0);

  Serial.println("=== Gas Detection Receiver Started ===");
  Serial.println("Listening for sensor data...");
  Serial.println("O2 Thresholds: >19.5%(Green) 16-19.5%(Yellow) <16%(Red)");
}

void loop() {
  // Handle button controls
  handleButton(MQ135_BUTTON, alertsEnabled_MQ135, lastButtonState_MQ135, MQ135_STATUS, "MQ135");
  handleButton(MQ7_BUTTON, alertsEnabled_MQ7, lastButtonState_MQ7, MQ7_STATUS, "MQ7");
  handleButton(MQ5_BUTTON, alertsEnabled_MQ5, lastButtonState_MQ5, MQ5_STATUS, "MQ5");

  // Handle status LED timeout
  if (statusLedOffTime != 0 && millis() > statusLedOffTime) {
    digitalWrite(STATUS_LED_PIN, LOW);
    statusLedOffTime = 0;
  }

  // Check for data timeout (communication loss)
  checkDataTimeout();

  // Process incoming UART data
  if (Serial2.available()) {
    processIncomingData();
  }

  // Run independent alert patterns (can run simultaneously)
  if (alertsEnabled_MQ135) patternMQ135();
  if (alertsEnabled_MQ7)   patternMQ7();
  if (alertsEnabled_MQ5)   patternMQ5();
}

void processIncomingData() {
  String data = Serial2.readStringUntil('\n');
  data.trim();
  Serial.println("Received: " + data);

  // Blink status LED to show data reception
  digitalWrite(STATUS_LED_PIN, HIGH);
  statusLedOffTime = millis() + STATUS_BLINK_MS;
  lastDataReceived = millis();

  // Clean up data for parsing
  data.replace(" ", "");   // Remove spaces
  data.replace("°C", "");  // Remove degree symbol
  data.replace("%", "");   // Remove percent symbol

  // Parse the new 6-field format: Temp,Humidity,MQ7,MQ5,MQ135,O2Raw
  float temp, hum;
  int mq7, mq5, mq135, o2raw;

  int parsed = sscanf(data.c_str(),
                      "Temp:%f,Humidity:%f,MQ7:%d,MQ5:%d,MQ135:%d,O2Raw:%d",
                      &temp, &hum, &mq7, &mq5, &mq135, &o2raw);

  if (parsed == 6) {
    // Calculate O2 percentage from raw value
    float o2percent = (o2raw * O2_CALIBRATION_FACTOR) + O2_ZERO_OFFSET;
    o2percent = constrain(o2percent, 0.0, 30.0);  // Increased upper limit

    // Store sensor values for alert patterns
    currentMQ135 = mq135;
    currentMQ7 = mq7;
    currentMQ5 = mq5;

    // Handle O2 RGB LED indication
    handleOxygenStatus(o2percent);

    // Display parsed data with debug info
    Serial.println("=== SENSOR DATA ===");
    Serial.printf("Temperature: %.1f°C\n", temp);
    Serial.printf("Humidity: %.1f%%\n", hum);
    Serial.printf("MQ7 (CO): %d %s\n", mq7, (mq7 > MQ7_THRESHOLD) ? "ALERT!" : "OK");
    Serial.printf("MQ5 (CH4): %d %s\n", mq5, (mq5 > MQ5_THRESHOLD) ? "ALERT!" : "OK");
    Serial.printf("MQ135 (Air): %d %s\n", mq135, (mq135 > MQ135_THRESHOLD) ? "ALERT!" : "OK");
    Serial.printf("O2 Raw: %d | O2: %.2f%% %s\n", o2raw, o2percent, getO2Status(o2percent).c_str());
    Serial.printf("DEBUG - Raw calc: %.2f, After constrain: %.2f\n", (o2raw * O2_CALIBRATION_FACTOR), o2percent);
    Serial.println("==================");
    
  } else {
    Serial.println("⚠ Parse Error! Expected 6 fields, got " + String(parsed));
  }
}

// Button toggle with sensor name for debugging
void handleButton(int buttonPin, bool &alertsEnabled, bool &lastButtonState, int statusLED, String sensorName) {
  bool state = digitalRead(buttonPin);
  if (lastButtonState == HIGH && state == LOW) {
    alertsEnabled = !alertsEnabled;
    digitalWrite(statusLED, alertsEnabled ? HIGH : LOW);
    
    // IMPORTANT: Turn OFF buzzer and LED when disabling alerts
    if (!alertsEnabled) {
      if (sensorName == "MQ135") {
        digitalWrite(MQ135_BUZZER, LOW);
        digitalWrite(MQ135_LED, LOW);
      } else if (sensorName == "MQ7") {
        digitalWrite(MQ7_BUZZER, LOW);
        digitalWrite(MQ7_LED, LOW);
        mq7PatternStep = 0;  // Reset pattern step
      } else if (sensorName == "MQ5") {
        digitalWrite(MQ5_BUZZER, LOW);
        digitalWrite(MQ5_LED, LOW);
      }
    }
    
    Serial.println(sensorName + " alerts " + (alertsEnabled ? "ENABLED" : "DISABLED"));
    delay(200);
  }
  lastButtonState = state;
}

// Check for communication timeout
void checkDataTimeout() {
  if (lastDataReceived > 0 && (millis() - lastDataReceived > DATA_TIMEOUT_MS)) {
    // Flash status LED to indicate communication loss
    digitalWrite(STATUS_LED_PIN, (millis() / 500) % 2);
    
    // Flash RGB LED red for communication error
    if ((millis() / 1000) % 2) {
      setRGBColor(255, 0, 0);  // Red
    } else {
      setRGBColor(0, 0, 0);    // Off
    }
  }
}

// Handle oxygen status with RGB LED
void handleOxygenStatus(float o2percent) {
  if (o2percent > O2_SAFE_THRESHOLD) {
    // Green - Safe oxygen levels
    setRGBColor(0, 255, 0);
  } else if (o2percent >= O2_WARNING_THRESHOLD) {
    // Yellow - Warning oxygen levels
    setRGBColor(255, 255, 0);
  } else {
    // Red - Dangerous oxygen levels
    setRGBColor(255, 0, 0);
  }
}

// Set RGB LED color - Fixed for COMMON ANODE RGB LED
void setRGBColor(int red, int green, int blue) {
  // For common anode RGB LED: LOW = ON, HIGH = OFF
  // Turn off all colors first (set all pins HIGH)
  digitalWrite(RGB_RED_PIN, HIGH);
  digitalWrite(RGB_GREEN_PIN, HIGH);
  digitalWrite(RGB_BLUE_PIN, HIGH);
  
  // Turn on only the required colors (set pins LOW)
  if (red > 0) digitalWrite(RGB_RED_PIN, LOW);
  if (green > 0) digitalWrite(RGB_GREEN_PIN, LOW);
  if (blue > 0) digitalWrite(RGB_BLUE_PIN, LOW);
}

// Get O2 status string for display
String getO2Status(float o2percent) {
  if (o2percent > O2_SAFE_THRESHOLD) {
    return "SAFE (Green)";
  } else if (o2percent >= O2_WARNING_THRESHOLD) {
    return "WARNING (Yellow)";
  } else {
    return "DANGER (Red)";
  }
}

// --- INDEPENDENT ALERT PATTERNS ---

// MQ135 = Fast blink (200ms) - Air Quality/H2S
void patternMQ135() {
  if (currentMQ135 > MQ135_THRESHOLD) {
    unsigned long now = millis();
    if (now - prevMillis135 >= 200) {
      prevMillis135 = now;
      digitalWrite(MQ135_BUZZER, !digitalRead(MQ135_BUZZER));
      digitalWrite(MQ135_LED, !digitalRead(MQ135_LED));
    }
  } else {
    digitalWrite(MQ135_BUZZER, LOW);
    digitalWrite(MQ135_LED, LOW);
  }
}

// MQ7 = Double short beep pattern - Carbon Monoxide
void patternMQ7() {
  unsigned long now = millis();
  if (currentMQ7 > MQ7_THRESHOLD) {
    switch (mq7PatternStep) {
      case 0: // first beep
        digitalWrite(MQ7_BUZZER, HIGH);
        digitalWrite(MQ7_LED, HIGH);
        prevMillis7 = now;
        mq7PatternStep = 1;
        break;
      case 1: // pause after first beep
        if (now - prevMillis7 >= 100) {
          digitalWrite(MQ7_BUZZER, LOW);
          digitalWrite(MQ7_LED, LOW);
          prevMillis7 = now;
          mq7PatternStep = 2;
        }
        break;
      case 2: // second beep
        if (now - prevMillis7 >= 100) {
          digitalWrite(MQ7_BUZZER, HIGH);
          digitalWrite(MQ7_LED, HIGH);
          prevMillis7 = now;
          mq7PatternStep = 3;
        }
        break;
      case 3: // pause after second beep
        if (now - prevMillis7 >= 100) {
          digitalWrite(MQ7_BUZZER, LOW);
          digitalWrite(MQ7_LED, LOW);
          prevMillis7 = now;
          mq7PatternStep = 4;
        }
        break;
      case 4: // long pause before repeating
        if (now - prevMillis7 >= 500) {
          mq7PatternStep = 0;
        }
        break;
    }
  } else {
    digitalWrite(MQ7_BUZZER, LOW);
    digitalWrite(MQ7_LED, LOW);
    mq7PatternStep = 0;
  }
}

// MQ5 = Slow long beep (1000ms) - Methane
void patternMQ5() {
  if (currentMQ5 > MQ5_THRESHOLD) {
    unsigned long now = millis();
    if (now - prevMillis5 >= 1000) {
      prevMillis5 = now;
      digitalWrite(MQ5_BUZZER, !digitalRead(MQ5_BUZZER));
      digitalWrite(MQ5_LED, !digitalRead(MQ5_LED));
    }
  } else {
    digitalWrite(MQ5_BUZZER, LOW);
    digitalWrite(MQ5_LED, LOW);
  }
}
