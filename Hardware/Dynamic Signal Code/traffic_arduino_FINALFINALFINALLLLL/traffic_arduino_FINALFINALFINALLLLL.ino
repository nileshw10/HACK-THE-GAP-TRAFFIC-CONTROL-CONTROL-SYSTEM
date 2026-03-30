/*
 * ╔══════════════════════════════════════════════════════════════════╗
 * ║        SMART TRAFFIC MANAGEMENT SYSTEM — Arduino Mega            ║
 * ╠══════════════════════════════════════════════════════════════════╣
 * ║                                                                  ║
 * ║  FULL WIRING CONNECTIONS:                                        ║
 * ║  ─────────────────────────────────────────────────────────────   ║
 * ║  TRAFFIC LIGHTS (connect LED + 220Ω resistor to each pin):       ║
 * ║    NORTH lane E : RED=Pin22   YELLOW=Pin23   GREEN=Pin24   y=g r=y  g=o gr=b      ║
 * ║    EAST  lane F : RED=Pin28   YELLOW=Pin29   GREEN=Pin30   y=o r=r g=y gr=br      ║
 * ║    SOUTH lane G : RED=Pin25   YELLOW=Pin26   GREEN=Pin27   y=b r=v  g=g gr=w      ║
 * ║    WEST  lane H : RED=Pin31   YELLOW=Pin32   GREEN=Pin33   y=o r=br g=y gr=r      ║
 * ║    All LED other legs → GND                                      ║
 * ║                                                                  ║
 * ║  BUZZER (active buzzer):                                         ║
 * ║    + leg → Pin 13                                                ║
 * ║    - leg → GND                                                   ║
 * ║                                                                  ║
 * ║  RF RECEIVER (XY-MK-5V / 1820 module):                           ║
 * ║    VCC  → Mega 5V                                                ║
 * ║    GND  → Mega GND                                               ║
 * ║    DATA → Mega Pin 11                                            ║
 * ║    ANT  → 17cm wire soldered for better range (optional)         ║ 
 * ║                                                                  ║
 * ║  MODE SWITCH (toggle switch):                                    ║
 * ║    One leg  → Mega Pin 10                                        ║
 * ║    Other leg → GND                                               ║
 * ║    Switch OPEN  (floating) → Pin reads HIGH → DYNAMIC mode       ║
 * ║    Switch CLOSED (to GND)  → Pin reads LOW  → FIXED TIMER mode   ║
 * ║    Note: Pin 10 uses INPUT_PULLUP internally, no resistor needed ║
 * ║                                                                  ║
 * ║  LCD (16x2 I2C module — small blue board on back of screen):     ║
 * ║    VCC → Mega 5V                                                 ║
 * ║    GND → Mega GND                                                ║
 * ║    SDA → Mega Pin 20   PURPLE                                    ║
 * ║    SCL → Mega Pin 21   WHITE                                     ║
 * ║    Note: if screen is blank after upload, change 0x27 to 0x3F    ║
 * ║          in the lcd(0x27, 16, 2) line below                      ║
 * ║                                                                  ║
 * ║  PC (Python + YOLO) → Mega via USB cable (same cable as upload)  ║
 * ║    Python sends commands at 9600 baud automatically              ║
 * ║                                                                  ║
 * ╠══════════════════════════════════════════════════════════════════╣
 * ║                                                                  ║
 * ║  SIGNAL LOGIC — CLOCKWISE, DENSITY-BASED GREEN TIME:             ║
 * ║  ─────────────────────────────────────────────────────────────   ║
 * ║  Lanes always cycle in clockwise order, every lane gets a turn:  ║
 * ║    North(0) → East(1) → South(2) → West(3) → North → repeat      ║
 * ║                                                                  ║
 * ║  Green time is calculated from YOLO vehicle count per lane:      ║
 * ║    greenTime = 5 + (vehicleCount × 6 seconds)                    ║
 * ║    Minimum = 5s  (empty lane — moves on quickly)                 ║
 * ║    Maximum = 60s (very busy lane — capped so others don't wait)  ║
 * ║                                                                  ║
 * ║  Example with real counts:                                       ║
 * ║    North: 1 vehicle  → 5 + (1×6) = 11s green                     ║
 * ║    East:  5 vehicles → 5 + (5×6) = 35s green                     ║
 * ║    South: 0 vehicles → 5s minimum green (empty lane)             ║
 * ║    West:  3 vehicles → 5 + (3×6) = 23s green                     ║
 * ║    Full cycle = 11+35+5+23 = 74 seconds then repeats             ║
 * ║                                                                  ║
 * ╠══════════════════════════════════════════════════════════════════╣
 * ║                                                                  ║
 * ║  TWO MODES — controlled by physical toggle switch on Pin 10:     ║
 * ║  ─────────────────────────────────────────────────────────────   ║
 * ║  DYNAMIC MODE (switch open):                                     ║
 * ║    Green time varies per lane based on YOLO vehicle count.       ║
 * ║    Busy lanes get more time, empty lanes get minimum time.       ║
 * ║                                                                  ║
 * ║  FIXED TIMER MODE (switch closed to GND):                        ║
 * ║    Every lane gets exactly 20 seconds green regardless of count. ║
 * ║    Use if cameras fail, WiFi drops, or any technical problem.    ║
 * ║                                                                  ║
 * ║  PYTHON WATCHDOG (automatic safety):                             ║
 * ║    If Python stops sending data for 30 seconds (PC crash,        ║
 * ║    WiFi failure, etc.) → Arduino auto-switches to FIXED mode.    ║
 * ║    LCD shows "Python offline". When Python reconnects and sends  ║
 * ║    a COUNT command → automatically reverts to DYNAMIC.           ║
 * ║                                                                  ║
 * ╠══════════════════════════════════════════════════════════════════╣
 * ║                                                                  ║
 * ║  EMERGENCY VEHICLE DETECTION — 2 methods, same result:           ║
 * ║  ─────────────────────────────────────────────────────────────   ║
 * ║  METHOD 1 — RF Button (Arduino Uno in ambulance):                ║
 * ║    Ambulance driver presses one of 4 buttons on Uno.             ║
 * ║    Uno sends "EMG_N/E/S/W" over 433MHz.                          ║
 * ║    Mega's RF receiver on Pin 11 picks it up instantly.           ║
 * ║    RF DEDUP: 433MHz re-broadcasts same message 2-3 times.        ║
 * ║    Duplicates arriving within 2 seconds are ignored.             ║
 * ║                                                                  ║
 * ║  METHOD 2 — YOLO Camera (automatic backup):                      ║
 * ║    Python detects ambulance in camera feed.                      ║
 * ║    Sends <EMERGENCY:N> over USB serial to Mega.                  ║
 * ║    Note: standard yolov8n.pt cannot detect ambulances            ║
 * ║    (not a COCO class). RF button is the primary method.          ║
 * ║                                                                  ║
 * ║  WHAT HAPPENS WHEN EMERGENCY TRIGGERS:                          ║
 * ║    1. Buzzer beeps 3 times (alert)                              ║
 * ║    2. All 4 lanes go RED immediately                            ║
 * ║    3. 300ms safety gap (all red)                                ║
 * ║    4. Ambulance lane goes GREEN for 30 seconds                  ║
 * ║    5. After 30s → all RED → clockwise cycle resumes             ║
 * ║       from the NEXT lane (ambulance lane doesn't repeat)        ║
 * ║    Works in both DYNAMIC and FIXED TIMER modes.                 ║
 * ║                                                                 ║
 * ╠═════════════════════════════════════════════════════════════════╣
 * ║                                                                 ║
 * ║  WHAT PYTHON SENDS OVER USB SERIAL (9600 baud):                 ║
 * ║    All commands are framed between < and > characters.          ║
 * ║    <COUNT:N:3>   → North lane currently has 3 vehicles          ║
 * ║    <COUNT:E:8>   → East  lane currently has 8 vehicles          ║
 * ║    <COUNT:S:0>   → South lane currently has 0 vehicles          ║
 * ║    <COUNT:W:5>   → West  lane currently has 5 vehicles          ║
 * ║    <DENSITY:16>  → total vehicles all lanes (for LCD display)   ║
 * ║    <EMERGENCY:N> → ambulance detected on North lane (YOLO)      ║
 * ║    Python sends COUNT for all 4 lanes every 2 seconds.          ║
 * ║                                                                 ║
 * ║  LCD AT SECONDARY INTERSECTION SHOWS:                           ║
 * ║    0-4  vehicles total → "Traffic:    LOW "  → safe to go       ║
 * ║    5-9  vehicles total → "Traffic:MODERATE"  → might be slow    ║
 * ║    10+  vehicles total → "Traffic:HIGH !! "  → use other route  ║
 * ║    Emergency active    → "!! EMERGENCY !!"   → avoid main road  ║
 * ║    Python offline      → "Python offline  "  → fixed timer on   ║
 * ║                                                                 ║
 * ╠═════════════════════════════════════════════════════════════════╣
 * ║  LIBRARIES — install via Sketch → Manage Libraries:             ║
 * ║    • RadioHead         by Mike McCaulay  (for RF 433MHz)        ║
 * ║    • LiquidCrystal I2C by Frank de Brabander (for LCD)          ║
 * ╚═════════════════════════════════════════════════════════════════╝
 */

