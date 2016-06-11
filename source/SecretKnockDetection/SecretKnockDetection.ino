#include "CurieIMU.h"

// Knock reading properties
const unsigned long KNOCK_LISTEN_TIMEOUT = 3000;   // 50000 ms = 5 seconds
//const unsigned long minKnockInterval = 150;              // 100 ms    // Might want to make this 150ms
//const unsigned long minKnockInterval = 180;              // 100 ms    // Might want to make this 150ms
const unsigned long minKnockInterval = 180;              // 100 ms    // Might want to make this 150ms
const unsigned int READ_KNOCK_THRESHOLD = 1000;
//const unsigned int READ_KNOCK_THRESHOLD = 900;
const unsigned int READ_KNOCK_DURATION = 1100;

// Saved Knocks
const unsigned long MAX_KNOCKS = 30;
volatile unsigned long savedKnocksIntervals[MAX_KNOCKS];
volatile unsigned int savedKnocksLen = 0;

// Reading Knocks
volatile unsigned long currKnockIntervals[MAX_KNOCKS];
volatile unsigned int currKnockIndex = 0;                // Used to denote current position in currKnockIntervals and the size of the array after population

// Mapping constants
const unsigned long RANGE_LOW = 0;
const unsigned long RANGE_HIGH = 100;
//const unsigned long MAX_PERCENT_TOLERANCE = 35;        // Percent difference allowed for valid knock checking. TODO: may need tweaking
const unsigned long MAX_PERCENT_TOLERANCE = 45;        // Percent difference allowed for valid knock checking. TODO: may need tweaking
const unsigned long AVE_PERCENT_TOlERANCE = 25;        // Average percent difference allowed for valid knock checking. TODO: may need tweaking

// Pins
const unsigned int PIN_ONBOARD_LED = 13;
const unsigned int PIN_BUTTON = 13;
const unsigned int PIN_POS = 6;
const unsigned int PIN_NEG = 5;
const unsigned int PIN_LED_GREEN = 11;
const unsigned int PIN_LED_RED = 12;

// Motor
//const unsigned long MOTOR_RUN_TIME = 3000;
const unsigned long MOTOR_RUN_TIME = 14000;
const unsigned long MOTOR_OFF_TIME = 2000;

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
  //  CurieIMU.setDetectionThreshold(CURIE_IMU_SHOCK, 990);  // 10 mg
  //  CurieIMU.setDetectionDuration(CURIE_IMU_SHOCK, 900);  // 200 ms
  CurieIMU.setDetectionThreshold(CURIE_IMU_SHOCK, READ_KNOCK_THRESHOLD);  // 10 mg
  CurieIMU.setDetectionDuration(CURIE_IMU_SHOCK, READ_KNOCK_DURATION);  // 200 ms
  CurieIMU.interrupts(CURIE_IMU_SHOCK);

  // Set up pins
  pinMode(PIN_ONBOARD_LED, OUTPUT);
  pinMode(PIN_POS, OUTPUT);
  pinMode(PIN_NEG, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_BUTTON, INPUT);

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

void handleSuccess() {
  Serial.println("Success");
  digitalWrite(PIN_ONBOARD_LED, HIGH);
  digitalWrite(PIN_LED_GREEN, HIGH);
  turnFront();
  delay(MOTOR_RUN_TIME);
  turnOff();
  delay(MOTOR_OFF_TIME);
  turnBack();
  delay(MOTOR_RUN_TIME);
  turnOff();
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_ONBOARD_LED, LOW);
}

void handleFailure() {
  Serial.println("Failure");
  digitalWrite(PIN_ONBOARD_LED, HIGH);
  digitalWrite(PIN_LED_RED, HIGH);
  delay(MOTOR_OFF_TIME);
  digitalWrite(PIN_LED_RED, LOW);
  digitalWrite(PIN_ONBOARD_LED, LOW);
}

void handleSave() {
  Serial.println("Saved");
  unsigned int i;
  digitalWrite(PIN_LED_GREEN, LOW);
  for (i = 0; i < 4; ++i) {
    digitalWrite(PIN_LED_GREEN, HIGH);
    digitalWrite(PIN_ONBOARD_LED, HIGH);
    delay(800);
    digitalWrite(PIN_ONBOARD_LED, LOW);
    digitalWrite(PIN_LED_GREEN, LOW);
  }
  digitalWrite(PIN_LED_GREEN, LOW);
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
  if (digitalRead(PIN_BUTTON) == HIGH) {
    saveKnocks();
    handleSave();
  } else {
    if (checkKnockPattern() == true) {
      handleSuccess();
    }
    else {
      handleFailure();
    }
  }

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

    //    Serial.print("timenow (");
    //    Serial.print(timenow);
    //    Serial.print(") - prevknock (");
    //    Serial.print(prevknock);
    //    Serial.print(") = timediff (");
    //    Serial.print(timediff);
    //    Serial.println(")");

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

  //  if (!(timediff < KNOCK_LISTEN_TIMEOUT)) {
  //    Serial.print("ENDED: final timediff = ");
  //    Serial.println(timediff);
  //  } else if (!(currKnockIndex < MAX_KNOCKS)) {
  //  }

  // Analyze knock
  analyzeKnock();
}

static void readKnock(void) {
  return;
}


/** Motor Control **/
void turnOff() {
  Serial.println("Turning off");
  Serial.println("");
  digitalWrite(PIN_POS, LOW);
  digitalWrite(PIN_NEG, LOW);
}

void turnFront() {
  turnOff();
  Serial.println("Turning front");
  Serial.println("");
  digitalWrite(PIN_POS, HIGH);
  digitalWrite(PIN_NEG, LOW);
}

void turnBack() {
  turnOff();
  Serial.println("Turning back");
  Serial.println("");
  digitalWrite(PIN_POS, LOW);
  digitalWrite(PIN_NEG, HIGH);
}
/** End Motor Control **/


