#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <EEPROM.h>

SoftwareSerial nodemcu(D6, D5);

const String firebaseHost = "greenlabso2-default-rtdb.asia-southeast1.firebasedatabase.app";
const String firebasePath = "/zxty";
const String wifiPath = "/zxty/wifi";
const long utcOffsetInSeconds = 25200;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "id.pool.ntp.org", utcOffsetInSeconds);

unsigned long timestamp;
String ssid = "user";      // Default SSID
String password = "admin123";  // Default Password

StaticJsonBuffer<1000> jsonBuffer;
int uniqueCode = 0;
float temp = 0;
float hum = 0;
float co2in = 0;
float co2out = 0;
float co2capt = 0;
float o2gen = 0;
float pm25 = 0;
float co2Capt = 0;

unsigned long Get_Epoch_Time() {
  timeClient.update();
  unsigned long now = timeClient.getEpochTime();
  return now;
}

void getWiFiCredentials() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    HTTPClient http;

    String url = "https://" + firebaseHost + wifiPath + ".json";
    client.setInsecure();
    http.begin(client, url);

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String payload = http.getString();
      Serial.print("Received payload: ");
      Serial.println(payload);

      StaticJsonBuffer<300> jsonBuffer;
      JsonObject& jsonResponse = jsonBuffer.parseObject(payload);

      if (jsonResponse.success()) {
        ssid = jsonResponse["ssid"].as<String>();
        password = jsonResponse["password"].as<String>();
        Serial.print("SSID: ");
        Serial.println(ssid);
        Serial.print("Password: ");
        Serial.println(password);

        // Save the new WiFi credentials to EEPROM
        saveWiFiCredentialsToEEPROM(ssid, password);
      } else {
        Serial.println("Failed to parse JSON");
      }
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    http.end(); // Close connection
  } else {
    Serial.println("WiFi not connected");
  }
}

void saveWiFiCredentialsToEEPROM(const String& ssid, const String& password) {
  int addr = 1; // Start at address 1 to avoid overwriting uniqueCode at address 0
  for (int i = 0; i < ssid.length(); i++) {
    EEPROM.write(addr++, ssid[i]);
  }
  EEPROM.write(addr++, '\0');  // Null-terminate the string

  for (int i = 0; i < password.length(); i++) {
    EEPROM.write(addr++, password[i]);
  }
  EEPROM.write(addr++, '\0');  // Null-terminate the string

  EEPROM.commit();
}

void loadWiFiCredentialsFromEEPROM() {
  int addr = 1;
  char ssidArr[32];
  char passArr[32];

  for (int i = 0; i < 32; i++) {
    ssidArr[i] = EEPROM.read(addr++);
    if (ssidArr[i] == '\0') break;  // Stop reading if null terminator is found
  }
  ssid = String(ssidArr);

  for (int i = 0; i < 32; i++) {
    passArr[i] = EEPROM.read(addr++);
    if (passArr[i] == '\0') break;  // Stop reading if null terminator is found
  }
  password = String(passArr);

  // Debugging information to ensure values are correctly read
  Serial.print("Loaded SSID from EEPROM: ");
  Serial.println(ssid);
  Serial.print("Loaded Password from EEPROM: ");
  Serial.println(password);
}

void setup() {
  Serial.begin(57600);
  nodemcu.begin(9600);
  while (!Serial) continue;
  EEPROM.begin(512);

  // Load WiFi credentials and unique code from EEPROM
  uniqueCode = EEPROM.read(0);
  loadWiFiCredentialsFromEEPROM();

  Serial.println("Connecting to WiFi...");

  WiFi.begin(ssid.c_str(), password.c_str());
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 10) {
    delay(1000);
    Serial.print(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to WiFi");
    getWiFiCredentials(); // Update WiFi credentials from Firebase and save them
  } else {
    Serial.println("Failed to connect to WiFi");
  }

  timeClient.begin();
  timeClient.update();
}


void loop() {
  timeClient.update();
  int time1 = timeClient.getHours();
  int min1 = timeClient.getMinutes();
  if (time1 == 7 && min1 == 2) {
    sistem();
  }
  if (time1 == 9 && min1 == 2) {
    sistem();
  }
  if (time1 == 11 && min1 == 2) {
    sistem();
  }
  if (time1 == 13 && min1 == 2) {
    sistem();
  }
  if (time1 == 15 && min1 == 2) {
    sistem();
  }
  if (time1 == 17 && min1 == 2) {
    sistem();
  } 
  if (time1 == 19 && min1 == 2) {
    sistem();
  } 
  if (time1 == 21 && min1 == 2) {
    sistem();
  }
}

void sistem() {
  StaticJsonBuffer<1000> jsonBuffer;
  JsonObject& data = jsonBuffer.parseObject(nodemcu);
  if (data == JsonObject::invalid()) {
    jsonBuffer.clear();
    return;
  }

  temp = data["data1"];
  hum = data["data2"];
  co2in = data["data3"];
  co2out = data["data4"];
  pm25 = data["data5"];
  co2capt = data["data6"];
  o2gen = data["data7"];
  timestamp = Get_Epoch_Time();

  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    HTTPClient http;

    uniqueCode++;
    EEPROM.write(0, uniqueCode);
    EEPROM.commit();

    String uniquePath = firebasePath + "/" + String(uniqueCode) + ".json";
    String url = "https://" + firebaseHost + uniquePath;

    Serial.print("Request URL: ");
    Serial.println(url);

    client.setInsecure();
    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");

    StaticJsonBuffer<200> sendBuffer;
    JsonObject& jsonToSend = sendBuffer.createObject();
    jsonToSend["uniqueCode"] = uniqueCode;
    jsonToSend["temperature"] = temp;
    jsonToSend["humidity"] = hum;
    jsonToSend["co2_in"] = co2in;
    jsonToSend["co2_out"] = co2out;
    jsonToSend["pm25"] = pm25;
    jsonToSend["co2_captured"] = co2capt;
    jsonToSend["oxygen_generated"] = o2gen;
    jsonToSend["timestamp"] = timestamp;

    String jsonData;
    jsonToSend.printTo(jsonData);


    Serial.print("Sending data: ");
    Serial.println(jsonData);

    int httpResponseCode = http.PUT(jsonData);
    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      Serial.println("Response: " + payload);
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    http.end();  // Close connection
  } else {
    Serial.println("WiFi not connected");
  }
  Serial.println("");
  jsonBuffer.clear();
  delay(10000);
}
