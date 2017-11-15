#include "SSD1306.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

HTTPClient http;

const char* ssid = "WIFI NAME";
const char* password = "WIFI PASSWORD";

String lights[64];
String lightName[64];
int light = 0;

int switchButton = 25;
bool switchButtonState = true;
long switchMillis = 0;

int toggleButton = 26;
bool toggleButtonState = true;
long toggleMillis = 0;

SSD1306 display(0x3c, 5, 4);

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  http.begin("http://IP_ADDRESS_OF_BRIDGE/api/API_KEY/lights");
  int httpCode = http.GET();

  if (httpCode > 0) {
    String json = http.getString();
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(json);

    for (JsonObject::iterator it = root.begin(); it != root.end(); ++it) {
      String key = it->key;
      String name = root[key]["name"];
      lightName[light] = key;
      lights[light] = name;
      light++;
      Serial.println("Found: " + name);
    }
    light = 0;

    display.init();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    //display.flipScreenVertically();
    display.setColor(WHITE);
    display.drawString(64, 14, lights[light]);
    display.display();
  } else {
    Serial.println("Error on HTTP request");
  }

  pinMode(switchButton, INPUT);
  pinMode(toggleButton, INPUT);

  http.end();
}

void loop() {
  if (!digitalRead(switchButton)) {
    long currentMillis = millis();
    if (currentMillis - switchMillis >= 1000) {
      switchMillis = currentMillis;

      if (switchButtonState) {
        switchButtonState = false;
      } else {
        switchButtonState = true;
      }
      change();
    }
  }

  if (!digitalRead(toggleButton)) {
    long currentMillis = millis();
    if (currentMillis - switchMillis >= 1000) {
      switchMillis = currentMillis;

      if (toggleButtonState) {
        toggleButtonState = false;
      } else {
        toggleButtonState = true;
      }
      toggle();
    }
  }
}

void change() {
  light++;
  if (lights[light] == "") {
    light = 0;
  }
  display.clear();
  display.drawString(64, 14, lights[light]);
  display.display();
  Serial.println("Set display to: " + lights[light]);
}

void toggle() {
  http.begin("http://IP_ADDRESS_OF_BRIDGE/api/API_KEY/lights/" + lightName[light]);
  String json = http.getString();
  int httpCode = http.GET();

  if (httpCode > 0) {
    String json = http.getString();
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(json);

    String state = root["state"]["on"];
    Serial.println(state);
    String realState;
    if (state == "true") {
      realState = "false";
    } else {
      realState = "true";
    }
    http.end();

    http.begin("http://IP_ADDRESS_OF_BRIDGE/api/API_KEY/lights/" + lightName[light] + "/state");
    http.addHeader("Content-Type", "text/plain");
    int responseCode = http.PUT("{\"on\": " + realState + "}");

    if (responseCode > 0) {
      String response = http.getString();
      Serial.println(responseCode);
      Serial.println(response);
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(responseCode);
    }
    http.end();
  }
}
