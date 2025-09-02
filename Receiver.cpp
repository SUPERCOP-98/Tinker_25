#define RXD2 16
#define TXD2 17

// Threshold
#define MQ135_THRESHOLD 100

// Outputs
#define BUZZER_PIN 27
#define ALERT_LED  4
#define STATUS_LED 12

// Push button input
#define BUTTON_PIN 21

bool alertsEnabled = true;   // Start with alerts enabled
bool lastButtonState = HIGH; // For debouncing

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(ALERT_LED, OUTPUT);
  pinMode(STATUS_LED, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // Push button with pull-up

  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(ALERT_LED, LOW);
  digitalWrite(STATUS_LED, HIGH);  // Start enabled

  Serial.println("UART Receiver with MQ135 Threshold + Button Control Ready...");
}

void loop() {
  // ---- Button handling (toggle on press) ----
  bool buttonState = digitalRead(BUTTON_PIN);
  if (lastButtonState == HIGH && buttonState == LOW) {
    alertsEnabled = !alertsEnabled;  // Toggle state
    digitalWrite(STATUS_LED, alertsEnabled ? HIGH : LOW);
    Serial.println(alertsEnabled ? "Alerts ENABLED" : "Alerts DISABLED");
    delay(200); // Simple debounce
  }
  lastButtonState = buttonState;

  // ---- UART data reading ----
  if (Serial2.available()) {
    String data = Serial2.readStringUntil('\n');
    Serial.println("Received: " + data);

    float temp, hum;
    int mq7, mq5, mq135;

    if (sscanf(data.c_str(), "Temp:%f,Humidity:%f,MQ7:%d,MQ5:%d,MQ135:%d",
               &temp, &hum, &mq7, &mq5, &mq135) == 5) {

      if (alertsEnabled && mq135 > MQ135_THRESHOLD) {
        Serial.println("⚠ ALERT: MQ135 Threshold Exceeded!");
        digitalWrite(BUZZER_PIN, HIGH);
        digitalWrite(ALERT_LED, HIGH);
      } else {
        digitalWrite(BUZZER_PIN, LOW);
        digitalWrite(ALERT_LED, LOW);
      }

    } else {
      Serial.println("⚠ Parsing Error: Check data format");
    }
  }
}
