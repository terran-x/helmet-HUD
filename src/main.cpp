#include <Arduino.h>
#include <WiFi.h>
#include <JPEGDEC.h>
#include <string.h>
#include "OLED_096_RGB.h"
#include "Network.h"

static OLED_096_RGB oled;
static JPEGDEC jpeg;

static const IPAddress CAM_IP(192, 168, 4, 2);
static constexpr uint16_t CAM_PORT = 81;
static constexpr int SRC_W = 160;
static constexpr int SRC_H = 120;
static constexpr int OUT_W = 128;
static constexpr int OUT_H = 64;
static constexpr int CROP_Y = (SRC_H - SRC_W * OUT_H / OUT_W) / 2;
static constexpr size_t MAX_JPG = 32768;

static uint16_t panelBuffer[64 * 128];
static uint8_t jpegBuffer[MAX_JPG];

class HttpChunkedReader {
public:
	explicit HttpChunkedReader(WiFiClient &client) : _client(client) {}

	int readByte() {
		if (_finished) return -1;
		if (_chunkRemaining == 0 && !nextChunk()) return -1;

		uint8_t value;
		if (_client.readBytes(&value, 1) != 1) return -1;
		--_chunkRemaining;
		return value;
	}

	bool readExact(uint8_t *buffer, size_t length) {
		size_t copied = 0;
		while (copied < length) {
			if (_finished) return false;
			if (_chunkRemaining == 0 && !nextChunk()) return false;

			const size_t available = length - copied;
			const size_t count = available < _chunkRemaining ? available : _chunkRemaining;
			const size_t received = _client.readBytes(buffer + copied, count);
			if (received == 0) return false;
			copied += received;
			_chunkRemaining -= received;
		}
		return true;
	}

	bool readLine(char *line, size_t capacity) {
		if (capacity == 0) return false;
		size_t length = 0;
		while (length + 1 < capacity) {
			const int value = readByte();
			if (value < 0) return false;
			if (value == '\n') {
				if (length > 0 && line[length - 1] == '\r') --length;
				line[length] = '\0';
				return true;
			}
			line[length++] = static_cast<char>(value);
		}
		line[length] = '\0';
		return false;
	}

private:
	WiFiClient &_client;
	size_t _chunkRemaining = 0;
	bool _needChunkTerminator = false;
	bool _finished = false;

	bool readRawLine(char *line, size_t capacity) {
		size_t length = 0;
		while (length + 1 < capacity) {
			uint8_t value;
			if (_client.readBytes(&value, 1) != 1) return false;
			if (value == '\n') {
				if (length > 0 && line[length - 1] == '\r') --length;
				line[length] = '\0';
				return true;
			}
			line[length++] = static_cast<char>(value);
		}
		return false;
	}

	bool nextChunk() {
		if (_needChunkTerminator) {
			uint8_t terminator[2];
			if (_client.readBytes(terminator, sizeof(terminator)) != sizeof(terminator) ||
				terminator[0] != '\r' || terminator[1] != '\n') return false;
			_needChunkTerminator = false;
		}

		char line[32];
		if (!readRawLine(line, sizeof(line))) return false;
		char *end = nullptr;
		const unsigned long chunkSize = strtoul(line, &end, 16);
		if (end == line) return false;
		if (chunkSize == 0) {
			_finished = true;
			return false;
		}

		_chunkRemaining = chunkSize;
		_needChunkTerminator = true;
		return true;
	}
};

static bool readResponseHeaders(WiFiClient &client) {
	char line[128];
	const size_t statusLength = client.readBytesUntil('\n', line, sizeof(line) - 1);
	if (statusLength == 0) return false;
	line[statusLength] = '\0';
	if (strncmp(line, "HTTP/1.1 200", 12) != 0 &&
		strncmp(line, "HTTP/1.0 200", 12) != 0) return false;

	while (client.connected()) {
		const size_t length = client.readBytesUntil('\n', line, sizeof(line) - 1);
		if (length == 0) return false;
		line[length] = '\0';
		if (strcmp(line, "\r") == 0 || line[0] == '\0') return true;
	}
	return false;
}

