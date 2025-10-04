#include <TFT_eSPI.h>
#include <SPI.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

struct ActiveCircle {
  float x, y, oldX, oldY, vx, vy;
  ActiveCircle(float x=-1, float y=-1, float vx=5, float vy=5)
      : x(x), y(y), oldX(x), oldY(y), vx(vx), vy(vy) {}
};

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite bgSprite = TFT_eSprite(&tft);      // static background
TFT_eSprite circleSprite = TFT_eSprite(&tft);  // moving circles
TFT_eSprite ball = TFT_eSprite(&tft);

Adafruit_MPU6050 mpu;

const int R = 3;
const int PADDING = 9;
const int ORIGINAL_DIAMETER = 240;
const int DIAMETER = 226; // 240x240 display
const int CIRCLE_SPRITE_LENGTH = 226;
const int SR = DIAMETER / 2;
const int DR = SR - R; 
const int32_t BG_COLOR = 0x18a3;

const int NUM_CIRCLES = 10;
const float VX_LIMIT = 1.0;
const float VY_LIMIT = 1.0;

int xOffset = SR % PADDING, yOffset = SR % PADDING;
int bgSpriteOffset = ceil((ORIGINAL_DIAMETER - DIAMETER) / 2.0);
int circleSpriteOffset = ceil((DIAMETER - CIRCLE_SPRITE_LENGTH) / 2.0);
int nx = ceil((DIAMETER - xOffset) / (float)PADDING);
int ny = ceil((DIAMETER - yOffset) / (float)PADDING);

float ax, ay;
unsigned long lastUpdate = 0;
const int frameDelay = 30;  // ms

ActiveCircle active[NUM_CIRCLES];
TFT_eSprite* balls[NUM_CIRCLES];

bool isInBounds(float x, float y) {
  float dx = SR - x;
  float dy = SR - y;
  return dx*dx + dy*dy <= DR*DR;
}

bool isUnderSpeedLimit(float vx, float vy) {
  return fabs(vx) < VX_LIMIT && fabs(vy) < VY_LIMIT;
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

    Serial.printf("vx: %f | vy: %f\n", c.vx, c.vy);

    // damping
    c.vx *= 0.95;
    c.vy *= 0.95;

    if (isUnderSpeedLimit(c.vx, c.vy)) {
      c.vx = 0.0;
      c.vy = 0.0;
    }

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

void eraseOldCircle(float oldX, float oldY) {
  if (oldX < 0 || oldY < 0) return;

  int px = (int)oldX;
  int py = (int)oldY;

  uint16_t *bgBuf = (uint16_t*) bgSprite.getPointer();
  int bgW = bgSprite.width();

  uint16_t *eraseBuf = (uint16_t*) circleSprite.getPointer();
  int ew = circleSprite.width();
  int eh = circleSprite.height();

  for (int yy = 0; yy < eh; yy++) {
    for (int xx = 0; xx < ew; xx++) {
      int bx = px - R + xx;
      int by = py - R + yy;

      // Bounds check inside bgSprite
      if (bx >= 0 && bx < bgW && by >= 0 && by < bgSprite.height()) {
        eraseBuf[yy * ew + xx] = bgBuf[by * bgW + bx];
      } else {
        eraseBuf[yy * ew + xx] = TFT_BLACK; // fallback if out of bounds
      }
    }
  }

  // Push restored block back to TFT
  circleSprite.pushSprite(px - R + bgSpriteOffset, py - R + bgSpriteOffset);
}

void drawFrame() {
  for (int i=0; i<NUM_CIRCLES; i++) {
    ActiveCircle &c = active[i];

    // ---- ERASE old position ----
    if (c.oldX >= 0 && c.oldY >= 0) {
      balls[i]->fillSprite(TFT_BLACK);  // clean the mini-sprite
      balls[i]->fillCircle(R, R, R, TFT_BLACK);  // draw background dot color
      balls[i]->pushSprite((int)c.oldX - R + bgSpriteOffset,
                           (int)c.oldY - R + bgSpriteOffset);
    }

    // Snap to grid if needed
    if (isUnderSpeedLimit(c.vx, c.vy)) {
      int cellX = round((c.x - xOffset) / (float)PADDING);
      int cellY = round((c.y - yOffset) / (float)PADDING);
      c.x = cellX * PADDING + xOffset;
      c.y = cellY * PADDING + yOffset;
    }

    // ---- DRAW new circle ----
    balls[i]->fillSprite(TFT_BLACK);
    balls[i]->fillCircle(R, R, R, TFT_BLUE);
    balls[i]->pushSprite((int)c.x - R + bgSpriteOffset,
                         (int)c.y - R + bgSpriteOffset,
                         TFT_BLACK);

    // update old position
    c.oldX = c.x;
    c.oldY = c.y;
  }
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
  
  // bgSprite.setColorDepth(16);
  // bgSprite.createSprite(DIAMETER, DIAMETER);
  // bgSprite.fillSprite(TFT_BLACK);
  
  //initBackground();
  // bgSprite.pushSprite(bgSpriteOffset, bgSpriteOffset, TFT_BLACK);
  
  initActiveCircles();
  for (int i = 0; i < NUM_CIRCLES; i++) {
    balls[i] = new TFT_eSprite(&tft);
    balls[i]->setColorDepth(16);
    balls[i]->createSprite(2*R+2, 2*R+2);
  }
  drawFrame();
}

void loop() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  ax = a.acceleration.x;
  ay = a.acceleration.y;
  updatePhysics(ax, ay);
  
  unsigned long now = millis();
  if (now - lastUpdate > frameDelay) {
    lastUpdate = now;
    drawFrame();
  }
}
