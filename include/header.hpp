// #define LED_PIN 13
#define SDA_PIN GPIO_NUM_21
#define SCL_PIN GPIO_NUM_22

#include <WiFi.h>
#include <Arduino_MQTT_Client.h>
#include <ThingsBoard.h>
#include "DHT20.h"
#include "Wire.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Wifi parameter
constexpr char WIFI_SSID[] = "KN";
constexpr char WIFI_PASSWORD[] = "nganxinhdep";

// Device parameter
constexpr char TOKEN[] = "ri10necgf3go0q2mh9fp";

constexpr char THINGSBOARD_SERVER[] = "app.coreiot.io";
constexpr uint16_t THINGSBOARD_PORT = 1883U;

constexpr uint32_t MAX_MESSAGE_SIZE = 1024U;
constexpr uint32_t SERIAL_DEBUG_BAUD = 115200U;

constexpr int16_t telemetrySendInterval = 10000U;