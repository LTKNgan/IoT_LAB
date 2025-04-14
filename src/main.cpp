#include "../include/header.hpp"
#include "tasks/tasks.hpp"

void setup() {
  Serial.begin(SERIAL_DEBUG_BAUD);

  xTaskCreate(connectWifi, "Connect Wifi", 4096, NULL, 1, NULL);
  xTaskCreate(connectThingsBoard, "Connect ThingsBoard", 4096, NULL, 1, NULL);
  // xTaskCreate(subscribeRPC, "Subcribe RPC", 2048, NULL, 2, NULL);
  xTaskCreate(sendTelemetryData, "Send Telementry", 2048, NULL, 2, NULL);
  xTaskCreate(OTAupdate, "OTA_SD_TO_FLASH", FIRMWARE_PACKET_SIZE + 1024 * 1, NULL, 2, NULL);
}

void loop() {
}
