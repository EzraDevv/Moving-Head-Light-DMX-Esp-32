#include <Arduino.h>
#include <math.h>

// ---------------- PINS ----------------
#define TXD2 17
#define RXD2 16
#define EN_PIN 21
#define DIM_CH 6

#define RED_CH 7
#define GREEN_CH 8
#define BLUE_CH 9

#define MIC_PIN 34

// ---------------- DMX ----------------
#define DMX_CHANNELS 512
uint8_t dmx[DMX_CHANNELS + 1];

// ---------------- CHANNEL MAP ----------------
#define PAN_CH 1
#define TILT_CH 3

// ---------------- MOTION ----------------
float t = 0;

float pan_s = 127;
float tilt_s = 127;

float smoothFactor = 0.15;

int panCenter = 127;
int tiltCenter = 240;

int panAmp = 100;
int tiltAmp = 80;

// ---------------- SOUND SYSTEM ----------------
float micSmoothed = 0;
float envelope = 0;
float noiseFloor = 30;

// auto gain
float micMin = 9999;
float micMax = 0;

// ---------------- COLOR STATE ----------------
float r_s = 0, g_s = 0, b_s = 0;
float r_t = 0, g_t = 0, b_t = 0;

// ---------------- TIMING ----------------
unsigned long lastColorChange = 0;
const int holdTime = 300;

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

  static unsigned long lastDMX = 0;
  if (millis() - lastDMX < 20) return;
  lastDMX = millis();

  digitalWrite(EN_PIN, HIGH);

  sendBreak();

  Serial2.write(dmx, DMX_CHANNELS + 1);
  Serial2.flush();

  digitalWrite(EN_PIN, LOW);
}

// ---------------- BETTER MIC DETECTION ----------------
float readMicDB() {

  int raw = analogRead(MIC_PIN);
  int centered = abs(raw - 2048);

  // envelope follower (fast attack, slow release)
  float attack = 0.35;
  float release = 0.08;

  if (centered > envelope)
    envelope += (centered - envelope) * attack;
  else
    envelope += (centered - envelope) * release;

  // auto gain tracking
  micMin = min(micMin, envelope);
  micMax = max(micMax, envelope);

  micMin += 0.02;
  micMax -= 0.01;

  float range = micMax - micMin;
  if (range < 20) range = 20;

  float normalized = (envelope - micMin) / range;
  normalized = constrain(normalized, 0.0, 1.0);

  return normalized;
}

// ---------------- SOUND → COLOR ----------------
void updateSoundLight() {

  float level = readMicDB();

  // ---------------- COLOR HOLD LOGIC ----------------
  if (millis() - lastColorChange > holdTime) {

    if (level > 0.75) {
      r_t = 0; g_t = 0; b_t = 255;
      lastColorChange = millis();
    }
    else if (level > 0.5) {
      r_t = 0; g_t = 255; b_t = 0;
      lastColorChange = millis();
    }
    else if (level > 0.3) {
      r_t = 255; g_t = 0; b_t = 0;
      lastColorChange = millis();
    }
    else {
      r_t = 0; g_t = 0; b_t = 0;
      lastColorChange = millis();
    }
  }

  // ---------------- SMOOTH BRIGHTNESS ----------------
  static float brightness = 0;

  float targetBrightness = level;
  brightness += (targetBrightness - brightness) * 0.08;

  // ---------------- SMOOTH COLOR TRANSITION ----------------
  float colorSpeed = 0.12;

  r_s += (r_t * brightness - r_s) * colorSpeed;
  g_s += (g_t * brightness - g_s) * colorSpeed;
  b_s += (b_t * brightness - b_s) * colorSpeed;

  dmx[RED_CH]   = (int)r_s;
  dmx[GREEN_CH] = (int)g_s;
  dmx[BLUE_CH]  = (int)b_s;
}

// ---------------- SETUP ----------------
void setup() {

  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, LOW);

  Serial2.begin(250000, SERIAL_8N2, RXD2, TXD2);

  analogReadResolution(12);

  for (int i = 0; i <= DMX_CHANNELS; i++) {
    dmx[i] = 0;
  }

  dmx[DIM_CH] = 255;
}

// ---------------- LOOP ----------------
void loop() {

  // ---------------- FIGURE-8 MOTION ----------------
  float panTarget = panCenter + sin(t) * panAmp;
  float tiltTarget = tiltCenter + sin(2 * t) * tiltAmp;

  pan_s += (panTarget - pan_s) * smoothFactor;
  tilt_s += (tiltTarget - tilt_s) * smoothFactor;

  dmx[PAN_CH]  = constrain((int)pan_s, 0, 255);
  dmx[TILT_CH] = constrain((int)tilt_s, 0, 255);

  // ---------------- SOUND ----------------
  updateSoundLight();

  // ---------------- DMX ----------------
  sendDMX();

  t += 0.05;
}
