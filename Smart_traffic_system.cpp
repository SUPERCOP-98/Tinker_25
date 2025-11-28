#include <TM1637Display.h>

// === CONFIG ===
const int numRoads = 4;
const int totalGreenPool = 60;  // Total seconds to distribute among all roads
const int minGreen = 5;         // Minimum green time for any road (even with 0 vehicles)
const int maxGreen = 40;        // Maximum green time for any road
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
int vehicleCount[numRoads];      // Total vehicles counted
int allocated[numRoads];         // Allocated green time
unsigned long lastCountCheck[numRoads]; // Last time we checked each sensor

// Measure distance in cm
int getDistance(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long duration = pulseIn(echo, HIGH, 20000);
  int distance = duration * 0.034 / 2;
  return distance;
}

// Detect object (returns true if object detected)
bool detectObject(int roadIndex) {
  int d = getDistance(trigPins[roadIndex], echoPins[roadIndex]);
  if (d > 2 && d < 7) {  // valid object in range
    return true;
  }
  return false;
}

// Check all sensors and update counts (non-blocking with delay protection)
void updateVehicleCounts() {
  for (int i = 0; i < numRoads; i++) {
    // Only check if enough time has passed (300ms debounce per sensor)
    if (millis() - lastCountCheck[i] > 300) {
      if (detectObject(i)) {
        vehicleCount[i]++;
        lastCountCheck[i] = millis();
        Serial.print("Road ");
        Serial.print(char('A' + i));
        Serial.print(": Vehicle #");
        Serial.print(vehicleCount[i]);
        Serial.println(" detected!");
      }
    }
  }
}

// Show a number on a TM1637 display
void showNumberTM(int idx, int val) {
  if (val < 0) val = 0;
  if (val > 9999) val = 9999;
  displays[idx].showNumberDec(val, false);
}

void setup() {
  // LEDs
  for (int i=0; i<numRoads; i++) {
    for (int j=0; j<3; j++) {
      pinMode(lightPins[i][j], OUTPUT);
      digitalWrite(lightPins[i][j], LOW);
    }
  }
  
  // Ultrasonic pins
  for (int i=0; i<numRoads; i++) {
    pinMode(trigPins[i], OUTPUT);
    pinMode(echoPins[i], INPUT);
    digitalWrite(trigPins[i], LOW);
    vehicleCount[i] = 0;
    lastCountCheck[i] = 0;
  }
  
  // Displays init
  for (int i=0; i<numRoads; i++) {
    displays[i].setBrightness(6);
    showNumberTM(i, 0);
  }
  
  // Serial for debug
  Serial.begin(9600);
  delay(1000);
  Serial.println("=== Dynamic Traffic System Started ===");
  Serial.println("Vehicle counting: Continuous on all roads");
  
  // Test RED lights
  delay(1000);
  Serial.println("Testing: All RED lights ON for 3 seconds...");
  allRed();
  delay(3000);
}

// Compute allocation proportionally to counts
void computeAllocation() {
  int totalVehicles = 0;
  for (int i=0; i<numRoads; i++) {
    totalVehicles += vehicleCount[i];
  }

  if (totalVehicles == 0) {
    // No vehicles detected, distribute equally
    for (int i=0; i<numRoads; i++) {
      allocated[i] = totalGreenPool / numRoads;
    }
  } else {
    // Distribute proportionally based on vehicle count
    for (int i=0; i<numRoads; i++) {
      if (vehicleCount[i] == 0) {
        allocated[i] = minGreen;
      } else {
        float proportion = (float)vehicleCount[i] / (float)totalVehicles;
        allocated[i] = (int)round(proportion * totalGreenPool);
        
        if (allocated[i] < minGreen) allocated[i] = minGreen;
        if (allocated[i] > maxGreen) allocated[i] = maxGreen;
      }
    }
  }
 
  // Debug output
  Serial.println("\n--- Time Allocation Based on Traffic Density ---");
  Serial.print("Total Vehicles Detected: ");
  Serial.println(totalVehicles);
  
  for (int i=0; i<numRoads; i++) {
    Serial.print("Road ");
    Serial.print(char('A' + i));
    Serial.print(": ");
    Serial.print(vehicleCount[i]);
    Serial.print(" vehicles");
    
    if (totalVehicles > 0) {
      float percentage = ((float)vehicleCount[i] / (float)totalVehicles) * 100.0;
      Serial.print(" (");
      Serial.print((int)percentage);
      Serial.print("%)");
    }
    
    Serial.print(" → ");
    Serial.print(allocated[i]);
    Serial.println(" seconds green");
  }
  Serial.println("-----------------------------------------------");
}

// Turn all roads red
void allRed() {
  for (int i=0; i<numRoads; i++) {
    digitalWrite(lightPins[i][0], HIGH);  // RED ON
    digitalWrite(lightPins[i][1], LOW);   // YELLOW OFF
    digitalWrite(lightPins[i][2], LOW);   // GREEN OFF
  }
}

