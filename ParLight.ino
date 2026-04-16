#define TXD2 17
#define RXD2 16
#define EN_PIN 21
#define MIC_PIN 34

HardwareSerial DMXSerial(1);
uint8_t dmxData[513];

unsigned long lastDMXTime = 0;


float smoothedDb = 0;


int currentR = 0, currentG = 0, currentB = 0;


int targetR = 0, targetG = 0, targetB = 0;


float brightness = 0;   
float fadeSpeed = 0.95; 


float colorSmooth = 0.1; 


float micSensitivity = 0.85;
int noiseFloor = 25;


unsigned long lastTrigger = 0;
int triggerDelay = 160; 


void setup() {
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, HIGH);

  DMXSerial.begin(250000, SERIAL_8N2, RXD2, TXD2);

  for (int i = 1; i <= 512; i++) {
    dmxData[i] = 0;
  }
}


void sendDMX() {
  if (micros() - lastDMXTime < 25000) return;
  lastDMXTime = micros();

  DMXSerial.end();

  pinMode(TXD2, OUTPUT);
  digitalWrite(TXD2, LOW);
  delayMicroseconds(100);

  digitalWrite(TXD2, HIGH);
  delayMicroseconds(12);

  DMXSerial.begin(250000, SERIAL_8N2, RXD2, TXD2);

  DMXSerial.write(0);

  for (int i = 1; i <= 512; i++) {
    DMXSerial.write(dmxData[i]);
  }
}


int readSoundLevel() {

  int minVal = 4095;
  int maxVal = 0;

  for (int i = 0; i < 50; i++) {
    int val = analogRead(MIC_PIN);
    if (val < minVal) minVal = val;
    if (val > maxVal) maxVal = val;
  }

  int peakToPeak = maxVal - minVal;

 
  peakToPeak = (int)(peakToPeak * micSensitivity);

 
  if (peakToPeak < noiseFloor) peakToPeak = 0;

  int db = map(peakToPeak, 0, 1500, 50, 90);

  
  smoothedDb = (smoothedDb * 0.7) + (db * 0.3);

  return (int)smoothedDb;
}


void triggerColor(int r, int g, int b) {
  targetR = r;
  targetG = g;
  targetB = b;
  brightness = 1.0;
}


void updateFade() {

 
  brightness *= fadeSpeed;
  if (brightness < 0.01) brightness = 0;

  
  currentR += (targetR - currentR) * colorSmooth;
  currentG += (targetG - currentG) * colorSmooth;
  currentB += (targetB - currentB) * colorSmooth;

  dmxData[1] = (int)(255 * brightness); 
  dmxData[2] = currentR;
  dmxData[3] = currentG;
  dmxData[4] = currentB;
}


void loop() {

  int db = readSoundLevel();

  
  if (millis() - lastTrigger > triggerDelay) {

    if (db >= 100) {
      triggerColor(0, 0, 255);
      lastTrigger = millis();
    }
    else if (db >= 90) {
      triggerColor(0, 255, 0);
      lastTrigger = millis();
    }
    else if (db >= 80) {
      triggerColor(255, 0, 0);
      lastTrigger = millis();
    }
  }

  
  updateFade();

  sendDMX();
}
