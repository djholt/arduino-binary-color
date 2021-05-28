#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include <CapacitiveSensor.h>

#define CAPACITIVE_THRESHOLD 10

#define JEWEL_PIN 53
#define MATRIX_PIN 49
#define STICK_PIN 51
#define SPEAKER_PIN 47

#define JEWEL_SIZE 7
#define MATRIX_SIZE 8
#define STICK_SIZE 8

unsigned long debounceDelay = 50;
unsigned long lastDebounceTimes[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
bool lastSensorStates[] = {false, false, false, false, false, false, false, false, false};
bool sensorStates[] = {false, false, false, false, false, false, false, false, false};

CapacitiveSensor s1 = CapacitiveSensor(41, 23);
CapacitiveSensor s2 = CapacitiveSensor(41, 25);
CapacitiveSensor s3 = CapacitiveSensor(41, 27);
CapacitiveSensor s4 = CapacitiveSensor(41, 29);
CapacitiveSensor s5 = CapacitiveSensor(41, 31);
CapacitiveSensor s6 = CapacitiveSensor(41, 33);
CapacitiveSensor s7 = CapacitiveSensor(41, 35);
CapacitiveSensor s8 = CapacitiveSensor(41, 37);
CapacitiveSensor s9 = CapacitiveSensor(41, 39);
CapacitiveSensor *sensors[] = {&s1, &s2, &s3, &s4, &s5, &s6, &s7, &s8, &s9};

byte bytes[] = {0, 0, 0};
int currentByteIndex = 0;
int currentStickSwipePixel = 0;

bool attractMode = true;
long timeSinceActive = 0;

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(MATRIX_SIZE, MATRIX_SIZE, 5, 1, MATRIX_PIN,
  NEO_TILE_TOP   + NEO_TILE_LEFT   + NEO_TILE_ROWS   + NEO_TILE_PROGRESSIVE +
  NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_PROGRESSIVE,
  NEO_GRB + NEO_KHZ800);

Adafruit_NeoPixel jewels = Adafruit_NeoPixel(JEWEL_SIZE * 8, JEWEL_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel sticks = Adafruit_NeoPixel(STICK_SIZE * 3, STICK_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(9600);
  initPixels();
  displayCurrentByte();
}

void loop() {
  checkSensors();
  swipeStick(currentByteIndex, currentByteColor32());
  delay(30);
}

void checkSensors() {
  if (checkSensorPressed(0)) {
    fillStick(currentByteIndex, currentByteColor32());
    for (int i = 0; i < 8; i++) {
      resetJewel(i);
    }
    currentByteIndex++;
    if (currentByteIndex == 3) {
      toneColorSet();
      displayCurrentColor();
      outputBytes();
      resetBytes();
      for (int i = 0; i < 3; i++) {
        resetStick(i);
      }
    } else {
      toneByteSet();
      displayCurrentByte();
    }
  }

  for (int i = 0; i < 8; i++) {
    if (checkSensorPressed(i+1)) {
      if (toggleBit(i)) {
        toneBitOn();
        toggleJewelOn(8-i-1, currentByteColor32());
      } else {
        toneBitOff();
        toggleJewelOff(8-i-1);
      }
      displayCurrentByte();
    }
  }
}

bool checkSensorPressed(int i) {
  bool pressed = false;
  long m = sensors[i]->capacitiveSensor(5);
  bool reading = m > CAPACITIVE_THRESHOLD;

  if (reading != lastSensorStates[i]) {
    lastDebounceTimes[i] = millis();
  }

  if (millis() - lastDebounceTimes[i] > debounceDelay) {
    if (reading != sensorStates[i]) {
      sensorStates[i] = reading;

      if (reading) {
        pressed = true;
        timeSinceActive = 0;
      }
    }
  }

  lastSensorStates[i] = reading;
  return pressed;
}

uint16_t currentByteColor16() {
  return matrix.Color(255 * (currentByteIndex == 0), 255 * (currentByteIndex == 1), 255 * (currentByteIndex == 2));
}

uint32_t currentByteColor32() {
  return jewels.Color(255 * (currentByteIndex == 0), 255 * (currentByteIndex == 1), 255 * (currentByteIndex == 2));
}

void displayCurrentByte() {
  matrix.fillScreen(0);
  matrix.setTextColor(currentByteColor16());

  String dec = String(bytes[currentByteIndex], DEC);

  matrix.setCursor(20 - 3 * dec.length(), 0);
  matrix.print(dec);

  matrix.show();
}

void displayCurrentColor() {
  uint16_t color = matrix.Color(bytes[0], bytes[1], bytes[2]);
  uint16_t white = matrix.Color(255, 255, 255);

  matrix.fillScreen(0);
  matrix.setTextColor(color);

  char hex[8];
  sprintf(hex, "%02X%02X%02X", bytes[0], bytes[1], bytes[2]);

  matrix.setCursor(5, 0);
  matrix.print(hex);

  for (int i = 0; i < 4; i++) {
    matrix.drawPixel(i, 2, white);
    matrix.drawPixel(i, 4, white);
  }
  for (int i = 0; i < 5; i++) {
    matrix.drawPixel(1, i+1, white);
    matrix.drawPixel(2, i+1, white);
  }

  matrix.show();
}

void initPixels() {
  jewels.begin();
  jewels.show();

  matrix.begin();
  matrix.setBrightness(50);
  matrix.setTextWrap(false);
  matrix.show();

  sticks.begin();
  sticks.show();
}

void outputBytes() {
  Serial.println(String(bytes[0]) + ',' + String(bytes[1]) + ',' + String(bytes[2]));
}

void resetBytes() {
  bytes[0] = 0;
  bytes[1] = 0;
  bytes[2] = 0;
  currentByteIndex = 0;
}

bool toggleBit(int i) {
  bytes[currentByteIndex] = bytes[currentByteIndex] ^ (1 << i);
  return bytes[currentByteIndex] & (1 << i);
}

void toneBitOn() {
  noTone(SPEAKER_PIN);
  tone(SPEAKER_PIN, 3200, 100);
  delay(110);
}

void toneBitOff() {
  noTone(SPEAKER_PIN);
  tone(SPEAKER_PIN, 2800, 100);
  delay(110);
}

void toneByteSet() {
  noTone(SPEAKER_PIN);
  tone(SPEAKER_PIN, 600, 100);
  delay(110);
  tone(SPEAKER_PIN, 800, 100);
  delay(110);
  tone(SPEAKER_PIN, 1000, 200);
  delay(220);
}

void toneColorSet() {
  int i;
  noTone(SPEAKER_PIN);
  for (i = 0; i < 8; i++) {
    tone(SPEAKER_PIN, 200 + 200 * i, 50);
    delay(55);
  }
  noTone(SPEAKER_PIN);
  for (i = 0; i < 8; i++) {
    tone(SPEAKER_PIN, 400 + 200 * i, 50);
    delay(55);
  }
  noTone(SPEAKER_PIN);
  for (i = 0; i < 8; i++) {
    tone(SPEAKER_PIN, 800 + 200 * i, 50);
    delay(55);
  }
  tone(SPEAKER_PIN, 800 + 200 * i, 500);
  delay(550);
}

void fillStick(int i, uint32_t color) {
  resetStick(i);
  for (int j = STICK_SIZE * i; j < STICK_SIZE * (i + 1); j++) {
    sticks.setPixelColor(j, color);
    sticks.show();
    delay(30);
  }
}

void resetJewel(int i) {
  for (int j = JEWEL_SIZE * i; j < JEWEL_SIZE * (i + 1); j++) {
    jewels.setPixelColor(j, 0);
  }
  jewels.show();
}

void resetStick(int i) {
  for (int j = STICK_SIZE * i; j < STICK_SIZE * (i + 1); j++) {
    sticks.setPixelColor(j, 0);
  }
  sticks.show();
}

void swipeStick(int i, uint32_t color) {
  resetStick(i);
  int pixel = currentStickSwipePixel;
  if (pixel >= STICK_SIZE) {
    pixel = STICK_SIZE - (pixel - STICK_SIZE) - 1;
  }
  sticks.setPixelColor(STICK_SIZE * i + pixel, color);
  currentStickSwipePixel = (currentStickSwipePixel + 1) % (STICK_SIZE * 2);
  sticks.show();
}

void toggleJewelOn(int i, uint32_t color) {
  for (int j = JEWEL_SIZE * i; j < JEWEL_SIZE * (i + 1); j++) {
    jewels.setPixelColor(j, color);
    jewels.show();
    delay(30);
  }
}

void toggleJewelOff(int i) {
  for (int j = JEWEL_SIZE * (i + 1) - 1; j >= JEWEL_SIZE * i; j--) {
    jewels.setPixelColor(j, 0);
    jewels.show();
    delay(30);
  }
}

