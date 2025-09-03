#include <LiquidCrystal.h>

// ====== LCD Pins ======
#define RS 12
#define EN 11
#define D4 5
#define D5 4
#define D6 3
#define D7 8   // moved to D8 to avoid conflict with sound sensor

LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

// ====== Sound Sensor Pin ======
#define MIC_PIN 2    // OUT pin of sound sensor connected to D2

// ====== Variables ======
volatile unsigned long pulseCount = 0;
unsigned long lastReport = 0;

// ====== Interrupt Service Routine ======
void onPulse() {
  pulseCount++;
}

void setup() {
  Serial.begin(115200);

  lcd.begin(16, 2);
  lcd.print("Sound Monitor");

  pinMode(MIC_PIN, INPUT);

  // MIC_PIN (to D2 / D3)
  attachInterrupt(digitalPinToInterrupt(MIC_PIN), onPulse, RISING);

  Serial.println("Sound Frequency Monitor Ready");
}

void loop() {
  unsigned long now = millis();

  if (now - lastReport >= 1000) {   //Update every 1 second
    noInterrupts();
    unsigned long count = pulseCount;
    pulseCount = 0;
    interrupts();

    float freqHz = count;   //Frequency (Hz)

    
    Serial.print("Sound Frequency: ");
    Serial.print(freqHz);
    Serial.println(" Hz");

    // Show on LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Frequency:");
    lcd.setCursor(0, 1);
    lcd.print(freqHz);
    lcd.print(" Hz");

    lastReport = now;
  }
}
