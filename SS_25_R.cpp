#define RXD2 16
#define TXD2 17

// Gas sensor thresholds
#define MQ135_THRESHOLD 660   // Air quality/H2S
#define MQ7_THRESHOLD   660   // Carbon Monoxide
#define MQ5_THRESHOLD   660   // Methane

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

// Data reception status LED
#define STATUS_LED_PIN 14
#define STATUS_BLINK_MS 300
#define DATA_TIMEOUT_MS 10000  // 10 seconds timeout

// Connection loss warning pattern
#define WARNING_BLINK_COUNT 5
#define WARNING_BLINK_ON_MS 200
#define WARNING_BLINK_OFF_MS 200
#define WARNING_PAUSE_MS 10000

// Global variables to store current sensor values
int currentMQ135 = 0, currentMQ7 = 0, currentMQ5 = 0;

// Button states
bool alertsEnabled_MQ135 = true, alertsEnabled_MQ7 = true, alertsEnabled_MQ5 = true;
bool lastButtonState_MQ135 = HIGH, lastButtonState_MQ7 = HIGH, lastButtonState_MQ5 = HIGH;

// Independent timing for each sensor pattern
unsigned long statusLedOffTime = 0;
unsigned long prevMillis135 = 0, prevMillis7 = 0, prevMillis5 = 0;
unsigned long lastDataReceived = 0;
bool dataReceivedOnce = false;
int mq7PatternStep = 0;   // for MQ7 double-beep sequence

// Connection loss warning variables
unsigned long warningBlinkTimer = 0;
int warningBlinkCount = 0;
bool warningBlinkState = false;
enum WarningState { WAITING, BLINKING, PAUSING };
WarningState warningState = WAITING;

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  // Initialize gas sensor pins
  pinMode(MQ135_BUZZER, OUTPUT); pinMode(MQ135_LED, OUTPUT); pinMode(MQ135_STATUS, OUTPUT); pinMode(MQ135_BUTTON, INPUT_PULLUP);
  pinMode(MQ7_BUZZER, OUTPUT);   pinMode(MQ7_LED, OUTPUT);   pinMode(MQ7_STATUS, OUTPUT);   pinMode(MQ7_BUTTON, INPUT_PULLUP);
  pinMode(MQ5_BUZZER, OUTPUT);   pinMode(MQ5_LED, OUTPUT);   pinMode(MQ5_STATUS, OUTPUT);   pinMode(MQ5_BUTTON, INPUT_PULLUP);

  // Initialize status LED
  pinMode(STATUS_LED_PIN, OUTPUT);

  // Set initial status LED states
  digitalWrite(MQ135_STATUS, HIGH);
  digitalWrite(MQ7_STATUS, HIGH);
  digitalWrite(MQ5_STATUS, HIGH);

  Serial.println("=== Gas Detection Receiver Started ===");
  Serial.println("Listening for MQ sensor data from Arduino...");
  Serial.println("MQ7=CO | MQ5=CH4 | MQ135=Air Quality/H2S");
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
  dataReceivedOnce = true;
  
  // Reset warning state when data is received
  warningState = WAITING;
  warningBlinkCount = 0;

  // Clean up data for parsing
  data.replace(" ", "");

  // Parse the format: MQ7:xxx,MQ5:xxx,MQ135:xxx
  int mq7, mq5, mq135;

  int parsed = sscanf(data.c_str(),
                      "MQ7:%d,MQ5:%d,MQ135:%d",
                      &mq7, &mq5, &mq135);

  if (parsed == 3) {
    // Store sensor values for alert patterns
    currentMQ135 = mq135;
    currentMQ7 = mq7;
    currentMQ5 = mq5;

    // Display parsed data
    Serial.println("=== SENSOR DATA ===");
    Serial.printf("MQ7 (CO): %d %s\n", mq7, (mq7 > MQ7_THRESHOLD) ? "ALERT!" : "OK");
    Serial.printf("MQ5 (CH4): %d %s\n", mq5, (mq5 > MQ5_THRESHOLD) ? "ALERT!" : "OK");
    Serial.printf("MQ135 (Air): %d %s\n", mq135, (mq135 > MQ135_THRESHOLD) ? "ALERT!" : "OK");
    Serial.println("==================");
    
  } else {
    Serial.println("Parse Error! Expected 3 fields, got " + String(parsed));
  }
}

// Button toggle with sensor name for debugging
void handleButton(int buttonPin, bool &alertsEnabled, bool &lastButtonState, int statusLED, String sensorName) {
  bool state = digitalRead(buttonPin);
  if (lastButtonState == HIGH && state == LOW) {
    alertsEnabled = !alertsEnabled;
    digitalWrite(statusLED, alertsEnabled ? HIGH : LOW);
    
    // Turn OFF buzzer and LED when disabling alerts
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

// Check for communication timeout and handle warning pattern
void checkDataTimeout() {
  // Only check timeout if we've received data at least once
  if (!dataReceivedOnce || (millis() - lastDataReceived <= DATA_TIMEOUT_MS)) {
    return;  // Connection is OK or never established
  }

  // Connection lost - run warning pattern
  unsigned long now = millis();
  
  switch (warningState) {
    case WAITING:
      // Start the warning sequence
      warningState = BLINKING;
      warningBlinkCount = 0;
      warningBlinkState = true;
      digitalWrite(STATUS_LED_PIN, HIGH);
      warningBlinkTimer = now;
      Serial.println("CONNECTION LOST - Starting warning blinks");
      break;
      
    case BLINKING:
      // Handle the 5 blinks
      if (warningBlinkState) {
        // LED is ON, wait for ON duration
        if (now - warningBlinkTimer >= WARNING_BLINK_ON_MS) {
          digitalWrite(STATUS_LED_PIN, LOW);
          warningBlinkState = false;
          warningBlinkTimer = now;
          warningBlinkCount++;
        }
      } else {
        // LED is OFF, wait for OFF duration
        if (now - warningBlinkTimer >= WARNING_BLINK_OFF_MS) {
          if (warningBlinkCount < WARNING_BLINK_COUNT) {
            // Continue blinking
            digitalWrite(STATUS_LED_PIN, HIGH);
            warningBlinkState = true;
            warningBlinkTimer = now;
          } else {
            // Finished 5 blinks, start pause
            warningState = PAUSING;
            warningBlinkTimer = now;
          }
        }
      }
      break;
      
    case PAUSING:
      // Wait 10 seconds before repeating
      if (now - warningBlinkTimer >= WARNING_PAUSE_MS) {
        warningState = WAITING;  // Restart the cycle
      }
      break;
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
