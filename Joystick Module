#define xPin A0
#define yPin A1
#define buttonPin 2

void setup() {
  pinMode(buttonPin, INPUT_PULLUP);
  Serial.begin(9600);
}

void loop() {
  int xVal = analogRead(xPin);
  int yVal = analogRead(yPin);
  int button = digitalRead(buttonPin);

  Serial.print("X: ");
  Serial.print(xVal);
  Serial.print(" | Y: ");
  Serial.print(yVal);
  Serial.print(" | Button: ");
  Serial.println(button == LOW ? "Pressed" : "Released");

  delay(300);
}
