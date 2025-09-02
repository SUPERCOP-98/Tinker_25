#define RXD2 16
#define TXD2 17

// Thresholds
#define MQ135_THRESHOLD 100
#define MQ7_THRESHOLD   300
#define MQ5_THRESHOLD   500

// MQ135 pins
#define MQ135_BUZZER 27
#define MQ135_LED    4
#define MQ135_BUTTON 21
#define MQ135_STATUS 12

// MQ7 pins
#define MQ7_BUZZER   25
#define MQ7_LED      33
#define MQ7_BUTTON   19
#define MQ7_STATUS   13

// MQ5 pins
#define MQ5_BUZZER   26
#define MQ5_LED      32
#define MQ5_BUTTON   18
#define MQ5_STATUS   15

// Global variables for push button states
bool alertsEnabled_MQ135 = true, alertsEnabled_MQ7 = true, alertsEnabled_MQ5 = true;
bool lastButtonState_MQ135 = HIGH, lastButtonState_MQ7 = HIGH, lastButtonState_MQ5 = HIGH;

// Blinking variables
unsigned long previousMillis_MQ135 = 0, previousMillis_MQ7 = 0, previousMillis_MQ5 = 0;
const long interval = 500; // blink every 500ms
bool alertState_MQ135 = false, alertState_MQ7 = false, alertState_MQ5 = false;

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  // Setup pins
  pinMode(MQ135_BUZZER, OUTPUT); pinMode(MQ135_LED, OUTPUT); pinMode(MQ135_STATUS, OUTPUT); pinMode(MQ135_BUTTON, INPUT_PULLUP);
  pinMode(MQ7_BUZZER, OUTPUT); pinMode(MQ7_LED, OUTPUT); pinMode(MQ7_STATUS, OUTPUT); pinMode(MQ7_BUTTON, INPUT_PULLUP);
  pinMode(MQ5_BUZZER, OUTPUT); pinMode(MQ5_LED, OUTPUT); pinMode(MQ5_STATUS, OUTPUT); pinMode(MQ5_BUTTON, INPUT_PULLUP);

  // Initialize outputs
  digitalWrite(MQ135_BUZZER, LOW); digitalWrite(MQ135_LED, LOW); digitalWrite(MQ135_STATUS, HIGH);
  digitalWrite(MQ7_BUZZER, LOW); digitalWrite(MQ7_LED, LOW); digitalWrite(MQ7_STATUS, HIGH);
  digitalWrite(MQ5_BUZZER, LOW); digitalWrite(MQ5_LED, LOW); digitalWrite(MQ5_STATUS, HIGH);

  Serial.println("UART Receiver with 3 Sensors & Alerts Ready...");
}

void loop() {
  // --- Button handling (toggle on press) ---
  handleButton(MQ135_BUTTON, alertsEnabled_MQ135, lastButtonState_MQ135, MQ135_STATUS);
  handleButton(MQ7_BUTTON, alertsEnabled_MQ7, lastButtonState_MQ7, MQ7_STATUS);
  handleButton(MQ5_BUTTON, alertsEnabled_MQ5, lastButtonState_MQ5, MQ5_STATUS);

  // --- UART data reading ---
  if (Serial2.available()) {
    String data = Serial2.readStringUntil('\n');
    Serial.println("Received: " + data);

    float temp, hum;
    int mq7, mq5, mq135;

    if (sscanf(data.c_str(), "Temp:%f,Humidity:%f,MQ7:%d,MQ5:%d,MQ135:%d",
               &temp, &hum, &mq7, &mq5, &mq135) == 5) {

      // Handle MQ135
      handleAlert(mq135, MQ135_THRESHOLD, alertsEnabled_MQ135, MQ135_BUZZER, MQ135_LED, alertState_MQ135, previousMillis_MQ135, "MQ135");

      // Handle MQ7
      handleAlert(mq7, MQ7_THRESHOLD, alertsEnabled_MQ7, MQ7_BUZZER, MQ7_LED, alertState_MQ7, previousMillis_MQ7, "MQ7");

      // Handle MQ5
      handleAlert(mq5, MQ5_THRESHOLD, alertsEnabled_MQ5, MQ5_BUZZER, MQ5_LED, alertState_MQ5, previousMillis_MQ5, "MQ5");

    } else {
      Serial.println("⚠ Parsing Error: Check data format");
    }
  }
}

// --- Function to handle push button toggle ---
void handleButton(int buttonPin, bool &alertsEnabled, bool &lastButtonState, int statusLED) {
  bool buttonState = digitalRead(buttonPin);
  if (lastButtonState == HIGH && buttonState == LOW) {
    alertsEnabled = !alertsEnabled;
    digitalWrite(statusLED, alertsEnabled ? HIGH : LOW);
    Serial.println(alertsEnabled ? "Alerts ENABLED" : "Alerts DISABLED");
    delay(200); // debounce
  }
  lastButtonState = buttonState;
}

// --- Function to handle alert blinking ---
void handleAlert(int sensorValue, int threshold, bool alertsEnabled, int buzzerPin, int ledPin, bool &alertState, unsigned long &previousMillis, const char* sensorName) {
  if (alertsEnabled && sensorValue > threshold) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      alertState = !alertState;
      digitalWrite(buzzerPin, alertState ? HIGH : LOW);
      digitalWrite(ledPin, alertState ? HIGH : LOW);
    }
    Serial.print("⚠ ALERT: "); Serial.print(sensorName); Serial.println(" Threshold Exceeded!");
  } else {
    digitalWrite(buzzerPin, LOW);
    digitalWrite(ledPin, LOW);
  }
}
