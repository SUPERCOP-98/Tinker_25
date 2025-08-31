#define irSensor 7

void setup() {
  pinMode(irSensor, INPUT);
  Serial.begin(9600);
}

void loop() {
  int state = digitalRead(irSensor);
  if (state == LOW) {
    Serial.println("Object Detected!");
  } else {
    Serial.println("No Object");
  }
  delay(500);
}
