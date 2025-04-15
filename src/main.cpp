#include "../include/header.hpp"
#include "tasks/tasks.hpp"

void setup() {
  Serial.begin(SERIAL_DEBUG_BAUD);

  xTaskCreate(connectWifi, "Connect Wifi", 4096, NULL, 1, NULL);
  xTaskCreate(connectThingsBoard, "Connect ThingsBoard", 4096, NULL, 1, NULL);
  // xTaskCreate(sendTelemetryData, "Send Telementry", 2048, NULL, 2, NULL);
  xTaskCreate(OTAupdate, "OTA update",1024 * 20, NULL, 2, NULL);
}

void loop() {
}
