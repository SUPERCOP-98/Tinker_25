#include <Servo.h>

Servo leg1; // Front Left
Servo leg2; // Front Right
Servo leg3; // Back Left
Servo leg4; // Back Right

void setup() {
  leg1.attach(3);
  leg2.attach(5);
  leg3.attach(6);
  leg4.attach(9);

  // Set all to neutral position
  leg1.write(90);
  leg2.write(90);
  leg3.write(90);
  leg4.write(90);
  delay(1000);
}

void loop() {
  // Step 1: Move Front Left + Back Right forward
  leg1.write(60); // Forward
  leg4.write(120); // Forward
  delay(400);

  // Step 2: Return to neutral
  leg1.write(90);
  leg4.write(90);
  delay(400);

  // Step 3: Move Front Right + Back Left forward
  leg2.write(120);
  leg3.write(60);
  delay(400);

  // Step 4: Return to neutral
  leg2.write(90);
  leg3.write(90);
  delay(400);
}
