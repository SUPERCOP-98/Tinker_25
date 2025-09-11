#include <Servo.h>

Servo myservo;
int rainsensor = 2;   // digital output pin from sensor
int rainState = 0;

unsigned long lastRainTime = 0;
int rainDelay = 10000; // 10 seconds delay before uncovering clothes

void setup() {
  pinMode(rainsensor, INPUT);
  myservo.attach(9);
  myservo.write(0);   // Start open
  Serial.begin(9600);
}

void loop() {
  rainState = digitalRead(rainsensor);

  if (rainState == LOW) {   // Rain detected (LOW = wet)
    Serial.println("Rain detected!");
    myservo.write(90);   // Cover clothes
    lastRainTime = millis();  // Reset timer
  } else {
    // Only uncover after rain stops + delay
    if (millis() - lastRainTime > rainDelay) {
      Serial.println("No rain for 10s, uncovering...");
      myservo.write(0);  // Uncover clothes
    }
  }

  delay(200); // small delay for stability
}
