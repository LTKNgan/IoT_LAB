#define LED_PIN GPIO_NUM_12
#define SDA_PIN GPIO_NUM_21
#define SCL_PIN GPIO_NUM_22

#include <WiFi.h>
#include <Arduino_MQTT_Client.h>
#include <ThingsBoard.h>
#include "DHT20.h"
#include "Wire.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <ArduinoJson.h> 
#include <OTA_Firmware_Update.h>
#include <Shared_Attribute_Update.h>
#include <Attribute_Request.h>
#include <Espressif_Updater.h>

// Wifi parameter
constexpr char WIFI_SSID[] = "KN";
constexpr char WIFI_PASSWORD[] = "nganxinhdep";

// Thingsboard parameter
constexpr char TOKEN[] = "AhbeGa4zPtrjwGYjomrq";
constexpr char THINGSBOARD_SERVER[] = "app.coreiot.io";
constexpr uint16_t THINGSBOARD_PORT = 1883U;
constexpr uint32_t MAX_MESSAGE_SIZE = 1024U;

constexpr uint32_t SERIAL_DEBUG_BAUD = 115200U;

// OTA update parameter
constexpr char CURRENT_FIRMWARE_TITLE[] = "firmware";
constexpr char CURRENT_FIRMWARE_VERSION[] = "1.0.0";
// Maximum amount of retries we attempt to download each firmware chunck over MQTT
constexpr uint8_t FIRMWARE_FAILURE_RETRIES = 12U;
// Size of each firmware chunck downloaded over MQTT,
// increased packet size, might increase download speed
constexpr uint16_t FIRMWARE_PACKET_SIZE = 4096U;
constexpr uint16_t MAX_MESSAGE_SEND_SIZE = FIRMWARE_PACKET_SIZE + 50U;
constexpr uint16_t MAX_MESSAGE_RECEIVE_SIZE = FIRMWARE_PACKET_SIZE + 50U;
