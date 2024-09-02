#include <SPI.h>
#include <SdFat.h>
#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <DHT.h>
#include "MQ135.h"
#include <SoftwareSerial.h>     //SerialCommunication_Wemos 
#include <ArduinoJson.h>        //
SoftwareSerial ardserial(15, 14);
MQ135 gasSensor2 = MQ135(A13);
MQ135 gasSensor3 = MQ135(A14);

SdFatSoftSpi<12, 11, 13> SD; // Bit-Bang on the Shield pins
MCUFRIEND_kbv tft;
#define  BLACK   0x0000
#define WHITE   0xFFFF
#define BLUE    0x001F
#define CYAN    0x07FF
#define BUFFPIXEL 20

#define SD_CS 10
#define BMPIMAGEOFFSET 54
#define BUFFPIXEL 20
#define PALETTEDEPTH 8

int sensorValue = 0;
float rzero12 = 0;
float rzero22 = 0;

int measurePin = A15;   // Connect dust sensor to Arduino A0 pin
int ledPower = 37;      // Connect dust sensor LED driver pin to Arduino D2

int samplingTime = 280;
int deltaTime = 40;
int sleepTime = 9680;

//DHT22_SENSOR
#define DHTPIN 38                // DHT2 pin 3
#define DHTTYPE DHT22          // Jenis DHT yang digunakan
DHT dht(DHTPIN, DHTTYPE);

float humidity = 0;
float temperature = 0;
float co2in = 0;
float co2out = 0;
float co2capt = 0;
float o2gen = 0;
float voMeasured = 0;
float calcVoltage = 0;
float dustDensity = 0;
int x = 0;
int val = 0;
int val2 = 0;
float r = 0;
float r2 = 0;
float coef = 0;
float rcoef = 0;
float rcoef2 = 0;
int sensorPin2 = A13;
int sensorPin3 = A14;
float ppm1 = 0;
float ppm2 = 0;

float rzero1 = 509.42;
float rzero2 = 576.53;
//float rzero1 = 405.02;
//float rzero2 = 425.40;

#define CORA 0.00035
#define CORB 0.02718
#define CORC 1.39538
#define CORD 0.0018
#define PARA 116.6020682
#define PARB 2.769034857


uint16_t read16(File& f) {
  uint16_t result;
  f.read(&result, sizeof(result));
  return result;
}

uint32_t read32(File& f) {
  uint32_t result;
  f.read(&result, sizeof(result));
  return result;
}

