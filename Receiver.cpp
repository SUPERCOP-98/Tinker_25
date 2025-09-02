#define RXD2 16
#define TXD2 17

#define MQ135_THRESHOLD 100

#define BUZZER_PIN 27
#define ALERT_LED  4
#define STATUS_LED 12
#define BUTTON_PIN 21

bool alertsEnabled = true;
bool lastButtonState = HIGH;

unsigned long previousMillis = 0;
const long interval = 500; // blink every 500ms
bool alertState = false;

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(ALERT_LED, OUTPUT);
  pinMode(STATUS_LED, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(ALERT_LED, LOW);
  digitalWrite(STATUS_LED, HIGH);

  Serial.println("UART Receiver with Blinking MQ135 Alerts Ready...");
}

void loop() {
  // --- Button handling (toggle on press) ---
  bool buttonState = digitalRead(BUTTON_PIN);
  if (lastButtonState == HIGH && buttonState == LOW) {
    alertsEnabled = !alertsEnabled;
    digitalWrite(STATUS_LED, alertsEnabled ? HIGH : LOW);
    Serial.println(alertsEnabled ? "MQ-135 ON!" : "MQ-135 OFF!");
    delay(200); // debounce
  }
  lastButtonState = buttonState;

  // --- UART data reading ---
  if (Serial2.available()) {
    String data = Serial2.readStringUntil('\n');
    Serial.println("Received: " + data);

    float temp, hum;
    int mq7, mq5, mq135;

    if (sscanf(data.c_str(), "Temp:%f,Humidity:%f,MQ7:%d,MQ5:%d,MQ135:%d",
               &temp, &hum, &mq7, &mq5, &mq135) == 5) {

      if (alertsEnabled && mq135 > MQ135_THRESHOLD) {
        // --- Blinking logic ---
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= interval) {
          previousMillis = currentMillis;
          alertState = !alertState;  // toggle
          digitalWrite(BUZZER_PIN, alertState ? HIGH : LOW);
          digitalWrite(ALERT_LED, alertState ? HIGH : LOW);
        }
        Serial.println("⚠ MQ135 Activated!");
      } else {
        digitalWrite(BUZZER_PIN, LOW);
        digitalWrite(ALERT_LED, LOW);
      }

    } else {
      Serial.println("⚠ Parsing Error: Check data format");
    }
  }
}