#include <RH_ASK.h>          // 433MHz RF library (RadioHead)
#include <SPI.h>             // Required by RadioHead internally
#include <Wire.h>            // I2C communication for LCD
#include <LiquidCrystal_I2C.h>  // LCD library

// ── LCD setup ─────────────────────────────────────────────────
// 0x27 is the I2C address of most LCD modules.
// If screen stays blank after upload → change 0x27 to 0x3F
// 16 = number of columns, 2 = number of rows
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ── RF Receiver setup ──────────────────────────────────────────
// RH_ASK(speed, rxPin, txPin, pttPin)
// speed=2000 bps, rxPin=11 (DATA wire from XY-MK-5V), txPin=12 unused
// The Mega ONLY receives — txPin=12 is declared but not connected
RH_ASK rf_driver(2000, 11, 12, 0);

// ── Traffic Light Pins ─────────────────────────────────────────
// Each lane has 3 pins: RED, YELLOW, GREEN
// Connect each pin → 220Ω resistor → LED anode → LED cathode → GND
const int RED_N=22, YEL_N=23, GRN_N=24;  // NORTH lane signals
const int RED_S=25, YEL_S=26, GRN_S=27;  // SOUTH lane signals
const int RED_E=28, YEL_E=29, GRN_E=30;  // EAST  lane signals
const int RED_W=31, YEL_W=32, GRN_W=33;  // WEST  lane signals

