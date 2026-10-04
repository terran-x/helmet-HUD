#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <pgmspace.h>
#ifndef PROGMEM
#define PROGMEM
#endif
#include <SparkFun_SSD1357_OLED.h>

// Screen size for SSD1357 panel (user: 128x64)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// ESP32 SPI pin configuration (change to match your wiring)
#define PIN_MOSI 23
#define PIN_SCLK 18
#define PIN_CS   5
#define PIN_DC   16
#define PIN_RST  17

// Create SSD1357 display object
SSD1357 display;

// Grid configuration based on text size 1 (which is 5x7 pixels + spacing)
#define CHAR_WIDTH 6
#define COLS (SCREEN_WIDTH / CHAR_WIDTH)

// Arrays to track the state of each falling rain column
int rainY[COLS];    // Vertical position of the leading character
int rainSpeed[COLS]; // Speed of the fall (higher = slower)
int rainTick[COLS];  // Counter to track individual column timing
uint16_t rainColor[COLS]; // Color per column (RGB565)

static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void wakeUpNeo(long dlyTime) {
  display.rectFillRAM(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x0000);
  display.setCursorRAM(0, 0);
  display.setDefaultFontColors(0xFFFF, 0x0000);
  display.print("Wake up Neo");
  delay(dlyTime);
  display.rectFillRAM(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x0000);
}

void setup() {
  Serial.begin(115200);

  // Initialize SPI and display
  SPI.begin(PIN_SCLK, -1, PIN_MOSI, PIN_CS);
  display.begin(PIN_DC, PIN_RST, PIN_CS, SPI);
  // Configure logical dimensions for this panel
  display.setWidth(SCREEN_WIDTH);
  display.setHeight(SCREEN_HEIGHT);
  display.linkDefaultFont();
  display.setDefaultFontColors(0xFFFF, 0x0000);

  // Initialize all columns with random start positions and speeds
  for (int i = 0; i < COLS; i++) {
    rainY[i] = random(-20, 0);       // Start above the screen for variance
    rainSpeed[i] = random(1, 4);     // Speed modifier
    rainTick[i] = 0;
    // Assign a random color for each column
    uint8_t r = random(50, 256);
    uint8_t g = random(50, 256);
    uint8_t b = random(50, 256);
    rainColor[i] = rgb565(r, g, b);
  }

  wakeUpNeo(3000);
}

void loop() {
  display.rectFillRAM(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x0000); // Refresh frame

  // Loop through all 21 columns to draw characters
  for (int x = 0; x < COLS; x++) {
    rainTick[x]++;
    
    // Move the column down based on its unique speed setting
    if (rainTick[x] >= rainSpeed[x]) {
      rainY[x]++;
      rainTick[x] = 0;
    }

    // Draw the trail of characters behind the leading edge
    // Changing 'j' sets how long the falling digital trail looks
    for (int j = 0; j < 5; j++) {
      int currentY = (rainY[x] - j) * 8; // Text size 1 characters are 8 pixels tall with spacing

      // Only draw if the character is visible within vertical screen boundaries
      if (currentY >= 0 && currentY < SCREEN_HEIGHT) {
        display.setCursorRAM(x * CHAR_WIDTH, currentY);
        // Set the color for this column
        display.setDefaultFontColors(rainColor[x], 0x0000);
        // Generate a random printable ASCII character
        char matrixChar = (char)random(33, 126);
        display.print(matrixChar);
      }
    }

    // If a column's trail has completely cleared the bottom, reset it to the top
    if ((rainY[x] - 5) * 8 > SCREEN_HEIGHT) {
      rainY[x] = random(-10, 0);
      rainSpeed[x] = random(1, 4);
    }
  }

  // SSD1351 draws directly; no separate `display()` call required
  delay(15);         // Controls overall frame rate (roughly 60 FPS target)
}
