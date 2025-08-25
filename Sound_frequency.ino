// ====== Pin Declarations ======
#define MIC_PIN 2    // Microphone OUT pin connected to D2

// ====== Variables ======
volatile unsigned long pulseCount = 0;
unsigned long lastReport = 0;

// ====== Interrupt Service Routine ======
void onPulse() {
  pulseCount++;
}

void setup() {
  Serial.begin(115200);
  pinMode(MIC_PIN, INPUT);

  // Attach interrupt on MIC_PIN (count rising edges)
  attachInterrupt(digitalPinToInterrupt(MIC_PIN), onPulse, RISING);
}

void loop() {
  unsigned long now = millis();

  if (now - lastReport >= 1000) {   // Report every 1 second
    noInterrupts();                 // Temporarily disable interrupts
    unsigned long count = pulseCount;
    pulseCount = 0;                 // Reset for next interval
    interrupts();

    float freqHz = count;           // Pulses per second = Hz (approx)

    Serial.print("Frequency: ");
    Serial.print(freqHz);
    Serial.println(" Hz");

    lastReport = now;
  }
}