// ── Buzzer ────────────────────────────────────────────────────
// Active buzzer: + leg → Pin 13, - leg → GND
// Beeps 3 times when emergency vehicle detected
const int BUZZER = 13;

// ── Mode Switch ────────────────────────────────────────────────
// Toggle switch: one leg → Pin 10, other leg → GND
// Pin 10 is configured INPUT_PULLUP (internal pull-up resistor)
// This means: switch OPEN (floating) → Pin reads HIGH → DYNAMIC
//             switch CLOSED (to GND) → Pin reads LOW  → FIXED
// No external resistor needed — INPUT_PULLUP handles it
#define MODE_PIN 10

// ── Lane Arrays — stored in CLOCKWISE order ───────────────────
// Each array holds {RED_pin, YELLOW_pin, GREEN_pin} for that lane
// Array index matches clockwise position: 0=N, 1=E, 2=S, 3=W
int laneN[] = {RED_N, YEL_N, GRN_N};
int laneE[] = {RED_E, YEL_E, GRN_E};
int laneS[] = {RED_S, YEL_S, GRN_S};
int laneW[] = {RED_W, YEL_W, GRN_W};

// lanes[] is a pointer array so we can access any lane by index
// lanes[0] = North, lanes[1] = East, lanes[2] = South, lanes[3] = West
int*       lanes[]     = {laneN, laneE, laneS, laneW};
const char laneChars[] = {'N', 'E', 'S', 'W'};  // for serial printing
const int  NUM_LANES   = 4;

// Indexes into each lane's pin array
#define RI 0   // RED   is at index 0 in laneX[]
#define YI 1   // YELLOW is at index 1
#define GI 2   // GREEN  is at index 2

// ── Timing settings — adjust these to tune the system ─────────
#define MIN_GREEN        5        // seconds — minimum green time (dynamic mode)
                                  // Even an empty lane gets 5s so cycle moves fast
#define MAX_GREEN        60       // seconds — maximum green time cap (dynamic mode)
                                  // Prevents one jammed lane from holding for too long
#define SECS_PER_VEHICLE 6        // seconds of extra green per vehicle detected
                                  // 1 vehicle=11s, 3 vehicles=23s, 5 vehicles=35s
#define FIXED_GREEN      20       // seconds — green time for every lane in fixed mode
#define YELLOW_TIME      3        // seconds — yellow phase between green and red
#define ALLRED_GAP       500      // ms — brief all-red gap between lane transitions
                                  // Safety gap so no two lanes are ever green together
#define EMERGENCY_HOLD   30000UL  // ms — how long ambulance lane stays green (30s)

