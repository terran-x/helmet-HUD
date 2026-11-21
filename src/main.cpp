#include <Arduino.h>
#include "OLED_096_RGB.h"
#include "OLED_096_GFX.h"
#include "Network.h"

static OLED_096_RGB oled;
static OLED_096_GFX gfx(oled);

void setup() {
	Serial.begin(115200);
	delay(200);

	if (!startAccessPoint()) {
    	Serial.println("Network initialization failed");
	}

	if (!oled.begin()) {
		Serial.println("OLED initialization failed");
		return;
	}

	gfx.setRotation(1);
}

void drawGFX(uint8_t count) {
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
	gfx.printf("Connected devices: %u\n", static_cast<unsigned>(count));

	gfx.drawCircle(104, 42, 12, OLED_096_RGB::RED);
	gfx.fillCircle(104, 42, 4, OLED_096_RGB::YELLOW);
}

void loop() {
	static int displayedCount = -1;
	const uint8_t count = connectedDeviceCount();

	if (count != displayedCount) {
		drawGFX(count);
		displayedCount = count;
	}

	delay(100);
}
