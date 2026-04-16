#include <Arduino.h>
#include <math.h>

// ---------------- PINS ----------------
#define TXD2 17
#define RXD2 16
#define EN_PIN 21
#define DIM_CH 6
#define WHITE_CH 10

// ---------------- DMX ----------------
#define DMX_CHANNELS 512
uint8_t dmx[DMX_CHANNELS + 1];

// ---------------- CHANNEL MAP ----------------
#define PAN_CH  1
#define TILT_CH 3

// ---------------- TIMING ----------------
unsigned long lastDMX = 0;
const int DMX_INTERVAL = 20;

// ---------------- MOTION ----------------
float t = 0;

float pan_s  = 127;
float tilt_s = 127;

float smoothFactor = 0.15;

// ✅ SAFE LIMITS
int panCenter  = 127;
int tiltCenter = 240;

int panAmp  = 100;  // keep inside range
int tiltAmp = 80;

// ---------------- DMX BREAK ----------------
void sendBreak() {
  Serial2.end();

  pinMode(TXD2, OUTPUT);

  digitalWrite(TXD2, LOW);
  delayMicroseconds(120);

  digitalWrite(TXD2, HIGH);
  delayMicroseconds(12);

  Serial2.begin(250000, SERIAL_8N2, RXD2, TXD2);
}

// ---------------- SEND DMX ----------------
void sendDMX() {
  digitalWrite(EN_PIN, HIGH);

  sendBreak();

  Serial2.write(dmx, DMX_CHANNELS + 1);
  Serial2.flush();

  digitalWrite(EN_PIN, LOW);
}

// ---------------- SETUP ----------------
void setup() {
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, LOW);

  Serial2.begin(250000, SERIAL_8N2, RXD2, TXD2);

  for (int i = 0; i <= DMX_CHANNELS; i++) {
    dmx[i] = 0;
  }

  // LIGHT ON (RED)
  dmx[DIM_CH] = 255;
  dmx[WHITE_CH] = 255;
}

// ---------------- LOOP ----------------
void loop() {

  if (millis() - lastDMX >= DMX_INTERVAL) {
    lastDMX = millis();

    // ✅ TRUE FIGURE-8 (LISSAJOUS CURVE)
    float panTarget  = panCenter  + sin(t) * panAmp;
    float tiltTarget = tiltCenter + sin(2 * t) * tiltAmp;

    // smoothing
    pan_s  += (panTarget - pan_s) * smoothFactor;
    tilt_s += (tiltTarget - tilt_s) * smoothFactor;

    // write DMX
    dmx[PAN_CH]  = constrain((int)pan_s, 0, 255);
    dmx[TILT_CH] = constrain((int)tilt_s, 0, 255);

    sendDMX();

    t += 0.05; // speed
  }
}