// ── Python Watchdog settings ───────────────────────────────────
// If Python stops sending COUNT commands for this long → auto FIXED mode
#define PYTHON_TIMEOUT   30000UL  // 30 seconds
unsigned long lastPythonContact = 0;   // timestamp of last COUNT received
bool          pythonConnected   = false; // false until first COUNT arrives
bool          watchdogFixed     = false; // true when watchdog has forced FIXED mode

// ── Vehicle Counts ─────────────────────────────────────────────
// Updated every 2 seconds by Python YOLO detection
// [0]=North, [1]=East, [2]=South, [3]=West
int vehicleCount[4] = {0, 0, 0, 0};

// ── Signal State ───────────────────────────────────────────────
int  currentLane = 0;      // current clockwise position (starts at North)
bool dynamicMode = true;   // true=DYNAMIC, false=FIXED (from switch reading)
bool lastEffMode = true;   // previous effective mode — used to detect changes

// ── Serial Parsing ─────────────────────────────────────────────
// Commands arrive as <VERB:ARG1:ARG2> — we read char by char
// and assemble between < and > into rxBuf, then parse
String rxBuf    = "";      // buffer being assembled
bool   inPacket = false;   // true when we've seen '<' but not '>' yet

// ── Emergency State ────────────────────────────────────────────
bool          emergencyActive = false;  // true while ambulance lane is green
unsigned long emergencyStart  = 0;     // millis() when emergency was triggered

// ── RF Deduplication ───────────────────────────────────────────
// 433MHz modules physically re-broadcast the same packet 2-3 times.
// Without dedup, one button press would call doEmergency() multiple times
// and reset the 30s timer mid-hold. We ignore same message within 2 seconds.
String        lastRFMsg    = "";   // last RF message received
unsigned long lastRFMillis = 0;    // when that message arrived
#define RF_DEDUP_MS 2000UL         // ignore duplicates within 2 seconds

// ── LCD Smart Update ───────────────────────────────────────────
// We store what's currently displayed. lcdWrite() only physically
// rewrites the display when content actually changes.
// This prevents visible flicker that would happen if we called
// lcd.clear() + lcd.print() every 2 seconds.
String lcdL0 = "";  // current content of LCD line 0
String lcdL1 = "";  // current content of LCD line 1

// ── Density for LCD ────────────────────────────────────────────
int totalVehicles = 0;      // total across all 4 lanes, sent by Python
#define DENSITY_LOW  5      // below this = LOW traffic on LCD
#define DENSITY_HIGH 10     // above this = HIGH traffic on LCD

// ── Forward declarations ───────────────────────────────────────
void checkRF();
void readSerial();
void parseCommand(const String& cmd);
void doEmergency(int laneIdx);
void doGreenCycle(int laneIdx, int greenSecs);
void allRed(int ms);
void setRed(int i);
void setYellow(int i);
void setGreen(int i);
void updateLCD();
void lcdWrite(const String& l0, const String& l1);
int  calcGreen(int count);
int  charToIdx(char c);
void checkWatchdog();

// =============================================================
//  SETUP — runs once on power-on or reset
// =============================================================
void setup() {
  Serial.begin(9600);  // must match SERIAL_BAUD in Python code

  // Set all 12 traffic light pins as OUTPUT and start all RED
  // This ensures intersection is safe on startup before any Python data arrives
  for (int i = 0; i < NUM_LANES; i++) {
    for (int j = 0; j < 3; j++) {
      pinMode(lanes[i][j], OUTPUT);
      digitalWrite(lanes[i][j], LOW);  // LOW first before setRed
    }
    setRed(i);  // RED on, YELLOW+GREEN off for every lane
  }

  // Buzzer off at startup
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);

  // Mode switch — INPUT_PULLUP means no external resistor needed
  // Floating (open) reads HIGH = DYNAMIC mode (safe default)
  // Connected to GND reads LOW = FIXED mode
  pinMode(MODE_PIN, INPUT_PULLUP);

  // Start RF receiver on Pin 11
  // If this fails, check that XY-MK-5V DATA pin is on Mega Pin 11
  if (!rf_driver.init()) {
    Serial.println(F("[RF] Init failed — check DATA wire on pin 11"));
  } else {
    Serial.println(F("[RF] Receiver ready on pin 11"));
  }

  // Read initial mode from switch before starting cycle
  dynamicMode = (digitalRead(MODE_PIN) == HIGH);
  lastEffMode = dynamicMode;

  // LCD startup: init, turn backlight on, show splash for 2 seconds
  lcd.init();
  lcd.backlight();
  lcdWrite("  Smart Traffic ", "  System Ready  ");
  delay(2000);
  updateLCD();  // show actual mode after splash

  Serial.print(F("[SYSTEM] Starting in "));
  Serial.println(dynamicMode ? F("DYNAMIC mode") : F("FIXED TIMER mode"));
  Serial.println(F("[SYSTEM] Clockwise cycle: N -> E -> S -> W"));
}

