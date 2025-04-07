#include <header.hpp>
#include "tasks/tasks.hpp"

constexpr char BLINKING_INTERVAL_ATTR[] = "blinkingInterval";
constexpr char LED_MODE_ATTR[] = "ledMode";
constexpr char LED_STATE_ATTR[] = "ledState";

volatile bool attributesChanged = false;
volatile int ledMode = 0;
volatile bool ledState = false;

volatile uint16_t blinkingInterval = 1000U;
constexpr uint16_t BLINKING_INTERVAL_MS_MIN = 10U;
constexpr uint16_t BLINKING_INTERVAL_MS_MAX = 60000U;

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

RPC_Response setLedState(const RPC_Data &data) {
  Serial.println("Received Switch state");
  bool newState = data;
  Serial.print("Switch state change: ");
  Serial.println(newState);
  digitalWrite(LED_PIN, newState);
  attributesChanged = true;
  return RPC_Response("setLedValue", newState);
}

const std::array<RPC_Callback, 1U> callbacks = {
RPC_Callback{ "setLedValue", setLedState }
};

constexpr std::array<const char *, 2U> SHARED_ATTRIBUTES_LIST = {
  LED_STATE_ATTR,
  BLINKING_INTERVAL_ATTR
};

void processSharedAttributes(const Shared_Attribute_Data &data) {
  for (auto it = data.begin(); it != data.end(); ++it) {
    if (strcmp(it->key().c_str(), BLINKING_INTERVAL_ATTR) == 0) {
      const uint16_t new_interval = it->value().as<uint16_t>();
      if (new_interval >= BLINKING_INTERVAL_MS_MIN && new_interval <= BLINKING_INTERVAL_MS_MAX) {
        blinkingInterval = new_interval;
        Serial.print("Blinking interval is set to: ");
        Serial.println(new_interval);
      }
    } else if (strcmp(it->key().c_str(), LED_STATE_ATTR) == 0) {
      ledState = it->value().as<bool>();
      digitalWrite(LED_PIN, ledState);
      Serial.print("LED state is set to: ");
      Serial.println(ledState);
    }
  }
  attributesChanged = true;
}

const Shared_Attribute_Callback attributes_callback(&processSharedAttributes, SHARED_ATTRIBUTES_LIST.cbegin(), SHARED_ATTRIBUTES_LIST.cend());
const Attribute_Request_Callback attribute_shared_request_callback(&processSharedAttributes, SHARED_ATTRIBUTES_LIST.cbegin(), SHARED_ATTRIBUTES_LIST.cend());

void subscribeRPC (void *pvParameters) {
  Serial.println("Subscribing for RPC...");
  while (!tb.RPC_Subscribe(callbacks.cbegin(), callbacks.cend())) {
    Serial.print(".");
    vTaskDelay(1000 / portTICK_PERIOD_MS);  
  }

  while (!tb.Shared_Attributes_Subscribe(attributes_callback)) {
    Serial.print(".");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  Serial.println("Subscribe done");

  while (!tb.Shared_Attributes_Request(attribute_shared_request_callback)) {
    Serial.print(".");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  if (attributesChanged) {
    attributesChanged = false;
    tb.sendAttributeData(LED_STATE_ATTR, digitalRead(LED_PIN));
  }
  vTaskDelete(NULL); 
}