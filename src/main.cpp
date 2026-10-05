#include <Arduino.h>
#include "OLED_096_RGB.h"
#include "OLED_096_GFX.h"

static OLED_096_RGB oled;
static OLED_096_GFX gfx(oled);

void setup() {
	Serial.begin(115200);
	delay(200);

	if (!oled.begin()) {
		Serial.println("OLED initialization failed");
		return;
	}

	gfx.setRotation(1);
	gfx.fillScreen(OLED_096_RGB::BLACK);
	gfx.drawRect(0, 0, gfx.width(), gfx.height(), OLED_096_RGB::WHITE);
	gfx.fillRect(4, 4, gfx.width() - 8, 18, OLED_096_RGB::BLUE);

	gfx.setTextSize(1);
	gfx.setTextColor(OLED_096_RGB::WHITE, OLED_096_RGB::BLUE);
	gfx.setCursor(8, 10);
	gfx.print("HELMET HUD");

	gfx.setTextColor(OLED_096_RGB::GREEN, OLED_096_RGB::BLACK);
	gfx.setCursor(8, 32);
	gfx.println("System ready");
	gfx.setCursor(8, 44);
	gfx.println("OLED 128x64");

	gfx.drawCircle(104, 42, 12, OLED_096_RGB::RED);
	gfx.fillCircle(104, 42, 4, OLED_096_RGB::YELLOW);

	Serial.println("Helmet HUD display ready");
}

void loop() {
}