// =============================================================
//  MAIN LOOP — runs continuously forever
//
//  Each iteration handles ONE lane's full green cycle.
//  Steps in order every iteration:
//    1. Check RF receiver — ambulance button has highest priority
//    2. Handle active emergency — hold until 30s expires
//    3. Read mode switch — dynamic or fixed?
//    4. Read serial from Python — fresh vehicle counts
//    5. Calculate green time for current lane
//    6. Run green → yellow → red cycle for current lane
//    7. Advance clockwise to next lane
// =============================================================
void loop() {

  // STEP 1 — RF receiver: checked first, every single iteration
  // This gives emergency the fastest possible response time
  checkRF();

  // STEP 2 — Emergency hold management
  // If an ambulance triggered emergency, we stay here until 30s expires.
  // While waiting, we still poll serial so Python's COUNT commands
  // are not lost during the hold (they'll be used when cycle resumes).
  if (emergencyActive) {
    if (millis() - emergencyStart >= EMERGENCY_HOLD) {
      // 30 seconds up — ambulance has passed
      emergencyActive = false;
      allRed(ALLRED_GAP);  // brief all-red safety gap
      currentLane = (currentLane + 1) % NUM_LANES;  // move to NEXT lane
      // Note: we advance past the ambulance lane so it doesn't get
      // a second green immediately — fair to other lanes
      Serial.println(F("[SYSTEM] Ambulance passed — cycle resumed"));
      updateLCD();
    } else {
      // Still within 30s — keep polling so we don't miss serial data
      readSerial();
      checkWatchdog();
      delay(100);  // 100ms sleep prevents CPU burning at full speed
    }
    return;  // don't proceed to normal cycle while emergency is active
  }

  // STEP 3 — Read mode switch + apply watchdog override
  // dynamicMode = what the physical switch says
  // effMode     = what we actually use (watchdog can override switch)
  dynamicMode = (digitalRead(MODE_PIN) == HIGH);
  checkWatchdog();
  bool effMode = dynamicMode && !watchdogFixed;
  // effMode is false if: switch says FIXED, OR watchdog has activated

  // Detect and report mode changes (for serial monitor + LCD)
  if (effMode != lastEffMode) {
    lastEffMode = effMode;
    if (watchdogFixed)
      Serial.println(F("[WATCHDOG] Python offline -> AUTO FIXED mode"));
    else
      Serial.println(effMode ? F("[MODE] DYNAMIC") : F("[MODE] FIXED TIMER"));
    updateLCD();
  }

  // STEP 4 — Read all pending serial data from Python
  // This updates vehicleCount[] with the latest YOLO detection results
  readSerial();

  // STEP 5 — Decide green time for current lane based on mode
  // Dynamic: calcGreen() uses the latest vehicleCount for this lane
  // Fixed:   always FIXED_GREEN (20s) regardless of count
  int greenSecs = effMode ? calcGreen(vehicleCount[currentLane]) : FIXED_GREEN;

  // Print what's happening to serial monitor for debugging
  Serial.print(effMode ? F("[DYN] ") : F("[FIX] "));
  Serial.print(F("Lane "));
  Serial.print(laneChars[currentLane]);
  Serial.print(F(" | "));
  Serial.print(vehicleCount[currentLane]);
  Serial.print(F(" vehicles | GREEN for "));
  Serial.print(greenSecs);
  Serial.println(F("s"));

  // STEP 6 — Run the full green cycle for current lane
  // Inside doGreenCycle(), RF is polled every 100ms so an emergency
  // can interrupt mid-green immediately without waiting for cycle to finish
  doGreenCycle(currentLane, greenSecs);

  // STEP 7 — Advance clockwise to next lane
  // Only if emergency didn't take over mid-cycle (doGreenCycle returns early
  // when emergencyActive becomes true, so we skip the advance here and let
  // the emergency hold in step 2 handle the lane position instead)
  if (!emergencyActive) {
    currentLane = (currentLane + 1) % NUM_LANES;  // 0→1→2→3→0→...
    allRed(ALLRED_GAP);  // 500ms all-red safety gap before next lane starts
  }
}

