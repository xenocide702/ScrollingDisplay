/**
 * This code largely stolen from https://github.com/kurimawxx00/esp32-wifi-message-board
 * Then hacked it so that it uses MQTT instead of HTTP and added ArduinoOTA
 */


#include <Arduino.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <LEDMatrixDriver.hpp>

#include "mainpage.h"
#include "font.h"
#include "mat_functions.h"

#include "credentials.h"


String message="CONNECTED!";
MDNSResponder MDNS;


void mqttCallback(char* topic, byte* payload, unsigned int length){
  char msg[length+1]; //don't forget the null
  strncpy(msg,(const char*)payload,length+1); msg[length]=0;
  message = String(msg);
  message.toUpperCase();  //font only has upper chars -_-
  Serial.printf("Got (%d):%s\r\n", length, msg);
}

WiFiClient wifiClient;
PubSubClient mqttClient(MQTT_HOST,1883,mqttCallback,wifiClient);

void setup() {
  Serial.begin(115200);
  lmd.setEnabled(true);
  lmd.setIntensity(5);

  Serial.print("Connectin to Ansible...");
  WiFi.begin(WIFI_SSID,WIFI_PASS);
  while(WiFi.status()!=WL_CONNECTED){
    Serial.print(".");
    delay(1000);
  }
  Serial.printf("OK! (%s)\r\n",WiFi.localIP().toString().c_str());

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  if(mqttClient.connect("ESP_Scroller", MQTT_USER, MQTT_PASS)){
    mqttClient.subscribe("lab/scrollingText");
  }
}

void loop() {
  ArduinoOTA.handle();
  mqttClient.loop();
  int len = message.length();
  writeToMatrix(message,len);
  lmd.display();
  delay(ANIM_DELAY);
  if(--x<len*-8){x=LEDMATRIX_WIDTH;}
  delay(20);
}