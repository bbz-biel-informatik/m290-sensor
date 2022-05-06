#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

const char* serverName = "https://m290.bbz.cloud/measurements";

#include <Wire.h>
#include "SSD1306Wire.h"
#include "font.h"

#include "DHT.h"

#define DHTPIN 2 // Wemos D4 is Ardiuno GPIO2
#define DHTTYPE DHT11

#define MOISTUREDATA A0
#define MOISTUREGND 16 // Wemos D0 is Arduino GPIO16

SSD1306Wire display(0x3c, SDA, SCL, GEOMETRY_64_48);
DHT dht(DHTPIN, DHTTYPE);

bool configured = true;
int counter = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("Begin");

  pinMode(MOISTUREDATA, INPUT);

  pinMode(MOISTUREGND, OUTPUT);
  digitalWrite(MOISTUREGND, LOW);

  dht.begin();

  display.init();
  display.flipScreenVertically();

  /* Display connection information on screen */
  drawMAC();

  delay(5000);

  /* Connect to Wifi or start access point to allow configuration */
  WiFiManager wifiManager;
  String ssid = "BBZ " + WiFi.macAddress();
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.autoConnect((const char*)ssid.c_str());
}

void configModeCallback(WiFiManager *myWiFiManager) {
  configured = false;
}

void saveConfigCallback() {
  configured = true;
}

void drawMAC() {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  String ssid1 = "BBZ " + WiFi.macAddress().substring(0, 9);
  String ssid2 = WiFi.macAddress().substring(9);
  display.setFont(ArialMT_Plain_16);
  display.drawStringMaxWidth(0, 0, 64, "Connect");
  display.setFont(Dialog_plain_8);
  display.drawStringMaxWidth(0, 18, 64, (const char*)ssid1.c_str());
  display.drawStringMaxWidth(18, 28, 64, (const char*)ssid2.c_str());
  display.drawStringMaxWidth(0, 40, 64, "192.168.1.4");
  display.display();
}

void drawValues(float t, float h, float s) {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10); display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 3, "T°");
  display.setFont(ArialMT_Plain_16); display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.drawString(64, 0, String(t));
  display.setFont(ArialMT_Plain_10); display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 19, "F%");
  display.setFont(ArialMT_Plain_16); display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.drawString(64, 16, String(h));
  display.setFont(ArialMT_Plain_10); display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 35, "B%");
  display.setFont(ArialMT_Plain_16); display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.drawString(64, 32, String(s));
  display.display();
}


void loop() {
  counter++;
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  int s = 100 - (analogRead(MOISTUREDATA) - 680) / 3.44;

  if (isnan(h)) {
    h = -1;
  }
  if(isnan(t)) {
    t = -1;
  }

  if(configured) {
    drawValues(t, h, s);
  } else {
    drawMAC();
  }
  delay(1000);

  if(counter >= 5) {
    counter = 0;
    WiFiClientSecure client;
    client.setInsecure();
    client.connect("m290.bbz.cloud", 443);
    HTTPClient http;
    http.begin(client, serverName);
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST("{\"mac_address\":\"" + WiFi.macAddress() + "\",\"temperature\":" + t + ",\"humidity\":" + h + ",\"soil_moisture\":" + s + "}");
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    http.end();
  }
}
