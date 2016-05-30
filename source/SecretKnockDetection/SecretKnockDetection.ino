#include "CurieIMU.h"
//#include <util/atomic.h>

// Knock reading properties
const unsigned long KNOCK_LISTEN_TIMEOUT = 3000;   // 50000 ms = 5 seconds
const unsigned long minKnockInterval = 150;              // 100 ms    // Might want to make this 150ms
volatile unsigned long prevKnockTime = 0;

// Saved Knocks
const unsigned long MAX_KNOCKS = 5;
volatile unsigned long savedKnocksIntervals[MAX_KNOCKS];
volatile unsigned int savedKnocksLen = 0;

// Reading Knocks
volatile unsigned long currKnockIntervals[MAX_KNOCKS];
volatile unsigned int currKnockIndex = 0;                // Used to denote current position in currKnockIntervals and the size of the array after population

// Mapping constants
const unsigned long RANGE_LOW = 0;
const unsigned long RANGE_HIGH = 100;
const unsigned long MAX_PERCENT_TOLERANCE = 25;        // Percent difference allowed for valid knock checking. TODO: may need tweaking
const unsigned long AVE_PERCENT_TOlERANCE = 25;        // Average percent difference allowed for valid knock checking. TODO: may need tweaking

// States
const byte STATE_IDLE = 0, STATE_LISTEN = 1, STATE_ANALYZE = 2;
volatile byte currState = STATE_IDLE;

// LED pins
const unsigned int PIN_ONBOARD_LED = 13;

// Debugging
const boolean DEBUG = true;
boolean DEBUG_SAVE_KNOCK = true;

void printKnocks (volatile unsigned long knocks[], volatile unsigned int len) {
  if (!DEBUG) return;
  // Testing: printing array of times
  Serial.print("[");
  unsigned int i;
  for (i = 0; i < len; i++) {
    Serial.print(knocks[i]);
    if (i < len - 1) Serial.print(", ");
  }
  Serial.println("]");
}

// End Debugging

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while (!Serial);

  CurieIMU.begin();
  CurieIMU.attachInterrupt(readKnock);
  //  CurieIMU.setDetectionThreshold(CURIE_IMU_SHOCK, 1000);  // 10 mg
  //  CurieIMU.setDetectionDuration(CURIE_IMU_SHOCK, 1000);  // 200 ms
  CurieIMU.setDetectionThreshold(CURIE_IMU_SHOCK, 990);  // 10 mg
  CurieIMU.setDetectionDuration(CURIE_IMU_SHOCK, 900);  // 200 ms
  CurieIMU.interrupts(CURIE_IMU_SHOCK);

  // Set up pins
  pinMode(PIN_ONBOARD_LED, OUTPUT);

  Serial.println("Started");
}

void preProcessKnocks() {
  /** Preprocess knocks by mapping interval values to an arbitrary range (RANGE_LOW to RANGE_HIGH) **/

  unsigned int i;
  unsigned long currInter;

  //  Find the lowest and highest interval values
  unsigned long low = KNOCK_LISTEN_TIMEOUT;       // This is the maximum possible interval value
  unsigned long high = 0;                         // This is the lowest possible interval value

  for (i = 0; i < currKnockIndex; ++i) {
    currInter = currKnockIntervals[i];
    if (currInter < low) low = currInter;
    if (currInter > high) high = currInter;
  }

  // Map values from RANGE_LOW to RANGE_HIGH
  for (i = 0; i < currKnockIndex; ++i) {
    currKnockIntervals[i] = map(currKnockIntervals[i], low, high, RANGE_LOW, RANGE_HIGH);
  }
}

void saveKnocks() {
  Serial.println("Saving knocks");
  savedKnocksLen = currKnockIndex;
  unsigned int i = 0;
  for (i = 0; i < savedKnocksLen; ++i) {
    savedKnocksIntervals[i] = currKnockIntervals[i];
  }
  //  memcpy(savedKnocksIntervals, currKnockIntervals, currKnockIndex);
}

