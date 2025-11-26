#include <TM1637Display.h>

// === CONFIG ===
const int numRoads = 4;
const int baseGreen = 10;       // default seconds
const int minGreen = 10;
const int maxGreen = 40;
const int extraPool = 30;       // extra seconds distributed proportionally
const unsigned long samplingWindowMs = 3000; // sampling period to count vehicles
const int detectionThresholdCm = 50; // distance below which an object is considered present
const int yellowTime = 2;       // yellow duration seconds

// === PINS ===
// Traffic lights: for each road: {RED, YELLOW, GREEN}
const int lightPins[numRoads][3] = {
  {2, 3, 4},    // Road A
  {5, 6, 7},    // Road B
  {8, 9, 10},   // Road C
  {11, 12, 13}  // Road D
};

// Ultrasonic (Trig, Echo) per road
const int trigPins[numRoads] = {22, 24, 26, 28};
const int echoPins[numRoads] = {23, 25, 27, 29};

// TM1637 displays (CLK, DIO) per road
const int dispCLK[numRoads] = {30, 32, 34, 36};
const int dispDIO[numRoads] = {31, 33, 35, 37};
TM1637Display displays[numRoads] = {
  TM1637Display(dispCLK[0], dispDIO[0]),
  TM1637Display(dispCLK[1], dispDIO[1]),
  TM1637Display(dispCLK[2], dispDIO[2]),
  TM1637Display(dispCLK[3], dispDIO[3])
};

// === STATE ===
volatile int counts[numRoads];
bool objectPresent[numRoads];
int allocated[numRoads];

// Utility: read distance in cm (simple blocking read)
unsigned int readDistanceCM(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  unsigned long duration = pulseIn(echoPin, HIGH, 30000UL); // timeout 30 ms
  if (duration == 0) return 999; // no echo
  unsigned int cm = duration / 58;
  return cm;
}

void clearAllLights() {
  for (int i=0;i<numRoads;i++){
    digitalWrite(lightPins[i][0], HIGH); // RED off? depends on wiring; we'll use common-cathode active LOW? 
    // We'll explicitly set pattern when needed. For simplicity use HIGH=OFF, LOW=ON below.
  }
}

// Show a number on a TM1637 display, showing up to 4 digits
void showNumberTM(int idx, int val) {
  if (val < 0) val = 0;
  if (val > 9999) val = 9999;
  displays[idx].showNumberDec(val, false);
}

void setup() {
  // LEDs
  for (int i=0;i<numRoads;i++){
    for (int j=0;j<3;j++){
      pinMode(lightPins[i][j], OUTPUT);
      digitalWrite(lightPins[i][j], HIGH); // OFF (assuming active LOW LED wiring through resistor to 5V common)
    }
  }
  // Ultrasonic pins
  for (int i=0;i<numRoads;i++){
    pinMode(trigPins[i], OUTPUT);
    pinMode(echoPins[i], INPUT);
    digitalWrite(trigPins[i], LOW);
    counts[i] = 0;
    objectPresent[i] = false;
  }
  // Displays init
  for (int i=0;i<numRoads;i++){
    displays[i].setBrightness(6); // 0..7
    showNumberTM(i, baseGreen);
  }
  // Serial for debug
  Serial.begin(115200);
  delay(200);
}

// Count vehicles during sampling window by polling all sensors frequently
void sampleCounts() {
  unsigned long start = millis();
  for (int i=0;i<numRoads;i++) counts[i] = 0;
  // reset objectPresent states
  for (int i=0;i<numRoads;i++) objectPresent[i] = false;

  while (millis() - start < samplingWindowMs) {
    for (int i=0;i<numRoads;i++) {
      unsigned int d = readDistanceCM(trigPins[i], echoPins[i]);
      bool nowPresent = (d <= detectionThresholdCm);
      // detect rising edge (no object -> object)
      if (nowPresent && !objectPresent[i]) {
        counts[i]++;           // new object arrived
        objectPresent[i] = true;
      } else if (!nowPresent && objectPresent[i]) {
        // object left
        objectPresent[i] = false;
      }
      // tiny pause to avoid hammering sensors
      delay(60);
    }
  }
  // debug
  Serial.print("Counts: ");
  for (int i=0;i<numRoads;i++) {
    Serial.print(counts[i]);
    Serial.print(i < numRoads-1 ? "," : "\n");
  }
}

// Compute allocation proportionally to counts
void computeAllocation() {
  int total = 0;
  for (int i=0;i<numRoads;i++) total += counts[i];

  if (total == 0) {
    for (int i=0;i<numRoads;i++) allocated[i] = baseGreen;
    return;
  }
  // distribute extraPool proportionally
  int distributed = 0;
  for (int i=0;i<numRoads;i++) {
    float frac = (float)counts[i] / (float)total;
    int extra = (int)round(frac * extraPool);
    allocated[i] = baseGreen + extra;
    if (allocated[i] < minGreen) allocated[i] = minGreen;
    if (allocated[i] > maxGreen) allocated[i] = maxGreen;
    distributed += extra;
  }
  // small correction: if rounding caused mismatch, adjust first element
  // (keep it simple; not critical)
  // update displays with allocated values
  for (int i=0;i<numRoads;i++) {
    showNumberTM(i, allocated[i]);
  }
}

// Set lights for a road: red,yellow,green booleans (true = ON)
void setRoadLights(int roadIdx, bool redOn, bool yellowOn, bool greenOn) {
  // We wrote pins HIGH as OFF in setup; assume LOW = ON
  digitalWrite(lightPins[roadIdx][0], redOn ? LOW : HIGH);
  digitalWrite(lightPins[roadIdx][1], yellowOn ? LOW : HIGH);
  digitalWrite(lightPins[roadIdx][2], greenOn ? LOW : HIGH);
}

// Turn all roads red
void allRed() {
  for (int i=0;i<numRoads;i++) {
    setRoadLights(i, true, false, false);
  }
}

void loop() {
  // 1) Sample traffic counts
  sampleCounts();

  // 2) Compute allocated green times
  computeAllocation();

  // 3) Run traffic cycle in order
  for (int r=0; r<numRoads; r++) {
    int gtime = allocated[r];
    // set this road green, others red
    for (int i=0;i<numRoads;i++){
      if (i == r) setRoadLights(i, false, false, true);
      else setRoadLights(i, true, false, false);
    }

    // show countdown on the road's display, other displays show their allocated values
    for (int sec = gtime; sec >= 1; sec--) {
      showNumberTM(r, sec);
      // ensure other displays show allocated (static)
      for (int j=0;j<numRoads;j++){
        if (j != r) showNumberTM(j, allocated[j]);
      }
      delay(1000);
    }

    // set yellow for this road
    setRoadLights(r, false, true, false);
    showNumberTM(r, yellowTime);
    delay(yellowTime * 1000);

    // after yellow, set road red
    setRoadLights(r, true, false, false);
  }

  // After full cycle loop, it repeats: at top of loop we'll sample again
}
