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

// Wifi parameter
constexpr char WIFI_SSID[] = "KN";
constexpr char WIFI_PASSWORD[] = "nganxinhdep";

// Thingsboard parameter
constexpr char TOKEN[] = "mgbDJ708ALYwpK5CoMcj";
constexpr char THINGSBOARD_SERVER[] = "app.coreiot.io";
constexpr uint16_t THINGSBOARD_PORT = 1883U;
constexpr uint32_t MAX_MESSAGE_SIZE = 1024U;

constexpr uint32_t SERIAL_DEBUG_BAUD = 115200U;