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

// Data reception status LED
#define STATUS_LED_PIN 14
#define STATUS_BLINK_MS 80

// Button states
bool alertsEnabled_MQ135 = true, alertsEnabled_MQ7 = true, alertsEnabled_MQ5 = true;
bool lastButtonState_MQ135 = HIGH, lastButtonState_MQ7 = HIGH, lastButtonState_MQ5 = HIGH;

// Timing
unsigned long statusLedOffTime = 0;
unsigned long prevMillis135 = 0, prevMillis7 = 0, prevMillis5 = 0;
int mq7PatternStep = 0;   // for MQ7 double-beep sequence

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  pinMode(MQ135_BUZZER, OUTPUT); pinMode(MQ135_LED, OUTPUT); pinMode(MQ135_STATUS, OUTPUT); pinMode(MQ135_BUTTON, INPUT_PULLUP);
  pinMode(MQ7_BUZZER, OUTPUT);   pinMode(MQ7_LED, OUTPUT);   pinMode(MQ7_STATUS, OUTPUT);   pinMode(MQ7_BUTTON, INPUT_PULLUP);
  pinMode(MQ5_BUZZER, OUTPUT);   pinMode(MQ5_LED, OUTPUT);   pinMode(MQ5_STATUS, OUTPUT);   pinMode(MQ5_BUTTON, INPUT_PULLUP);

  pinMode(STATUS_LED_PIN, OUTPUT);

  digitalWrite(MQ135_STATUS, HIGH);
  digitalWrite(MQ7_STATUS, HIGH);
  digitalWrite(MQ5_STATUS, HIGH);

  Serial.println("Receiver ready with distinct beep patterns...");
}

void loop() {
  // Handle buttons
  handleButton(MQ135_BUTTON, alertsEnabled_MQ135, lastButtonState_MQ135, MQ135_STATUS);
  handleButton(MQ7_BUTTON, alertsEnabled_MQ7, lastButtonState_MQ7, MQ7_STATUS);
  handleButton(MQ5_BUTTON, alertsEnabled_MQ5, lastButtonState_MQ5, MQ5_STATUS);

  // Status LED timeout
  if (statusLedOffTime != 0 && millis() > statusLedOffTime) {
    digitalWrite(STATUS_LED_PIN, LOW);
    statusLedOffTime = 0;
  }

  // UART data
  if (Serial2.available()) {
    String data = Serial2.readStringUntil('\n');
    data.trim();
    Serial.println("Received: " + data);

    // Blink status LED
    digitalWrite(STATUS_LED_PIN, HIGH);
    statusLedOffTime = millis() + STATUS_BLINK_MS;

    // Normalize for parsing
    data.replace(" ", "");  // thin space
    data.replace("₂", "2"); // subscript 2
    data.replace("°C", "");
    data.replace("%", "");

    float temp, hum, o2percent;
    int mq7, mq5, mq135, o2raw;

    int parsed = sscanf(data.c_str(),
                        "Temp:%f,Humidity:%f,MQ7:%d,MQ5:%d,MQ135:%d,O2Raw:%d,O2:%f",
                        &temp, &hum, &mq7, &mq5, &mq135, &o2raw, &o2percent);

    if (parsed == 7) {
      if (alertsEnabled_MQ135) patternMQ135(mq135);
      if (alertsEnabled_MQ7)   patternMQ7(mq7);
      if (alertsEnabled_MQ5)   patternMQ5(mq5);

      Serial.print("O2 Raw: "); Serial.print(o2raw);
      Serial.print(" | O2 %: "); Serial.println(o2percent);
    } else {
      Serial.println("⚠ Parsing Error!");
    }
  }
}

// Button toggle
void handleButton(int buttonPin, bool &alertsEnabled, bool &lastButtonState, int statusLED) {
  bool state = digitalRead(buttonPin);
  if (lastButtonStat
