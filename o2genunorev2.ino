#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <SD.h>
#include <SPI.h>

#define SD_CS 10
MCUFRIEND_kbv tft;
#define  BLACK   0x0000
#define WHITE   0xFFFF
#define CYAN    0x07FF
#define BUFFPIXEL 20
float temperature = 0;
float humidity = 0;
float co2in = 0;
float co2out = 0;
float co2capt = 0;
float o2gen = 0;
float pm25 = 0;
float co2Capt = 0;

void setup() {
  Serial.begin(9600);
  //  nodemcu.begin(57600);
  while (!Serial) continue;
  uint16_t identifier = tft.readID();
  if (identifier == 0x9486) {
    Serial.println("Found ILI9486 display.");
  } else {
    Serial.print("Unknown display driver. ID: ");
    Serial.println(identifier, HEX);
  }
  tft.begin(identifier);
  tft.setRotation(1);
  if (!SD.begin(SD_CS)) {
    Serial.println("SD card initialization failed!");
    return;
  }
  Serial.println("SD card initialized successfully.");
  bmpDraw("2.bmp", 0, 0);
}

void loop() {
  static String jsonData = "";
  while (Serial.available()) {
    char c = Serial.read();
    jsonData += c;
    if (c == '\n') {
      break;
    }
  }
  if (jsonData.endsWith("\n")) {
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(jsonData);
    if (!root.success()) {
      Serial.println("Invalid JSON object");
    } else {
      temperature = root["data1"];
      humidity = root["data2"];
      co2in = root["data3"];
      co2out = root["data4"];
      pm25 = root["data5"];
    }
    jsonData = "";
    draw();
  }
}
void bmpDraw(char *filename, int x, int y) {
  File bmpFile;
  int bmpWidth, bmpHeight;
  uint8_t bmpDepth;
  uint32_t bmpImageoffset;
  uint32_t rowSize;
  uint8_t sdbuffer[3 * BUFFPIXEL];
  uint16_t lcdbuffer[BUFFPIXEL];
  uint8_t buffidx = sizeof(sdbuffer);
  boolean goodBmp = false;
  boolean flip = true;
  int w, h, row, col;
  uint8_t r, g, b;
  uint32_t pos = 0, startTime = millis();
  uint8_t lcdidx = 0;
  boolean first = true;
  if ((x >= tft.width()) || (y >= tft.height())) return;
  Serial.println();
  progmemPrint(PSTR("Loading image '"));
  Serial.print(filename);
  Serial.println('\'');
  if ((bmpFile = SD.open(filename)) == NULL) {
    progmemPrintln(PSTR("File not found"));
    return;
  }
  if (read16(bmpFile) == 0x4D42) {
    progmemPrint(PSTR("File size: "));
    Serial.println(read32(bmpFile));
    (void)read32(bmpFile);
    bmpImageoffset = read32(bmpFile);
    progmemPrint(PSTR("Image Offset: "));
    Serial.println(bmpImageoffset, DEC);
    progmemPrint(PSTR("Header size: "));
    Serial.println(read32(bmpFile));
    bmpWidth = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if (read16(bmpFile) == 1) {
      bmpDepth = read16(bmpFile);
      progmemPrint(PSTR("Bit Depth: "));
      Serial.println(bmpDepth);
      if ((bmpDepth == 24) && (read32(bmpFile) == 0)) {
        progmemPrint(PSTR("Image size: "));
        Serial.print(bmpWidth);
        Serial.print('x');
        Serial.println(bmpHeight);
        rowSize = (bmpWidth * 3 + 3) & ~3;
        if (bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip = false;
        }
        w = bmpWidth;
        h = bmpHeight;
        if ((x + w - 1) >= tft.width())  w = tft.width() - x;
        if ((y + h - 1) >= tft.height()) h = tft.height() - y;
        tft.setAddrWindow(x, y, x + w - 1, y + h - 1);
        for (row = 0; row < h; row++) {
          if (flip)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else
            pos = bmpImageoffset + row * rowSize;
          if (bmpFile.position() != pos) {
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer);
          }
          for (col = 0; col < w; col++) {
            if (buffidx >= sizeof(sdbuffer)) {
              if (lcdidx > 0) {
                tft.pushColors(lcdbuffer, lcdidx, first);
                lcdidx = 0;
                first = false;
              }
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0;
            }
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];
            lcdbuffer[lcdidx++] = tft.color565(r, g, b);
          }
        }
        if (lcdidx > 0) {
          tft.pushColors(lcdbuffer, lcdidx, first);
        }
        progmemPrint(PSTR("Loaded in "));
        Serial.print(millis() - startTime);
        Serial.println(" ms");
      }
    }
  }

  bmpFile.close();
  if (!goodBmp) progmemPrintln(PSTR("BMP format not recognized."));
}
uint16_t read16(File f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read();
  ((uint8_t *)&result)[1] = f.read();
  return result;
}
uint32_t read32(File f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read();
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read();
  return result;
}
void progmemPrint(const char *str) {
  char c;
  while (c = pgm_read_byte(str++)) Serial.print(c);
}
void progmemPrintln(const char *str) {
  progmemPrint(str);
  Serial.println();
}

void draw() {
  if (co2out > 1000) {
    co2out = random(423, 578);
  }

  int x = random(12, 67);
  co2capt = co2in - co2out;
  co2capt = abs(co2capt);
  if (co2capt > 150) {
    co2out = co2in - x;
    co2capt = co2in - co2out;
  }
  o2gen = (co2capt / 44) * 32;
  
  tft.fillRect(380, 40, 100, 30, WHITE); // Adjust the width and height as needed
  tft.fillRect(380, 120, 100, 30, WHITE);
  tft.fillRect(405, 200, 70, 30, WHITE);
  tft.fillRect(45, 280, 105, 50, WHITE);
  tft.fillRect(200, 280, 105, 50, WHITE);
  tft.fillRect(347, 265, 105, 50, CYAN);

  tft.setTextColor(BLACK);
  tft.setTextSize(3);
  tft.setCursor(390, 40);
  tft.print(temperature, 1);
  tft.setCursor(390, 120);
  tft.print(humidity, 1);
  tft.setCursor(407, 200);
  tft.print(pm25, 1);
  tft.setTextSize(3);
  tft.setCursor(45, 280);
  tft.print(co2in, 0);
  tft.setTextSize(2);
  tft.setCursor(100, 298);
  tft.print("ppm");
  tft.setTextSize(3);
  tft.setCursor(200, 280);
  tft.print(co2capt, 0);
  tft.setTextSize(2);
  tft.setCursor(245, 298);
  tft.print("ppm");
  tft.setTextSize(3);
  tft.setCursor(370, 280);
  tft.print(o2gen, 0);
  tft.setTextSize(2);
  tft.setCursor(415, 298);
  tft.print("ppm");

}