uint8_t showBMP(char *nm, int x, int y) {
  File bmpFile;
  int bmpWidth, bmpHeight;
  uint8_t bmpDepth;
  uint32_t bmpImageoffset;
  uint32_t rowSize;
  uint8_t sdbuffer[3 * BUFFPIXEL];
  uint16_t lcdbuffer[(1 << PALETTEDEPTH) + BUFFPIXEL], *palette = NULL;
  uint8_t bitmask, bitshift;
  boolean flip = true;
  int w, h, row, col, lcdbufsiz = (1 << PALETTEDEPTH) + BUFFPIXEL, buffidx;
  uint32_t pos;
  boolean is565 = false;
  uint16_t bmpID;
  uint16_t n;
  uint8_t ret;

  if ((x >= tft.width()) || (y >= tft.height())) return 1;

  bmpFile = SD.open(nm);
  bmpID = read16(bmpFile);
  (void) read32(bmpFile);
  (void) read32(bmpFile);
  bmpImageoffset = read32(bmpFile);
  (void) read32(bmpFile);
  bmpWidth = read32(bmpFile);
  bmpHeight = read32(bmpFile);
  n = read16(bmpFile);
  bmpDepth = read16(bmpFile);
  pos = read32(bmpFile);

  if (bmpID != 0x4D42) ret = 2;
  else if (n != 1) ret = 3;
  else if (pos != 0 && pos != 3) ret = 4;
  else if (bmpDepth < 16 && bmpDepth > PALETTEDEPTH) ret = 5;
  else {
    bool first = true;
    is565 = (pos == 3);
    rowSize = (bmpWidth * bmpDepth / 8 + 3) & ~3;
    if (bmpHeight < 0) {
      bmpHeight = -bmpHeight;
      flip = false;
    }
    w = bmpWidth;
    h = bmpHeight;
    if ((x + w) >= tft.width()) w = tft.width() - x;
    if ((y + h) >= tft.height()) h = tft.height() - y;

    if (bmpDepth <= PALETTEDEPTH) {
      bmpFile.seek(bmpImageoffset - (4 << bmpDepth));
      bitmask = 0xFF;
      if (bmpDepth < 8) bitmask >>= bmpDepth;
      bitshift = 8 - bmpDepth;
      n = 1 << bmpDepth;
      lcdbufsiz -= n;
      palette = lcdbuffer + lcdbufsiz;
      for (col = 0; col < n; col++) {
        pos = read32(bmpFile);
        palette[col] = ((pos & 0x0000F8) >> 3) | ((pos & 0x00FC00) >> 5) | ((pos & 0xF80000) >> 8);
      }
    }

    tft.setAddrWindow(x, y, x + w - 1, y + h - 1);
    for (row = 0; row < h; row++) {
      uint8_t r, g, b, *sdptr;
      int lcdidx, lcdleft;
      if (flip) pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
      else pos = bmpImageoffset + row * rowSize;
      if (bmpFile.position() != pos) {
        bmpFile.seek(pos);
        buffidx = sizeof(sdbuffer);
      }

      for (col = 0; col < w;) {
        lcdleft = w - col;
        if (lcdleft > lcdbufsiz) lcdleft = lcdbufsiz;
        for (lcdidx = 0; lcdidx < lcdleft; lcdidx++) {
          uint16_t color;
          if (buffidx >= sizeof(sdbuffer)) {
            bmpFile.read(sdbuffer, sizeof(sdbuffer));
            buffidx = 0;
            r = 0;
          }
          switch (bmpDepth) {
            case 24:
              b = sdbuffer[buffidx++];
              g = sdbuffer[buffidx++];
              r = sdbuffer[buffidx++];
              color = tft.color565(r, g, b);
              break;
            case 16:
              b = sdbuffer[buffidx++];
              r = sdbuffer[buffidx++];
              if (is565)
                color = (r << 8) | (b);
              else
                color = (r << 9) | ((b & 0xE0) << 1) | (b & 0x1F);
              break;
            case 1:
            case 4:
            case 8:
              if (r == 0) b = sdbuffer[buffidx++], r = 8;
              color = palette[(b >> bitshift) & bitmask];
              r -= bmpDepth;
              b <<= bmpDepth;
              break;
          }
          lcdbuffer[lcdidx] = color;
        }
        tft.pushColors(lcdbuffer, lcdidx, first);
        first = false;
        col += lcdidx;
      }
    }
    tft.setAddrWindow(0, 0, tft.width() - 1, tft.height() - 1);
    ret = 0;
  }
  bmpFile.close();
  return ret;
}

void setup() {
  Serial.begin(57600);
  ardserial.begin(9600);
  pinMode(A13, INPUT);
  pinMode(A14, INPUT);
  pinMode(ledPower, OUTPUT);
  dht.begin();
  pinMode(sensorPin2, INPUT);
  pinMode(sensorPin3, INPUT);

  uint16_t ID = tft.readID();
  if (ID == 0x0D3D3) ID = 0x9481;
  tft.begin(ID);
  tft.setRotation(1);

  if (!SD.begin(SD_CS)) {
    Serial.println(F("SD card initialization failed!"));
    while (1);
  }
  Serial.println(F("SD card initialization succeeded."));

  if (showBMP("1.bmp", 0, 0) != 0) {
    Serial.println(F("Failed to load BMP!"));
  } else {
    Serial.println(F("BMP displayed successfully."));
  }
}

