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

WiFiClient wifiClient;
Arduino_MQTT_Client mqttClient(wifiClient);
ThingsBoard tb(mqttClient, MAX_MESSAGE_SIZE);

DHT20 dht20;

void connectWifi (void *pvParameters) {
  Serial.println("Connecting to Wifi...");
  // Attempting to establish a connection to the given WiFi network
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.print(".");
  }
  Serial.print("\nWifi connected!\nIP:");
  Serial.println(WiFi.localIP());

  vTaskDelete(NULL); 
}

void connectThingsBoard (void *pvParameters){
  // connect to thingsboard
  Serial.print("\nConnecting to ");
  Serial.print(THINGSBOARD_SERVER);
  Serial.print(" with token ");
  Serial.println(TOKEN);
  while (!tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT)) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.print(".");
  }
  Serial.println("Connect to thingsboard successfully!");
  tb.sendAttributeData("macAddress", WiFi.macAddress().c_str());
  vTaskDelete(NULL); 
}

void sendTelemetryData (void *pvParameters){
  DHT20 dht20;
  Wire.begin(SDA_PIN, SCL_PIN);
  dht20.begin();
  while(1){
    dht20.read();
    double temperature = dht20.getTemperature();
    double humidity = dht20.getHumidity();

    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("Failed to read from DHT20 sensor!");
    } else {
      Serial.print("Temperature: ");
      Serial.print(temperature);
      Serial.print(" °C, Humidity: ");
      Serial.print(humidity);
      Serial.println(" %");

      tb.sendTelemetryData("temperature", temperature);
      tb.sendTelemetryData("humidity", humidity);
    }
    vTaskDelay(telemetrySendInterval);
  }
}

void setup() {
  Serial.begin(SERIAL_DEBUG_BAUD);

  xTaskCreate(connectWifi, "Connect Wifi", 4096, NULL, 1, NULL);
  xTaskCreate(connectThingsBoard, "Connect ThingsBoard", 4096, NULL, 2, NULL);
  xTaskCreate(sendTelemetryData, "Send Telementry", 2048, NULL, 3, NULL);
}

void loop() {
}
