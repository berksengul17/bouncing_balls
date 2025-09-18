#include <TFT_eSPI.h>
#include <SPI.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);

Adafruit_MPU6050 mpu;

const int R = 3;
const int PADDING = 9;
const int DIAMETER = 240; // 240x240 display
const int SR = DIAMETER / 2;
const int DR = SR - R; // max drawable radius
const int32_t BG_COLOR = 0x18a3;

int xOffset = SR % PADDING, yOffset = SR % PADDING;

int cellX = (SR - xOffset) / PADDING;
int cellY = (SR - yOffset) / PADDING;

int nx = ceil((DIAMETER - xOffset) / (float)PADDING);
int ny = ceil((DIAMETER - yOffset) / (float)PADDING);

float ax, ay;

unsigned long lastUpdate = 0;
const int frameDelay = 30;  // ms between frames

// if a point is (-1, -1) then it means there is no circle drawn there
struct Point {
  int x;
  int y;
  Point(int xVal = -1, int yVal = -1) : x(xVal), y(yVal) {}
};

struct ActiveCircle {
  int cellX;
  int cellY;
};

Point* circles;
bool *occupied;

const int NUM_CIRCLES = 150;   // how many you want
ActiveCircle active[NUM_CIRCLES];

inline Point& AT(Point* circles, int y, int x, int nx) {
    return circles[y * nx + x];
}

// given the center of the circle can we draw it without overflow
bool isInBounds(int x, int y) {
  int dx = SR - x;
  int dy = SR - y;
  return dx*dx + dy*dy <= DR*DR;
}

int pixelToCell(int pixel, int offset) {
  return (pixel - offset) / PADDING;
}

void placeCircles() {
  int count = 0;
  for (int y = 0; y < ny && count < NUM_CIRCLES; y++) {
    for (int x = 0; x < nx && count < NUM_CIRCLES; x++) {
      Point p = AT(circles, y, x, nx);
      if (p.x == -1 || p.y == -1) continue; // skip invalid cells

      active[count].cellX = x;
      active[count].cellY = y;

      tft.fillCircle(p.x, p.y, R, TFT_BLUE);
      count++;
    }
  }
}

void initBackground() {
  // shift so center aligns
  for (int x=xOffset; x<DIAMETER; x+=PADDING) {
    for (int y=yOffset; y<DIAMETER; y+=PADDING) {
      if(!isInBounds(x, y)) continue; 
      tft.fillCircle(x, y, R, BG_COLOR);

      int cX = pixelToCell(x, xOffset);
      int cY = pixelToCell(y, yOffset);

      AT(circles, cY, cX, nx) = {x, y};
    }
  }
}

// get the acceleration in the x and y and update frame accordingly
void updateFrame(float ax, float ay) {
  int updateX = 0, updateY = 0;
  // X-axis controlled by IMU tilt
  if (ax > 0.4) updateX = 1;
  else if (ax < -0.4) updateX = -1;

  // Y-axis controlled by IMU tilt
  if (ay > 0.4) updateY = -1;
  else if (ay < -0.4) updateY = 1;
  else updateY = 1;  // 👈 default downward pull

  // Serial.printf("ax: %f | ay: %f | updateX: %d | updateY: %d", ax, ay, updateX, updateY);

  for (int i = 0; i < NUM_CIRCLES; i++) {
    int oldIndex = active[i].cellY * nx + active[i].cellX;

    int nextX = active[i].cellX + updateX;
    int nextY = active[i].cellY + updateY;

    // clamp to grid
    if (nextX < 0) nextX = 0;
    if (nextX >= nx) nextX = nx - 1;
    if (nextY < 0) nextY = 0;
    if (nextY >= ny) nextY = ny - 1;

    int newIndex = nextY * nx + nextX;

    // skip if occupied
    if (occupied[newIndex]) continue;

    Point candidate = AT(circles, nextY, nextX, nx);
    if (candidate.x == -1 || candidate.y == -1) continue;

    // erase old circle
    Point oldP = AT(circles, active[i].cellY, active[i].cellX, nx);
    tft.fillCircle(oldP.x, oldP.y, R, BG_COLOR);

    // free old cell
    occupied[oldIndex] = false;

    // update
    active[i].cellX = nextX;
    active[i].cellY = nextY;

    // occupy new cell
    occupied[newIndex] = true;

    // draw new
    tft.fillCircle(candidate.x, candidate.y, R, TFT_BLUE);
  }
}

void setup() {
  Serial.begin(9600);

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");

  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  circles = new Point[ny*nx];
  occupied = new bool[ny*nx];
  for (int i = 0; i < nx * ny; i++) {
    occupied[i] = false;
  }
  initBackground();
  placeCircles();
}

void loop() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  ax = a.acceleration.x;
  ay = a.acceleration.y;
  
  unsigned long now = millis();
  if (now - lastUpdate > frameDelay) {
    lastUpdate = now;
    updateFrame(ax, ay);
  }
}