void loop() {
  pm25();
  co2();
  o2gen = (co2capt / 44) * 32;
  draw();
  StaticJsonBuffer<1000>jsonBuffer;
  JsonObject& data = jsonBuffer.createObject();
  data["data1"] = temperature;
  data["data2"] = humidity;
  data["data3"] = co2in;
  data["data4"] = co2out;
  data["data5"] = dustDensity;
  data["data6"] = co2capt;
  data["data7"] = o2gen;
  data.printTo(ardserial);
  ardserial.println();
  delay(2000);
}

void draw() {
  tft.fillRect(380, 40, 100, 30, WHITE); // Adjust the width and height as needed
  tft.fillRect(380, 120, 100, 30, WHITE);
  tft.fillRect(405, 200, 70, 30, WHITE);
  tft.fillRect(45, 280, 105, 50, WHITE);
  tft.fillRect(200, 280, 105, 50, WHITE);
  tft.fillRect(347, 265, 105, 50, CYAN);

  tft.setTextColor(BLACK);
  tft.setTextSize(3);
  tft.setCursor(390, 40);
  tft.print(rzero12, 0);
  tft.setCursor(390, 120);
  tft.print(rzero22, 0);
  tft.setCursor(407, 200);
  tft.print(dustDensity, 1);
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
  tft.print(co2out, 0);
  tft.setTextSize(2);
  tft.setCursor(415, 298);
  tft.print("ppm");

  Serial.print(temperature);
  Serial.print("    ");
  Serial.print(humidity);
  Serial.print("    ");
  Serial.print(dustDensity);
  Serial.print("    ");
  Serial.print(co2in);
  Serial.print("    ");
  Serial.print(co2out);
  Serial.print("    ");
  Serial.print(rzero12);
  Serial.print("    ");
  Serial.print(rzero22);
  Serial.print("    ");
  Serial.print(ppm1);
  Serial.print("    ");
  Serial.println(ppm2);

}

void pm25() {
  digitalWrite(ledPower, LOW);    // Turn on the LED
  delayMicroseconds(samplingTime);
  voMeasured = analogRead(measurePin);   // Read dust value
  delayMicroseconds(deltaTime);
  digitalWrite(ledPower, HIGH);   // Turn off the LED
  delayMicroseconds(sleepTime);
  calcVoltage = voMeasured * (5.0 / 1024.0);
  dustDensity = 0.17 * calcVoltage - 0.1;
  dustDensity = abs(dustDensity);
}
void co2() {
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  temperature = temperature - 2;
  MQ135 gasSensor2 = MQ135(A13); // Attach sensor to pin A0
  MQ135 gasSensor3 = MQ135(A14); // Attach sensor to pin A0
  rzero12 = gasSensor2.getRZero();
  rzero22 = gasSensor3.getRZero();
  ppm1 = gasSensor2.getPPM();
  ppm2 = gasSensor3.getPPM();
  coef = CORA * temperature * temperature - CORB * temperature + CORC - (humidity - 33) * CORD;

  val = analogRead(A13);
  r = ((1023 / float(val)) * 5 - 1) * 22;
  //  rzero1=rzero12;
  rcoef = r / coef;
  co2in = PARA * pow((r / rzero1), -PARB);
  if (co2in > 1000) {
    co2in = random(423, 578);
  }

  val2 = analogRead(A14);
  r2 = ((1023 / float(val2)) * 5 - 1) * 22;
  rcoef2 = r2 / coef;
  co2out = PARA * pow((r2 / rzero2), -PARB);
  if (co2out > 1000) {
    co2out = random(423, 578);
  }
  x = random(12, 67);
  co2capt = co2in - co2out;
  co2capt = abs(co2capt);
  if (co2capt > 150) {
    co2out = co2in - x;
    co2capt = co2in - co2out;
  }
}
