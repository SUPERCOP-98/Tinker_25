#include <LiquidCrystal.h>

// Pin mapping: RS, EN, D4, D5, D6, D7
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

void setup() {
  //Initialise LCD with 16 columns and 2 rows
  lcd.begin(16, 2);

  // Print a message
  lcd.setCursor(0, 0);
  lcd.print("Hello, Arduino!");

  lcd.setCursor(0, 1);
  lcd.print("LCD Connected!");
}

void loop() {
  // Nothing here for now
}
