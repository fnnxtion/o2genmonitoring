#include <Adafruit_GFX.h>        // Core graphics library
#include <MCUFRIEND_kbv.h>       // Hardware-specific library
#include <SD.h>
#include <SPI.h>

#define  BLACK   0x0000
#define WHITE   0xFFFF
// Define pin connections
#define SD_CS 10  // Set the chip select line for the SD card

MCUFRIEND_kbv tft;

void setup() {
  Serial.begin(9600);
  uint16_t identifier = tft.readID();
  if (identifier == 0x9486) {
    Serial.println("Found ILI9486 display.");
  } else {
    Serial.print("Unknown display driver. ID: ");
    Serial.println(identifier, HEX);
  }

  tft.begin(identifier);
  tft.setRotation(1); // Set the rotation for landscape mode
  tft.fillScreen(BLACK); // Clear the screen with black color

  // Initialize SD card
  if (!SD.begin(SD_CS)) {
    Serial.println("SD card initialization failed!");
    return;
  }
  Serial.println("SD card initialized successfully.");
}

void loop() {
  // Display the background image
  bmpDraw("7.bmp", 0, 0);

  // Dummy sensor values
  float o2Prod = 20;       // O2 production value
  float co2Capt = 1250;    // CO2 capture value
  float temperature = 24;  // Temperature value
  float humidity = 80;     // Humidity value
  float pm25 = 300;        // PM2.5 value

  // Set text color and size
  tft.setTextColor(WHITE); // White color
  tft.setTextSize(2);

  // Draw the RPM-like gauge
  drawGauge(temperature);

  // Draw dividing lines with a thickness of 3 pixels
  drawBoldLine(200, 0, 200, 190, WHITE, 2); // Vertical line
  drawBoldLine(200, 60, 320, 60, WHITE, 2); // Horizontal line
  drawBoldLine(200, 120, 320, 120, WHITE, 2); // Horizontal line
  drawBoldLine(0, 190, 320, 190, WHITE, 2); // Horizontal line

  // Display humidity
  tft.setCursor(240, 10);
  tft.print("HUM ");
  tft.setTextColor(RED); // Red color for temperature
  tft.setCursor(230, 30);
  tft.print(humidity, 1);
  tft.println("%");
  tft.setTextColor(WHITE); // Reset to white color

  // Display temperature
  tft.setCursor(240, 70);
  tft.print("TEMP ");
  tft.setTextColor(RED); // Red color for temperature
  tft.setCursor(230, 90);
  tft.print(temperature, 1);
  tft.println(" C");

  tft.setTextColor(WHITE); // Reset to white color

  // Display PM2.5
  tft.setCursor(230, 140);
  tft.print("PM2.5 ");
  tft.setCursor(210, 160);
  tft.setTextColor(RED); // Red color for temperature
  tft.print(pm25, 1);
  tft.println(" ppm");
  tft.setTextColor(WHITE); // Reset to white color

  // Display CO2 production
  tft.setCursor(20, 210);
  tft.print("CO2 Prod ");
  tft.print(co2Capt, 1);
  tft.print(" ppm");

  delay(2000); // Update every 2 seconds
}

void drawGauge(float temperature) {
  // Draw RPM-like gauge (placeholder, you can customize as needed)
  int gaugeRadius = 50;
  int gaugeCenterX = 80;
  int gaugeCenterY = 130;

  // Draw gauge background
  tft.fillCircle(gaugeCenterX, gaugeCenterY, gaugeRadius, BLUE); // Blue color
  tft.fillCircle(gaugeCenterX, gaugeCenterY, gaugeRadius - 10, WHITE); // White color

  // Draw gauge needle
  float angle = map(temperature, 0, 50, -120, 120);
  float radian = angle * PI / 180;
  int needleLength = gaugeRadius - 15;
  int needleX = gaugeCenterX + needleLength * cos(radian);
  int needleY = gaugeCenterY + needleLength * sin(radian);
  tft.drawLine(gaugeCenterX, gaugeCenterY, needleX, needleY, BLACK); // Black color

  // Display temperature value inside gauge
  tft.setCursor(gaugeCenterX - 10, gaugeCenterY - 5);
  tft.setTextColor(BLACK); // Black color
  tft.setTextSize(2);
  tft.print((int)temperature);
}

void drawBoldLine(int x0, int y0, int x1, int y1, uint16_t color, int thickness) {
  for (int i = 0; i < thickness; i++) {
    tft.drawLine(x0, y0 + i, x1, y1 + i, color);
    tft.drawLine(x0 + i, y0, x1 + i, y1, color);
  }
}

#define BUFFPIXEL 20

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

  if (read16(bmpFile) == 0x4D42) { // BMP signature
    progmemPrint(PSTR("File size: "));
    Serial.println(read32(bmpFile));
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    progmemPrint(PSTR("Image Offset: "));
    Serial.println(bmpImageoffset, DEC);
    progmemPrint(PSTR("Header size: "));
    Serial.println(read32(bmpFile));
    bmpWidth = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if (read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      progmemPrint(PSTR("Bit Depth: "));
      Serial.println(bmpDepth);
      if ((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

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

        for (row = 0; row < h; row++) { // For each scanline...
          if (flip) // Bitmap is stored bottom-to-top order (normal BMP)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else     // Bitmap is stored top-to-bottom
            pos = bmpImageoffset + row * rowSize;
          if (bmpFile.position() != pos) { // Need seek?
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer); // Force buffer reload
          }

          for (col = 0; col < w; col++) { // For each column...
            // Time to read more pixel data?
            if (buffidx >= sizeof(sdbuffer)) {
              // Push LCD buffer to the display first
              if (lcdidx > 0) {
                tft.pushColors(lcdbuffer, lcdidx, first);
                lcdidx = 0;
                first = false;
              }
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0; // Set index to beginning
            }

            // Convert pixel from BMP to TFT format
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];
            lcdbuffer[lcdidx++] = tft.color565(r, g, b);
          }
        }
        // Write any remaining data to LCD
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
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
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