// =============================================================
//  GREEN CYCLE
//  Sequence: all RED → current lane GREEN → YELLOW → RED
//
//  RF receiver and serial are polled every 100ms during the green
//  and yellow phases. If an ambulance presses the RF button mid-green,
//  emergencyActive becomes true and this function returns immediately,
//  interrupting the current cycle without waiting for green to finish.
// =============================================================
void doGreenCycle(int laneIdx, int greenSecs) {
  allRed(0);           // all lanes RED first (no gap here, gap is at end)
  setGreen(laneIdx);   // turn this lane GREEN

  // Green phase — loop until greenSecs have passed
  unsigned long t = millis();
  while (millis() - t < (unsigned long)greenSecs * 1000UL) {
    checkRF();       // check for ambulance RF button press
    readSerial();    // receive fresh counts from Python
    checkWatchdog(); // check if Python has gone offline
    if (emergencyActive) return;  // ambulance detected — exit immediately
    delay(100);      // 100ms tick — RF checked 10 times per second
  }

  // Yellow phase — warn other drivers green is ending
  setYellow(laneIdx);
  t = millis();
  while (millis() - t < (unsigned long)YELLOW_TIME * 1000UL) {
    checkRF();  // still check RF during yellow in case ambulance is urgent
    if (emergencyActive) return;
    delay(100);
  }

  setRed(laneIdx);  // this lane back to RED, ready for next lane
}

// =============================================================
//  GREEN TIME CALCULATOR — dynamic mode only
//
//  Formula: greenTime = MIN_GREEN + (vehicleCount × SECS_PER_VEHICLE)
//  Clamped between MIN_GREEN and MAX_GREEN.
//
//  With current settings (MIN=5, MAX=60, SECS_PER=6):
//    0 vehicles → 5s   (minimum — empty lane moves on fast)
//    1 vehicle  → 11s
//    3 vehicles → 23s
//    5 vehicles → 35s
//    8 vehicles → 53s
//    10+        → 60s  (capped — prevents starvation of other lanes)
// =============================================================
int calcGreen(int count) {
  int t = MIN_GREEN + (count * SECS_PER_VEHICLE);
  if (t < MIN_GREEN) t = MIN_GREEN;  // enforce minimum
  if (t > MAX_GREEN) t = MAX_GREEN;  // enforce maximum cap
  return t;
}

// =============================================================
//  EMERGENCY TRIGGER
//  Called by BOTH checkRF() and parseCommand() (YOLO backup).
//  Same action regardless of which source triggered it.
//
//  Sequence:
//    1. Guard: if already in emergency, ignore duplicate calls
//    2. Set emergency flag + record start time
//    3. Record which lane = ambulance lane (cycle resumes after it)
//    4. Buzzer beeps 3 times
//    5. All lanes RED → 300ms safety gap → ambulance lane GREEN
//    6. Update LCD to show EMERGENCY
//    7. loop() step 2 holds here for 30s then resumes clockwise
// =============================================================
void doEmergency(int laneIdx) {
  if (emergencyActive) return;  // already handling one, ignore

  emergencyActive = true;
  emergencyStart  = millis();
  currentLane     = laneIdx;  // after 30s, cycle resumes from (laneIdx+1)

  // 3 buzzer beeps to alert people at intersection
  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER, HIGH); delay(200);
    digitalWrite(BUZZER, LOW);  delay(150);
  }

  allRed(0);    // all lanes RED immediately
  delay(300);   // 300ms safety gap — all red before ambulance gets green
  setGreen(laneIdx);  // ambulance lane GREEN

  Serial.print(F("[EMERGENCY] Lane "));
  Serial.print(laneChars[laneIdx]);
  Serial.println(F(" GREEN for 30s — all other lanes RED"));
  updateLCD();
}

