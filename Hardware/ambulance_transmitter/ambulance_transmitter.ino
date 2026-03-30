/*
 * ============================================================
 *  AMBULANCE RF TRANSMITTER — Arduino Uno (4 Button Version)
 * ============================================================
 *  One button per lane direction. Press the button matching
 *  whichever lane the ambulance is entering from.
 *
 *  Wiring:
 *    TX module DATA → Uno Pin 12
 *    TX module VCC  → Uno 5V
 *    TX module GND  → Uno GND
 *
 *    Button NORTH   → Pin 2  and GND
 *    Button EAST    → Pin 3  and GND
 *    Button SOUTH   → Pin 4  and GND
 *    Button WEST    → Pin 5  and GND
 *
 *  Library needed:
 *    Arduino IDE → Manage Libraries → search "RadioHead"
 *    Install "RadioHead" by Mike McCaulay
 * ============================================================
 */

#include <RH_ASK.h>
#include <SPI.h>

// RH_ASK(speed, rxPin, txPin, pttPin)
RH_ASK rf_driver(2000, 11, 12, 0);

#define BTN_N  2   // North button → Pin 2
#define BTN_E  3   // East  button → Pin 3
#define BTN_S  4   // South button → Pin 4
#define BTN_W  5   // West  button → Pin 5

void setup() {
  Serial.begin(9600);

  pinMode(BTN_N, INPUT_PULLUP);
  pinMode(BTN_E, INPUT_PULLUP);
  pinMode(BTN_S, INPUT_PULLUP);
  pinMode(BTN_W, INPUT_PULLUP);

  if (!rf_driver.init()) {
    Serial.println("RF init failed — check pin 12 wiring");
  } else {
    Serial.println("RF Transmitter ready — press a button to send emergency");
    Serial.println("  Pin 2 = NORTH  |  Pin 3 = EAST");
    Serial.println("  Pin 4 = SOUTH  |  Pin 5 = WEST");
  }
}

void sendEmergency(const char* laneCode) {
  Serial.print("Sending emergency: ");
  Serial.println(laneCode);

  // Send 5 times for reliability (433MHz is noisy)
  for (int i = 0; i < 5; i++) {
    rf_driver.send((uint8_t*)laneCode, strlen(laneCode));
    rf_driver.waitPacketSent();
    delay(80);
  }

  Serial.println("Sent.");
  delay(500);   // debounce — prevents re-sending while button held
}

void loop() {
  if (digitalRead(BTN_N) == LOW) sendEmergency("EMG_N");
  if (digitalRead(BTN_E) == LOW) sendEmergency("EMG_E");
  if (digitalRead(BTN_S) == LOW) sendEmergency("EMG_S");
  if (digitalRead(BTN_W) == LOW) sendEmergency("EMG_W");
}
