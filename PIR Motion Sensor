#define pirPin 8

void setup() {
  pinMode(pirPin, INPUT);
  Serial.begin(9600);
}

void loop() {
  int motion = digitalRead(pirPin);
  if (motion == HIGH) {
    Serial.println("Motion Detected!");
  } else {
    Serial.println("No Motion");
  }
  delay(1000);
}
