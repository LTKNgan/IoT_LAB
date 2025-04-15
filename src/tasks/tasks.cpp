#include <header.hpp>
#include "tasks/tasks.hpp"

WiFiClient espClient;
Arduino_MQTT_Client mqttClient(espClient);
OTA_Firmware_Update<> ota;
const std::array<IAPI_Implementation*, 1U> apis = {
    &ota,
};

ThingsBoard tb(mqttClient, MAX_MESSAGE_RECEIVE_SIZE, MAX_MESSAGE_SEND_SIZE, Default_Max_Stack_Size, apis);

bool currentFWSent = false;
bool updateRequestSent = false;

Espressif_Updater<> updater;

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

void reconnectWifi (void *pvParameters) {
  while (1) {
    if (WiFi.status() != WL_CONNECTED) {
      xTaskCreate(connectWifi, "Connect Wifi", 4096, NULL, 1, NULL);
    }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
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
    double temperatureC = dht20.getTemperature();
    double temperatureF = temperatureC * 1.8 + 32.0;
    double humidity = dht20.getHumidity();

    if (isnan(temperatureF) || isnan(humidity)) {
      Serial.println("Failed to read from DHT20 sensor!");
    } else {
      Serial.print("Temperature: ");
      Serial.print(temperatureF);
      Serial.print(" °F, Humidity: ");
      Serial.print(humidity);
      Serial.println(" %");

      tb.sendTelemetryData("temperature", temperatureF);
      tb.sendTelemetryData("humidity", humidity);
    }
    vTaskDelay(telemetrySendInterval);
  }
}

void update_starting_callback() {
  // Nothing to do
}

void finished_callback(const bool & success) {
  if (success) {
    Serial.println("Done, Reboot now");
    esp_restart();
    return;
  }
  Serial.println("Downloading firmware failed");
}

void progress_callback(const size_t & current, const size_t & total) {
  Serial.printf("Progress %.2f%%\n", static_cast<float>(current * 100U) / total);
}

void OTAupdate(void *pvParameters) {
  Serial.println("Check for OTA");

  // ota.Subscribe_Firmware_Update(ota_callback);
  while (!currentFWSent) {
    currentFWSent = ota.Firmware_Send_Info(CURRENT_FIRMWARE_TITLE, CURRENT_FIRMWARE_VERSION);
  }

  while (!updateRequestSent) {
    Serial.println("Firwmare Update...");
    const OTA_Update_Callback callback(CURRENT_FIRMWARE_TITLE, CURRENT_FIRMWARE_VERSION, &updater, &finished_callback, &progress_callback, &update_starting_callback, FIRMWARE_FAILURE_RETRIES, FIRMWARE_PACKET_SIZE);
    updateRequestSent = ota.Start_Firmware_Update(callback);
  }

  while (1) {
    tb.loop();  
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}