boolean checkKnockPattern() {

  // First check if the number of knocks are the same
  if (currKnockIndex != savedKnocksLen) {
    Serial.println("Failing because lengths don't match");
    return false;
  }

  Serial.println("SAVED: Knocks saved");
  printKnocks(savedKnocksIntervals, savedKnocksLen);
  Serial.println("Knocks Read");
  printKnocks(currKnockIntervals, currKnockIndex);
  Serial.println("");

  unsigned int i;
  unsigned long percDiff;
  unsigned long totalDiff = 0;
  for (i = 0; i < currKnockIndex; ++i) {
    percDiff = (unsigned long) abs((long)savedKnocksIntervals[i] - (long)currKnockIntervals[i]);
    if (percDiff > MAX_PERCENT_TOLERANCE) {
      Serial.print("FAILED: ");
      Serial.print(percDiff);
      Serial.print(" out of tolerance at index ");
      Serial.print(i);
      Serial.print(" with saved ");
      Serial.print(savedKnocksIntervals[i]);
      Serial.print(" and read ");
      Serial.println(currKnockIntervals[i]);
      Serial.println();
      return false;
    }
    totalDiff += percDiff;
  }

  // Check if the total average time differences are acceptable
  if ((totalDiff / currKnockIndex) > AVE_PERCENT_TOlERANCE) {
    Serial.print("FAILED: ");
    Serial.print(totalDiff);
    Serial.println(" out of AVERAGE tolerance.");
    Serial.println();
    return false;
  }

  // Otherwise, we're good!
  return true;
}

void showSuccess() {
  Serial.println("Success");
  digitalWrite(PIN_ONBOARD_LED, HIGH);
  delay(1000);
  digitalWrite(PIN_ONBOARD_LED, LOW);
}

void showFailure() {
  Serial.println("Failure");
  int i = 0;
  for (i = 0; i < 10; ++i) {
    digitalWrite(PIN_ONBOARD_LED, HIGH);
    delay(200);
    digitalWrite(PIN_ONBOARD_LED, LOW);
  }
}

void unlockDoor() {
  Serial.println("Unlocking door");
}

void analyzeKnock() {
  noInterrupts();
  Serial.println("Analyzing knock");

  Serial.println("RAW: Knocks Read");
  printKnocks(currKnockIntervals, currKnockIndex);
  Serial.println("");

  // Pre process knocks to apply appropriate interval
  preProcessKnocks();

  Serial.println("PROCESSED: Knocks Read");
  printKnocks(currKnockIntervals, currKnockIndex);
  Serial.println("");

  // Check if we were saving a new knock
  if (DEBUG_SAVE_KNOCK) {
    saveKnocks();
    DEBUG_SAVE_KNOCK = false;
  } else {
    if (checkKnockPattern() == true) {
      showSuccess();
      unlockDoor();
    }
    else {
      showFailure();
    }
  }

  currState = STATE_IDLE;

  interrupts();
}

boolean knockDetected() {
  return (CurieIMU.shockDetected(X_AXIS, POSITIVE) ||
          CurieIMU.shockDetected(X_AXIS, NEGATIVE) ||
          CurieIMU.shockDetected(Y_AXIS, POSITIVE) ||
          CurieIMU.shockDetected(Y_AXIS, NEGATIVE) ||
          CurieIMU.shockDetected(Z_AXIS, POSITIVE) ||
          CurieIMU.shockDetected(Z_AXIS, NEGATIVE));
}

void loop() {
  // Only continue if a shock was received
  if (!knockDetected())
    return;

  // Set up timing characteristics
  unsigned long timenow = millis();
  unsigned long prevknock = timenow;
  unsigned long timediff = 0;
  Serial.print("Shock loop received at time: ");
  Serial.println(timenow);

  // Reset current knocks saved
  currKnockIndex = 0;

  // Debounce and wait
  delay(minKnockInterval);

  while ((timediff < KNOCK_LISTEN_TIMEOUT) && (currKnockIndex < MAX_KNOCKS)) {
    timenow = millis();
    timediff = timenow - prevknock;

    if (knockDetected()) {
      Serial.print("Knock detected at time: ");
      Serial.println(timenow);

      currKnockIntervals[currKnockIndex] = timediff;
      currKnockIndex++;
      prevknock = timenow;

      // Debounce and wait
      delay(minKnockInterval);
    }
  }

  // Analyze knock
  analyzeKnock();
}

static void readKnock(void) {
  return;
}


