#include <Servo.h>
const int PIN_MOTOR = 3, PIN_SERVO = 5;

Servo myservo;  // create servo object to control a servo
int pos = 0;    // variable to store the servo position

void setup() {
  // put your setup code here, to run once:
  
  pinMode(PIN_MOTOR, OUTPUT);

  myservo.attach(PIN_SERVO);  // attaches the servo on pin 9 to the servo object
  myservo.write(pos);
  delay(1000);
}

void loop() {
//  myservo.write(10);
//  delay(500);  
  for (pos = 0; pos <= 180; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    myservo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position
  }
  // put your main code here, to run repeatedly:
//  digitalWrite(PIN_MOTOR, HIGH);
//  digitalWrite(13, true);
//  delay(2000);
//  
//  digitalWrite(PIN_MOTOR, LOW);
//  digitalWrite(13, false);
//  delay(2000);
}
