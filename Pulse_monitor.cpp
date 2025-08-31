#define pulsePin A0

int signal;
int threshold = 550;
boolean pulseDetected = false;
unsigned long lastBeatTime = 0;
int BPM = 0;
int beatAvg = 0;

const int numReadings = 5;   // Average last 5 beats
int readings[numReadings];
int readIndex = 0;
int total = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("Pulse Meter with Averaging");
  for (int i = 0; i < numReadings; i++) {
    readings[i] = 0;
  }
}

void loop() {
  signal = analogRead(pulsePin);

  if (signal > threshold && !pulseDetected) {
    pulseDetected = true;
    unsigned long currentTime = millis();

    if (lastBeatTime > 0) {
      unsigned long beatInterval = currentTime - lastBeatTime;

      if (beatInterval > 300 && beatInterval < 2000) {
        BPM = 60000 / beatInterval;

        // Store BPM in moving average buffer
        total = total - readings[readIndex];
        readings[readIndex] = BPM;
        total = total + readings[readIndex];
        readIndex = (readIndex + 1) % numReadings;

        beatAvg = total / numReadings;

        Serial.print("BPM: ");
        Serial.print(BPM);
        Serial.print(" | Avg BPM: ");
        Serial.print(beatAvg);
        Serial.print(" | Signal: ");
        Serial.println(signal);
      }
    }
    lastBeatTime = currentTime;
  }

  if (signal < threshold) {
    pulseDetected = false;
  }

  delay(10);
}