// =============================================================
//  RF RECEIVER
//  Checks if XY-MK-5V module has received a packet.
//  If a valid EMG_N/E/S/W message arrives → trigger emergency.
//
//  DEDUP: 433MHz modules re-broadcast the same packet 2-3 times
//  in quick succession. Without this check, one button press would
//  call doEmergency() multiple times in rapid succession, resetting
//  the 30s emergency timer. We ignore the same message if it arrives
//  again within RF_DEDUP_MS (2 seconds).
// =============================================================
void checkRF() {
  uint8_t buf[12];
  uint8_t buflen = sizeof(buf);
  if (!rf_driver.recv(buf, &buflen)) return;  // nothing received

  buf[buflen] = '\0';           // null-terminate to make it a C string
  String msg = String((char*)buf);
  msg.trim();                   // remove any trailing whitespace/newlines

  // Deduplication check — ignore same message within 2 seconds
  unsigned long now = millis();
  if (msg == lastRFMsg && (now - lastRFMillis) < RF_DEDUP_MS) return;
  lastRFMsg    = msg;
  lastRFMillis = now;

  Serial.print(F("[RF] Received: ")); Serial.println(msg);

  // Match message to lane index
  // These strings must exactly match what ambulance Uno transmits
  int idx = -1;
  if      (msg == "EMG_N") idx = 0;  // North button pressed
  else if (msg == "EMG_E") idx = 1;  // East  button pressed
  else if (msg == "EMG_S") idx = 2;  // South button pressed
  else if (msg == "EMG_W") idx = 3;  // West  button pressed

  if (idx >= 0) doEmergency(idx);
  // idx == -1 means unknown message — ignored silently
}

// =============================================================
//  SERIAL READER
//  Python wraps every command between < and > characters.
//  We read the USB serial one character at a time.
//  When we see '<'  → start collecting into rxBuf
//  When we see '>'  → command is complete, call parseCommand()
//  Any character in between → append to rxBuf
//  64-char limit on rxBuf guards against runaway/corrupted data
// =============================================================
void readSerial() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if      (c == '<')             { rxBuf = ""; inPacket = true;  }
    else if (c == '>' && inPacket) { inPacket = false; parseCommand(rxBuf); rxBuf = ""; }
    else if (inPacket && rxBuf.length() < 64) { rxBuf += c; }
  }
}

// =============================================================
//  COMMAND PARSER
//  Splits rxBuf by ':' into up to 4 parts.
//  Examples:
//    "COUNT:N:3"    → parts[0]="COUNT" parts[1]="N" parts[2]="3"
//    "DENSITY:16"   → parts[0]="DENSITY" parts[1]="16"
//    "EMERGENCY:N"  → parts[0]="EMERGENCY" parts[1]="N"
// =============================================================
void parseCommand(const String& cmd) {
  String parts[4];
  int count = 0, start = 0;
  for (int i = 0; i <= (int)cmd.length() && count < 4; i++) {
    if (i == (int)cmd.length() || cmd[i] == ':') {
      parts[count++] = cmd.substring(start, i);
      start = i + 1;
    }
  }
  if (count == 0) return;  // empty command, ignore

  String verb = parts[0];
  verb.toUpperCase();  // make case-insensitive

  // ── COUNT:X:N — vehicle count for one lane ─────────────────
  // Python sends this for all 4 lanes every 2 seconds.
  // We store it and use it next time that lane gets its green phase.
  if (verb == "COUNT" && count >= 3) {
    int idx = charToIdx(parts[1].charAt(0));  // 'N'→0, 'E'→1 etc.
    if (idx < 0) return;  // unknown lane character, ignore
    vehicleCount[idx] = parts[2].toInt();     // store the count

    // Reset watchdog — Python is alive and sending data
    lastPythonContact = millis();
    if (!pythonConnected) {
      pythonConnected = true;
      Serial.println(F("[WATCHDOG] Python connected"));
    }
    if (watchdogFixed) {
      // Watchdog had forced FIXED mode — Python is back, revert to DYNAMIC
      watchdogFixed = false;
      Serial.println(F("[WATCHDOG] Python reconnected -> resuming DYNAMIC mode"));
      updateLCD();
    }

    Serial.print(F("[COUNT] Lane "));
    Serial.print(laneChars[idx]);
    Serial.print(F(" = "));
    Serial.println(vehicleCount[idx]);
  }

  // ── DENSITY:N — total vehicle count for LCD ─────────────────
  // Python sends sum of all 4 lanes. We use it to update LCD
  // at secondary intersection. Only redraw LCD if value changed.
  else if (verb == "DENSITY" && count >= 2) {
    int n = parts[1].toInt();
    if (n != totalVehicles) {
      totalVehicles = n;
      updateLCD();  // lcdWrite() only redraws if text actually changed
    }
  }

  // ── EMERGENCY:X — YOLO detected ambulance (backup method) ───
  // Standard yolov8n.pt cannot detect ambulances (not a COCO class)
  // so this command will rarely arrive in practice.
  // The RF button is the primary emergency trigger.
  // This is kept as a backup in case a custom-trained model is used.
  else if (verb == "EMERGENCY" && count >= 2) {
    int idx = charToIdx(parts[1].charAt(0));
    if (idx >= 0) {
      Serial.println(F("[YOLO] Ambulance detected in camera — emergency triggered"));
      doEmergency(idx);
    }
  }
}