static bool readJpegFrame(HttpChunkedReader &body, size_t &jpegLength) {
	char line[96];
	jpegLength = 0;
	bool foundLength = false;

	while (body.readLine(line, sizeof(line))) {
		if (strncmp(line, "Content-Length:", 15) == 0) {
			const unsigned long length = strtoul(line + 15, nullptr, 10);
			if (length < 4 || length > MAX_JPG) return false;
			jpegLength = length;
			foundLength = true;
		} else if (line[0] == '\0' && foundLength) {
			return body.readExact(jpegBuffer, jpegLength);
		}
	}
	return false;
}

static int onJpegDraw(JPEGDRAW *draw) {
	constexpr int cropHeight = SRC_H - 2 * CROP_Y;
	for (int y = 0; y < draw->iHeight; ++y) {
		const int srcY = draw->y + y;
		if (srcY < CROP_Y || srcY >= SRC_H - CROP_Y) continue;
		const int cropY = srcY - CROP_Y;
		const int outY = (cropY * OUT_H + cropHeight - 1) / cropHeight;
		if (outY >= OUT_H || outY * cropHeight / OUT_H != cropY) continue;
		const int nativeX = OUT_H - 1 - outY;
		for (int x = 0; x < draw->iWidth; ++x) {
			const int srcX = draw->x + x;
			if (srcX < 0 || srcX >= SRC_W) continue;
			const int outX = (srcX * OUT_W + SRC_W - 1) / SRC_W;
			if (outX >= OUT_W || outX * SRC_W / OUT_W != srcX) continue;
			panelBuffer[outX * 64 + nativeX] = draw->pPixels[y * draw->iWidth + x];
		}
	}
	return 1;
}

static void presentFrame() {
	oled.blit(panelBuffer);
}

static bool openCameraStream(WiFiClient &client) {
	if (!client.connect(CAM_IP, CAM_PORT, 2000)) return false;
	client.setTimeout(3000);
	client.print("GET /stream HTTP/1.1\r\nHost: 192.168.4.2\r\nConnection: keep-alive\r\n\r\n");
	if (!readResponseHeaders(client)) {
		client.stop();
		return false;
	}
	return true;
}

void setup() {
	Serial.begin(115200);
	delay(200);

	if (!startAccessPoint()) {
		Serial.println("Network initialization failed");
		return;
	}
	if (!oled.begin()) {
		Serial.println("OLED initialization failed");
		return;
	}

	oled.clear(OLED_096_RGB::BLACK);
	Serial.println("HUD hub ready; waiting for camera at 192.168.4.2");
}

void loop() {
	WiFiClient camera;
	if (!openCameraStream(camera)) {
		delay(500);
		return;
	}

	Serial.println("Camera stream connected");
	HttpChunkedReader responseBody(camera);
	uint32_t frames = 0;
	uint32_t startedAt = millis();

	while (camera.connected()) {
		size_t jpegLength;
		if (!readJpegFrame(responseBody, jpegLength)) break;

		memset(panelBuffer, 0, sizeof(panelBuffer));
		if (jpeg.openRAM(jpegBuffer, static_cast<int>(jpegLength), onJpegDraw)) {
			jpeg.setPixelType(RGB565_LITTLE_ENDIAN);
			jpeg.decode(0, 0, 0);
			jpeg.close();
			presentFrame();
		}

		if (++frames % 30 == 0) {
			const uint32_t elapsed = millis() - startedAt;
			Serial.printf("Display: %.1f fps\n", elapsed ? 30000.0f / elapsed : 0.0f);
			startedAt = millis();
		}
	}

	camera.stop();
	Serial.println("Camera stream lost; retrying");
}
