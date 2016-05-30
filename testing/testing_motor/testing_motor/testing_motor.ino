const int PIN_POS = 5;
const int PIN_NEG = 6;

void setup() {
  Serial.begin(9600);
  // put your setup code here, to run once:
  Serial.println("started");

  pinMode(PIN_POS, OUTPUT);
  pinMode(PIN_NEG, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
}

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

void serialEvent() {
  Serial.println("Reading Serial Event");
  
  char instr[200];
  int i = 0;
  while (Serial.available()) {
    instr[i++] = (char) Serial.read();
  }
  instr[i] = '\0';

  Serial.println(instr);

  if (strcmp(instr, "front") == 0) turnFront();
  else if (strcmp(instr, "back") == 0) turnBack();
  else if (strcmp(instr, "off") == 0) turnOff();

  Serial.println("");
}