// =============================================================
//  PYTHON WATCHDOG
//  Called during every green cycle and emergency hold.
//
//  If Python has connected before (pythonConnected=true) but no
//  COUNT has arrived for PYTHON_TIMEOUT (30s), something is wrong
//  (PC crashed, WiFi dropped, Python script stopped).
//  We force FIXED mode so the intersection keeps running safely.
//  When Python reconnects and sends a COUNT, watchdogFixed resets.
// =============================================================
void checkWatchdog() {
  if (!pythonConnected) return;  // Python never connected — nothing to watch
  if (!watchdogFixed && (millis() - lastPythonContact > PYTHON_TIMEOUT)) {
    watchdogFixed = true;
    Serial.println(F("[WATCHDOG] No data from Python for 30s -> AUTO FIXED mode"));
    updateLCD();
  }
}

// =============================================================
//  LCD SMART UPDATE
//
//  lcdWrite() compares new content to what's currently on screen.
//  If identical → does nothing (no flicker, no unnecessary writes).
//  If different  → lcd.clear() then print both lines.
//
//  updateLCD() decides what to show based on current system state:
//    Emergency active  → EMERGENCY message (highest priority)
//    Watchdog fixed    → Python offline warning
//    Switch FIXED      → fixed timer message
//    DYNAMIC + density → traffic density level
// =============================================================
void lcdWrite(const String& l0, const String& l1) {
  if (l0 == lcdL0 && l1 == lcdL1) return;  // no change, skip rewrite
  lcdL0 = l0;
  lcdL1 = l1;
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(l0);
  lcd.setCursor(0, 1); lcd.print(l1);
}

void updateLCD() {
  // Emergency overrides everything — highest priority display
  if (emergencyActive) {
    lcdWrite("!! EMERGENCY !!", "Expect  Delays!");
    return;
  }

  bool effMode = dynamicMode && !watchdogFixed;

  if (!effMode) {
    // Not in dynamic mode — show which reason
    if (watchdogFixed)
      lcdWrite("Mode: AUTO FIXED", "Python offline  ");  // watchdog triggered
    else
      lcdWrite("Mode:   FIXED   ", "Green: 20s each ");  // switch is in FIXED position
    return;
  }

  // Dynamic mode — show live traffic density for people at secondary intersection
  String line1;
  if      (totalVehicles < DENSITY_LOW)  line1 = "Traffic:    LOW ";  // 0-4 vehicles
  else if (totalVehicles < DENSITY_HIGH) line1 = "Traffic:MODERATE";  // 5-9 vehicles
  else                                   line1 = "Traffic:HIGH !! ";  // 10+ vehicles
  lcdWrite("Mode:  DYNAMIC  ", line1);
}

// =============================================================
//  SIGNAL HELPERS
//  setRed/setYellow/setGreen take a lane index (0-3) and set
//  the correct 3 pins for that lane accordingly.
//  allRed() sets all 4 lanes to RED at once.
// =============================================================
void allRed(int ms) {
  for (int i = 0; i < NUM_LANES; i++) setRed(i);
  if (ms > 0) delay(ms);  // optional delay after setting all red
}

void setRed(int i) {
  // RED on, YELLOW off, GREEN off
  digitalWrite(lanes[i][RI], HIGH);
  digitalWrite(lanes[i][YI], LOW);
  digitalWrite(lanes[i][GI], LOW);
}

void setYellow(int i) {
  // RED off, YELLOW on, GREEN off
  digitalWrite(lanes[i][RI], LOW);
  digitalWrite(lanes[i][YI], HIGH);
  digitalWrite(lanes[i][GI], LOW);
}

void setGreen(int i) {
  // RED off, YELLOW off, GREEN on
  digitalWrite(lanes[i][RI], LOW);
  digitalWrite(lanes[i][YI], LOW);
  digitalWrite(lanes[i][GI], HIGH);
}

// =============================================================
//  LANE CHARACTER TO INDEX CONVERTER
//  Converts 'N'→0, 'E'→1, 'S'→2, 'W'→3 for array indexing
//  Returns -1 for unknown characters (safe guard against bad data)
// =============================================================
int charToIdx(char c) {
  switch (c) {
    case 'N': return 0;
    case 'E': return 1;
    case 'S': return 2;
    case 'W': return 3;
    default:  return -1;  // unknown lane — caller should check for -1
  }
}
