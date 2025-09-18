# IMU-Controlled Circle Animation on TFT Display  

This project demonstrates an interactive animation on a **240x240 TFT display** using an **MPU6050 accelerometer/gyroscope**. A grid of background circles is drawn, and a set of active circles (blue) move across the grid in response to device tilt.  

The goal is to create a visually engaging “falling pixel” effect where circles move and collide within the circular display boundary.  

---

## Features  
- **TFT_eSPI Rendering**: Uses the [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) library for fast graphics on a 240x240 display.  
- **MPU6050 Integration**: Reads accelerometer values from the Adafruit MPU6050 sensor.  
- **Grid System**: Background circles drawn in a padded grid, clipped to a circular boundary.  
- **Interactive Motion**: Active blue circles respond to tilt (X/Y axis acceleration).  
- **Collision Handling**: Prevents circles from overlapping by tracking occupied grid cells.  
- **Gravity Effect**: Circles naturally “fall” downward unless tilted.  

---

## Hardware Requirements  
- ESP32 (recommended for memory and performance).  
- 240x240 TFT display (GC9A01, ST7789, etc.).  
- MPU6050 accelerometer/gyroscope module.  
- Jumper wires and breadboard.  

---

## Library Dependencies  
Make sure the following libraries are installed in Arduino IDE / PlatformIO:  
- [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)  
- [Adafruit MPU6050](https://github.com/adafruit/Adafruit_MPU6050)  
- [Adafruit Sensor](https://github.com/adafruit/Adafruit_Sensor)  
- [Wire](https://www.arduino.cc/en/reference/wire) (I2C communication)  
- SPI (Arduino built-in)  

---

## How It Works  
1. The TFT screen is filled with background circles to create a pixelated effect.  
2. A number of circles (`NUM_CIRCLES`) are placed randomly on the grid.  
3. The MPU6050 reads tilt data (acceleration on X and Y).  
4. Circles move according to tilt direction while avoiding overlap.  
5. Circles stop at valid grid positions, maintaining the pixel illusion.  

---

## Usage  
1. Connect the TFT display and MPU6050 to the ESP32 using SPI (for display) and I2C (for MPU).  
2. Upload the provided Arduino sketch.  
3. Tilt the device to move the blue circles across the screen.  

---

## Demo  
- Circles shift left/right with **X-axis tilt**.  
- Circles fall downward or move upward depending on **Y-axis tilt**.  
- Grid and boundary ensure circles remain inside the drawable area.  

---

## Customization  
- `NUM_CIRCLES`: Adjust how many active circles are drawn.  
- `frameDelay`: Change animation speed (ms per frame).  
- `PADDING`: Controls grid spacing between circles.  
- Colors can be customized (currently `TFT_BLUE` for active circles).  

---

## Future Improvements  
- Add bounce physics when circles collide.  
- Improve performance by using sprite buffering.  
- Experiment with different IMU mappings for more natural motion.  
- Add wave-like effects inspired by pixel illusion demos.  

---

## License  
This project is released under the MIT License.  
