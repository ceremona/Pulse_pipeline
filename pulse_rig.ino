/*
  pulse_rig.ino — supercapacitor pulse-discharge fixture controller
  Companion to the battery-cycle-life notebook: an HPPC-lite test rig.

  WIRING (must match your build):
    D8 -> Q2 base via 1k   (LOW  = charger ON,  high-side PNP from 5V)
    D9 -> Q1 gate via 100  (HIGH = load ON,     low-side N-FET)
    A0 -> CAP_H (cap + terminal). 2.7V max < 5V ADC reference: safe direct.

  FAIL-SAFE DEFAULTS:
    Both switches default OFF at reset (R2 pull-up holds the charger off,
    R6 pull-down holds the load off) — a mid-test Arduino crash can never
    leave the cap discharging unattended.

  INTERLOCKS:
    - refuses to pulse below V_MIN_FIRE (report, do not fire)
    - refuses to charge above V_MAX_SAFE (hard clamp, cap is 2.7V rated)
    - charge loop has a hard timeout

  SERIAL PROTOCOL (115200 baud, newline-terminated):
    S            status report
    C<target_mV> charge to target, then stop (clamped to V_MAX_SAFE)
    P<ms>        fire one pulse if interlocks pass (clamped 50..5000 ms)
    B<ms>        top up charge if low, then fire — one-button batch shot
  Every reply starts with OK / ERR / READY for machine parsing.
*/

const uint8_t PIN_CHG  = 8;    // BC327 base via 1k
const uint8_t PIN_GATE = 9;    // IRLZ44N gate via 100
const uint8_t PIN_VCAP = A0;   // cap + terminal

const float VREF         = 5.0;      // ADC reference
const float V_MIN_FIRE   = 2.20;     // interlock: no pulse below this
const float V_MAX_SAFE   = 2.60;     // never charge above (2.7V rated cap)
const float V_CHG_STOP   = 0.004;    // charge-loop stop hysteresis (V)
const unsigned long CHG_TIMEOUT_MS = 300000UL;  // 5-minute charge failsafe
const unsigned long CHG_POLL_MS    = 25;        // charge loop tick
const unsigned long ADC_AVG        = 32;        // oversampling count

float readVcap() {
  // Oversampled A0 read. ~5mV resolution: plenty for interlocks and charge
  // targeting; the METROLOGY happens on the scope, not on this ADC.
  unsigned long acc = 0;
  for (unsigned long i = 0; i < ADC_AVG; i++) {
    acc += analogRead(PIN_VCAP);
    delayMicroseconds(50);
  }
  return (acc / (float)ADC_AVG) * VREF / 1023.0;
}

void chargeOn()  { digitalWrite(PIN_CHG, LOW);  }  // PNP conducts, base pulled low
void chargeOff() { digitalWrite(PIN_CHG, HIGH); }  // plus R2 pull-up at reset
void loadOn()    { digitalWrite(PIN_GATE, HIGH); }
void loadOff()   { digitalWrite(PIN_GATE, LOW);  }  // plus R6 pull-down at reset

// Charge to target, blocking. Returns the voltage actually reached.
float chargeTo(float target) {
  if (target > V_MAX_SAFE) target = V_MAX_SAFE;   // hard clamp
  unsigned long t0 = millis();
  while (readVcap() < target - V_CHG_STOP) {
    if (millis() - t0 > CHG_TIMEOUT_MS) {          // failsafe
      chargeOff();
      return readVcap();
    }
    chargeOn();
    delay(CHG_POLL_MS);
  }
  chargeOff();                                     // disconnect BEFORE pulse:
  delay(50);                                        // clean measurement loop
  return readVcap();
}

void firePulse(unsigned long ms) {
  loadOn();
  delay(ms);      // ms-scale is all this rig needs; the scope timestamps physics
  loadOff();
}

void setup() {
  pinMode(PIN_CHG, OUTPUT);  chargeOff();   // safe defaults FIRST
  pinMode(PIN_GATE, OUTPUT); loadOff();
  Serial.begin(115200);
  delay(300);                                 // rail settle
  Serial.print(F("READY v="));
  Serial.println(readVcap(), 3);
}

void loop() {
  static char line[24];
  static uint8_t n = 0;
  if (!Serial.available()) return;
  char c = (char)Serial.read();
  if (c == '\r') return;
  if (c == '\n') { line[n] = 0; handle(line); n = 0; }
  else if (n < sizeof(line) - 1) { line[n++] = c; }
}

void handle(const char* cmd) {
  char op = cmd[0];
  long arg = (strlen(cmd) > 1) ? atol(cmd + 1) : 0;

  if (op == 'S') {
    Serial.print(F("DATA state=idle v="));
    Serial.println(readVcap(), 3);
    return;
  }

  if (op == 'C') {                       // C2450 -> charge to 2.45V
    float target = (arg > 100 && arg < 2700) ? arg / 1000.0 : 2.45;
    unsigned long t0 = millis();
    float v = chargeTo(target);
    Serial.print(F("OK CHARGE v="));
    Serial.print(v, 3);
    Serial.print(F(" t_s="));
    Serial.println((millis() - t0) / 1000.0);
    return;
  }

  if (op == 'P' || op == 'B') {          // B = auto top-up, then fire
    float v = readVcap();
    if (op == 'B' && v < V_MIN_FIRE) {
      Serial.println(F("OK topping charge"));
      v = chargeTo(2.45);
    }
    if (v < V_MIN_FIRE) {                // interlock: report, never fire
      Serial.print(F("ERR BLOCKED low_vcap v="));
      Serial.println(v, 3);
      return;
    }
    unsigned long ms = (arg >= 50 && arg <= 5000) ? (unsigned long)arg : 1000;
    float vBefore = v;
    firePulse(ms);
    Serial.print(F("OK PULSE ms="));
    Serial.print(ms);
    Serial.print(F(" v_before="));
    Serial.print(vBefore, 3);
    Serial.print(F(" v_after="));
    Serial.println(readVcap(), 3);
    return;
  }

  Serial.println(F("ERR CMD"));
}
