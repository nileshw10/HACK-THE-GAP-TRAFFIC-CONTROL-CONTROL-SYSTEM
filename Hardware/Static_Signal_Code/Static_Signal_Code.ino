// ======================================
// 4-Way Static Traffic Signal
// Clean & Safe Version
// ======================================

// -------- Road 1 --------
const int R1_R = 2;
const int R1_Y = 3;
const int R1_G = 4;

// -------- Road 2 --------
const int R2_R = 5;
const int R2_Y = 6;
const int R2_G = 7;

// -------- Road 3 --------
const int R3_R = 8;
const int R3_Y = 9;
const int R3_G = 10;

// -------- Road 4 --------
const int R4_R = 11;
const int R4_Y = 12;
const int R4_G = 13;

// Timing (milliseconds)
const int GREEN_TIME  = 5000;
const int YELLOW_TIME = 1500;
const int ALL_RED_TIME = 500;

void setup() {
  for (int i = 2; i <= 13; i++) {
    pinMode(i, OUTPUT);
  }
  setAllRed();
}

void loop() {

  controlRoad(R1_R, R1_Y, R1_G);
  controlRoad(R2_R, R2_Y, R2_G);
  controlRoad(R3_R, R3_Y, R3_G);
  controlRoad(R4_R, R4_Y, R4_G);
}

// ======================================
// Function to Control One Road
// ======================================
void controlRoad(int redPin, int yellowPin, int greenPin) {

  setAllRed();
  delay(ALL_RED_TIME);   // Safety gap

  digitalWrite(redPin, LOW);   // Turn OFF red
  digitalWrite(greenPin, HIGH);
  delay(GREEN_TIME);

  digitalWrite(greenPin, LOW);
  digitalWrite(yellowPin, HIGH);
  delay(YELLOW_TIME);

  digitalWrite(yellowPin, LOW);
}

// ======================================
// Function to Set All Roads RED
// ======================================
void setAllRed() {

  digitalWrite(R1_R, HIGH);
  digitalWrite(R2_R, HIGH);
  digitalWrite(R3_R, HIGH);
  digitalWrite(R4_R, HIGH);

  digitalWrite(R1_Y, LOW);
  digitalWrite(R2_Y, LOW);
  digitalWrite(R3_Y, LOW);
  digitalWrite(R4_Y, LOW);

  digitalWrite(R1_G, LOW);
  digitalWrite(R2_G, LOW);
  digitalWrite(R3_G, LOW);
  digitalWrite(R4_G, LOW);
}