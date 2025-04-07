#include <header.hpp>
#include "tasks/tasks.hpp"

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

RPC_Response setLedState(const RPC_Data &data) {
  Serial.println("Received Switch state");
  bool newState = data;
  Serial.print("Switch state change: ");
  Serial.println(newState);
  digitalWrite(LED_PIN, newState);
  return RPC_Response("setLedValue", newState);
}

const std::array<RPC_Callback, 1U> callbacks = {
RPC_Callback{ "setLedValue", setLedState }
};

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

  Serial.println("Subscribing for RPC...");
  if (!tb.RPC_Subscribe(callbacks.cbegin(), callbacks.cend())) {
    Serial.println("Failed to subscribe for RPC");
    return;
  }
  Serial.println("Subscribe done");

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