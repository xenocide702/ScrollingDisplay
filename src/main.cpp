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

#include "webServerHelpers.h"

#include "credentials.h"

#define BUTTON_ON_PIN 25
#define BUTTON_OFF_PIN 27


String message="CONNECTED!";
MDNSResponder MDNS;


bool ledThrob[5];
const uint8_t leds[] = {2,23,18,5,15};
const int16_t buttons[] = {4095, 3050,2200, 1400,610};
const uint8_t adcIn=35;

bool up = true;
int16_t brightness;

WebServer server(80);

void mqttCallback(char* topic, byte* payload, unsigned int length);
WiFiClient wifiClient;
PubSubClient mqttClient(MQTT_HOST,1883,mqttCallback,wifiClient);

void mqttCallback(char* topic, byte* payload, unsigned int length){
  
  if(strcmp("lab/keypad/lights",topic)==0){
    
    for(int i=0;i<5;i++){
      String strPayload = String((const char*)payload);
      String str = "\"button\":" + String(i);
      if(strPayload.indexOf(str)!=-1){
        if(strPayload.indexOf("throb")!=-1){
          ledThrob[i] = true;
          ledcAttachPin(leds[i],0);
        }
        else{
          ledThrob[i] = false;
          ledcDetachPin(leds[i]);
          digitalWrite(leds[i],LOW);
        }
        break;
      }
    }
  }
  
}


void setup() {
  Serial.begin(115200);

  Serial.print("Connectin to Ansible...");
  WiFi.begin(WIFI_SSID,WIFI_PASS);
  while(WiFi.status()!=WL_CONNECTED){
    Serial.print(".");
    delay(1000);
  }
  Serial.printf("OK! (%s)\r\n",WiFi.localIP().toString().c_str());

  Serial.print("Starting http...");
  initHttpServer();
  Serial.println("Done!");

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

  if(mqttClient.connect("espKeypad", MQTT_USER, MQTT_PASS)){
    Serial.println("MQTT Server connected!");
    mqttClient.subscribe("lab/keypad/lights");
    //mqttClient.subscribe("lab/scrollingText");
    //mqttClient.subscribe("lab/lights");

  }
  pinMode(BUTTON_ON_PIN,INPUT_PULLUP);
  pinMode(BUTTON_OFF_PIN,INPUT_PULLUP);


  ledcSetup(0,4000,8);
  for(int i=0;i<5;i++){
    ledcAttachPin(leds[i],0);
  }
  pinMode(adcIn,ANALOG);
}
uint8_t analPins[4] = {36,39,34,35};
void readButtons(){
  uint16_t vals[4];
  for(int i=0;i<4;i++){
    vals[i] = analogRead(analPins[i]);
    Serial.printf("[%d]: %d\r\n",i,vals[i]);
  }
  char strVals[32];
  snprintf(strVals,32,"[%d,%d,%d,%d]",vals[0],vals[1],vals[2],vals[3]);
  mqttClient.publish("lab/sliders",strVals);

}
bool buttonOn,buttonOff;
int ledIndex=0;
void loop() {
  if(!mqttClient.connected()){
    Serial.print("Reconnecting...");
    if(mqttClient.connect("ESP_sliders", MQTT_USER, MQTT_PASS)){
      mqttClient.subscribe("lab/keypad/lights");
      Serial.println("Done!");
    }

  }
  ArduinoOTA.handle();
  mqttClient.loop();
  server.handleClient();

  ledcWrite(0,brightness);
  if(up%2==1){brightness+=5;if(brightness>=250){up=false;}}
  else{brightness-=5;if(brightness<=50){up=true;}}

  for(int i=0;i<4;i++){pinMode(analPins[i],ANALOG);}
  delay(1000);
  readButtons();
}