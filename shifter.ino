#include <TFT_eSPI.h>
#include <SPI.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite bgSprite = TFT_eSprite(&tft);      // static background
TFT_eSprite circleSprite = TFT_eSprite(&tft);  // moving circles
TFT_eSprite ball = TFT_eSprite(&tft);

Adafruit_MPU6050 mpu;

const int R = 3;
const int PADDING = 9;
const int ORIGINAL_DIAMETER = 240;
const int DIAMETER = 234; // 240x240 display
const int CIRCLE_SPRITE_LENGTH = 226;
const int SR = DIAMETER / 2;
const int DR = SR - R; 
const int32_t BG_COLOR = 0x18a3;

int xOffset = SR % PADDING, yOffset = SR % PADDING;
int bgSpriteOffset = ceil((ORIGINAL_DIAMETER - DIAMETER) / 2.0);
int circleSpriteOffset = ceil((DIAMETER - CIRCLE_SPRITE_LENGTH) / 2.0);
int nx = ceil((DIAMETER - xOffset) / (float)PADDING);
int ny = ceil((DIAMETER - yOffset) / (float)PADDING);

float ax, ay;
unsigned long lastUpdate = 0;
const int frameDelay = 30;  // ms

struct ActiveCircle {
  float x, y, vx, vy;
  ActiveCircle(float x=-1, float y=-1, float vx=5, float vy=5)
      : x(x), y(y), vx(vx), vy(vy) {}
};

const int NUM_CIRCLES = 1;
ActiveCircle active[NUM_CIRCLES];

bool isInBounds(float x, float y) {
  float dx = SR - x;
  float dy = SR - y;
  return dx*dx + dy*dy <= DR*DR;
}

bool checkForCircleCollision(int idx, float x, float y) {
  for (int i=0; i<NUM_CIRCLES; i++) {
    if (i == idx) continue;
    ActiveCircle c = active[i];
    float dx = c.x - x;
    float dy = c.y - y;
    float dist = dx*dx + dy*dy;
    if (dist <= (2*R)*(2*R)) return true;
  }
  return false;
}

bool isColliding(int idx, float x, float y) {
  return (checkForCircleCollision(idx, x, y) || !isInBounds(x, y));
}

void initBackground() {
  for (int x=xOffset; x<DIAMETER; x+=PADDING) {
    for (int y=yOffset; y<DIAMETER; y+=PADDING) {
      if (!isInBounds(x, y)) continue;
      bgSprite.fillCircle(x, y, R, BG_COLOR);
    }
  }
}

void initActiveCircles() {
  int count = 0;
  for (int y = 0; y < ny && count < NUM_CIRCLES; y++) {
    for (int x = 0; x < nx && count < NUM_CIRCLES; x++) {
      if (!isInBounds(x*PADDING + xOffset, y*PADDING + yOffset)) continue;
      active[count].x = x*PADDING + xOffset;
      active[count].y = y*PADDING + yOffset;
      count++;
    }
  }
}

void updatePhysics(float ax, float ay) {
  if (fabs(ax) < 0.4 && fabs(ay) < 0.4) return;

  for (int i=0; i<NUM_CIRCLES; i++) {
    ActiveCircle &c = active[i];

    // apply acceleration
    c.vx += ax * 2.0;
    c.vy -= ay * 2.0;

    // damping
    c.vx *= 0.95;
    c.vy *= 0.95;

    float newX = c.x + c.vx;
    float newY = c.y + c.vy;

    if (isColliding(i, newX, newY)) {
      c.vx = -c.vx * 0.8;
      c.vy = -c.vy * 0.8;
    } else {
      c.x = newX;
      c.y = newY;
    }
  }
}

void drawFrame() {
  bgSprite.fillSprite(TFT_BLACK);
  circleSprite.fillSprite(TFT_BLACK);
  initBackground();

  for (int i=0; i<NUM_CIRCLES; i++) {
    ActiveCircle c = active[i];
    int x = c.x;
    int y = c.y;

    if (fabs(c.vx) < 0.2 && fabs(c.vy) < 0.2) {  
      int cellX = ceil(x / PADDING);
      int cellY = ceil(y / PADDING);

      x = cellX * R;
      y = cellY * R;

      Serial.printf("cellX: %d, cellY: %d\n", cellX, cellY);
    }

    circleSprite.fillCircle(x - circleSpriteOffset, y - circleSpriteOffset, R, TFT_BLUE);
  }

  circleSprite.pushToSprite(&bgSprite, circleSpriteOffset, circleSpriteOffset, TFT_BLACK);
  bgSprite.pushSprite(bgSpriteOffset, bgSpriteOffset);
}

void setup() {
  Serial.begin(9600);

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) delay(10);
  }
  Serial.println("MPU6050 Found!");

  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  
  bgSprite.setColorDepth(16);
  bgSprite.createSprite(DIAMETER, DIAMETER);
  bgSprite.fillSprite(TFT_BLACK);

  initActiveCircles();

  circleSprite.setColorDepth(16);
  circleSprite.createSprite(CIRCLE_SPRITE_LENGTH, CIRCLE_SPRITE_LENGTH);
  drawFrame();
}

void loop() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  ax = a.acceleration.x;
  ay = a.acceleration.y;

  unsigned long now = millis();
  if (now - lastUpdate > frameDelay) {
    lastUpdate = now;
    updatePhysics(ax, ay);
    drawFrame();
  }
}
