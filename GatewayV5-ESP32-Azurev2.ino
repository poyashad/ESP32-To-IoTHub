/*
   Gateway - Adafruit Feather ESP32

   Purpose:
   Acts as Master Node and receives data from 2.4 gHz Network Mesh Nodes.
   Converts the data received from a node to a JSON package.
   JSON string gets sent to a MQTT Server.

   Use:
   Configure Credentials and Radio Setup in NetworkManager.h


   Version 4 15/2-2018
   Created by Poja Shad & Arthur Payne
   FINAL VERSION 


*/
#include <SPI.h>
#include "NetworkManager.h"

NetworkManager networkManager;

uint32_t displayTimer = 0;

void setup() {
 Serial.begin(115200);
  delay(2000);
  networkManager.init(); // 1.Set Master Node/Starts Mesh - 2.Connects to WiFi- 3.Starts Timeclient - 4.Connects To Azure

}

void loop() {

  // Reconnect to WiFi if it drops.
 if ( WiFi.status() != WL_CONNECTED ) {
    Serial.println(networkManager.timeClient.getFormattedTime());
    networkManager.connectWiFi();
    networkManager.connectCloud();
  }
  networkManager.updateTime(); // Get fresh/updated time from server

  networkManager.updateMesh();

  networkManager.updateDHCP();

  // If the Master Node is connected to the RF Mesh, receive payload and send to cloud
  if (networkManager.network.available()) {
    networkManager.sendPayloadToCloud();

  }

  if (millis() - displayTimer > 3000) {
    displayTimer = millis();
    networkManager.printInfo();

  }

}