void loop() {
  Serial.println("\n========== NEW TRAFFIC CYCLE ==========");
  
  // Compute allocation based on vehicle counts from previous cycle
  // (First cycle will use count of 0 for all, distributing time equally)
  computeAllocation();
  
  // Start with all roads RED
  Serial.println("All roads: RED - Continuous vehicle counting active...\n");
  allRed();
  delay(1000);
  
  // Run traffic cycle for each road
  for (int r=0; r<numRoads; r++) {
    int greenTime = allocated[r];
    
    Serial.println("\n----------------------------");
    Serial.print("Road ");
    Serial.print(char('A' + r));
    Serial.println("'s Turn:");
    
    Serial.print("Vehicle counts for NEXT cycle: ");
    for (int i=0; i<numRoads; i++) {
      Serial.print(char('A' + i));
      Serial.print("=");
      Serial.print(vehicleCount[i]);
      Serial.print(i < numRoads-1 ? ", " : "\n");
    }
    
    // Calculate wait times for each road
    int waitTimes[numRoads];
    for (int i=0; i<numRoads; i++) {
      if (i == r) {
        waitTimes[i] = greenTime; // Current road shows its green countdown
      } else if (i > r) {
        // Roads ahead in queue
        int wait = greenTime + yellowTime;
        for (int j=r+1; j<i; j++) {
          wait += allocated[j] + yellowTime;
        }
        waitTimes[i] = wait;
      } else {
        // Roads that already passed - wait for full cycle
        int wait = greenTime + yellowTime;
        for (int j=r+1; j<numRoads; j++) {
          wait += allocated[j] + yellowTime;
        }
        for (int j=0; j<i; j++) {
          wait += allocated[j] + yellowTime;
        }
        waitTimes[i] = wait;
      }
    }
    
    Serial.print("Wait times: ");
    for (int i=0; i<numRoads; i++) {
      Serial.print(char('A' + i));
      Serial.print("=");
      Serial.print(waitTimes[i]);
      Serial.print("s");
      Serial.print(i < numRoads-1 ? ", " : "\n");
    }
    
    // PHASE 1: YELLOW
    Serial.print("Road ");
    Serial.print(char('A' + r));
    Serial.println(": YELLOW (2s)");
    
    for (int i=0; i<numRoads; i++) {
      if (i == r) {
        digitalWrite(lightPins[i][0], LOW);
        digitalWrite(lightPins[i][1], HIGH);
        digitalWrite(lightPins[i][2], LOW);
      } else {
        digitalWrite(lightPins[i][0], HIGH);
        digitalWrite(lightPins[i][1], LOW);
        digitalWrite(lightPins[i][2], LOW);
      }
    }
    
    // Yellow phase with continuous counting
    unsigned long startTime = millis();
    while (millis() - startTime < yellowTime * 1000) {
      updateVehicleCounts();
      // Update displays every 100ms
      static unsigned long lastDisplayUpdate = 0;
      if (millis() - lastDisplayUpdate > 100) {
        lastDisplayUpdate = millis();
        for (int i=0; i<numRoads; i++) {
          showNumberTM(i, waitTimes[i]); // Show wait time
        }
      }
    }
    
    // Decrement wait times after yellow
    for (int i=0; i<numRoads; i++) {
      waitTimes[i] -= yellowTime;
      if (waitTimes[i] < 0) waitTimes[i] = 0;
    }
    
    // PHASE 2: GREEN
    Serial.print("Road ");
    Serial.print(char('A' + r));
    Serial.print(": GREEN (");
    Serial.print(greenTime);
    Serial.println("s) - Counting continues on all roads...");
    
    for (int i=0; i<numRoads; i++) {
      if (i == r) {
        digitalWrite(lightPins[i][0], LOW);
        digitalWrite(lightPins[i][1], LOW);
        digitalWrite(lightPins[i][2], HIGH);
      } else {
        digitalWrite(lightPins[i][0], HIGH);
        digitalWrite(lightPins[i][1], LOW);
        digitalWrite(lightPins[i][2], LOW);
      }
    }
    
    // Green phase with countdown timer and continuous counting
    for (int sec = greenTime; sec >= 1; sec--) {
      unsigned long secStart = millis();
      
      // Show countdown on green road, wait time on red roads
      for (int i=0; i<numRoads; i++) {
        showNumberTM(i, waitTimes[i]);
      }
      
      // Continue counting while showing timers
      while (millis() - secStart < 1000) {
        updateVehicleCounts();
        delay(50); // Small delay to avoid overwhelming the sensors
      }
      
      // Decrement all wait times
      for (int i=0; i<numRoads; i++) {
        waitTimes[i]--;
        if (waitTimes[i] < 0) waitTimes[i] = 0;
      }
    }
    
    // PHASE 3: Back to RED
    Serial.print("Road ");
    Serial.print(char('A' + r));
    Serial.println(": GREEN -> RED");
    
    allRed();
    delay(500);
  }
  
  Serial.println("\n========== CYCLE COMPLETE ==========");
  Serial.println("Vehicle counts will be used for NEXT cycle allocation");
  Serial.print("Current counts: ");
  for (int i=0; i<numRoads; i++) {
    Serial.print(char('A' + i));
    Serial.print("=");
    Serial.print(vehicleCount[i]);
    Serial.print(i < numRoads-1 ? ", " : "\n");
  }
}
