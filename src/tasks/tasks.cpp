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
ThingsBoard tb(mqttClient, MAX_MESSAGE_RECEIVE_SIZE, MAX_MESSAGE_SEND_SIZE, Default_Max_Stack_Size, apis);

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




Espressif_Updater<> updater;
// Statuses for updating
bool shared_update_subscribed = false;
bool currentFWSent = false;
bool updateRequestSent = false;
bool requestedShared = false;


OTA_Firmware_Update<> ota;
Attribute_Request<2U, MAX_ATTRIBUTES> attr_request;
const std::array<IAPI_Implementation*, 3U> apis = {
    &shared_update,
    &attr_request,
    &ota
};


// struct binary_data_t {
//     size_t size;
//     size_t remaining_size;
//     void * data;
// };

// /////////////////////////////////////////////////////////////////////////////////////////////////////////////

void processSharedAttributeRequest(const JsonObjectConst &data) {
  //Info
  const size_t jsonSize = Helper::Measure_Json(data);
  char buffer[jsonSize];
  serializeJson(data, buffer, jsonSize);
  Serial.println(buffer);
}

void processSharedAttributeUpdate(const JsonObjectConst &data) {
  //Info
  const size_t jsonSize = Helper::Measure_Json(data);
  char buffer[jsonSize];
  serializeJson(data, buffer, jsonSize);
  Serial.println(buffer);
}

// void otaSDToFlashTask(void* pvParameter) {
//     FILE * ota_bin_file = fopen(UPDAT_FILE_PATH, "rb");
//     esp_ota_handle_t update_handle;
//     esp_partition_t const * update_partition = esp_ota_get_next_update_partition(NULL);
//     binary_data_t data;

//     if (ota_bin_file == nullptr) {
//         ESP_LOGE("MAIN", "Failed to open file for Update");
//         vTaskDelete(NULL);
//     } else {
//         esp_err_t error = ESP_OK;
//         ESP_LOGI("MAIN", "Opened File for Update");
//         fseek(ota_bin_file, 0, SEEK_END);
//         data.size = ftell(ota_bin_file);
//         data.remaining_size = data.size;
//         ESP_LOGI("MAIN", "Update Size: %u", data.size);
//         data.data = malloc(FIRMWARE_PACKET_SIZE);
//         fseek(ota_bin_file, 0, SEEK_SET);
//         esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
//         while (data.remaining_size > 0) {
//             size_t const size = data.remaining_size <= FIRMWARE_PACKET_SIZE ? data.remaining_size : FIRMWARE_PACKET_SIZE;
//             fread(data.data, size, 1, ota_bin_file);
//             error = esp_ota_write(update_handle, data.data, size);
//             if (data.remaining_size <= FIRMWARE_PACKET_SIZE) {
//                 break;
//             }
//             data.remaining_size -= FIRMWARE_PACKET_SIZE;
//             vTaskDelay(0);
//         }
//         if (error != ESP_OK) {
//             ESP_LOGE("MAIN", "Failed to write OTA data: 0x%X (%s)", error, esp_err_to_name(error));
//         }
//         error = esp_ota_end(update_handle);
//         if (error != ESP_OK) {
//             ESP_LOGE("MAIN", "Failed to end OTA update: 0x%X (%s)", error, esp_err_to_name(error));
//         }

//         error = esp_ota_set_boot_partition(update_partition);
//         if (error != ESP_OK) {
//             ESP_LOGE("MAIN", "Failed to set boot partition: 0x%X (%s)", error, esp_err_to_name(error));
//         } else {
//             ESP_LOGI("MAIN", "Updated with data from SD card, Restarting");
//             esp_restart();
//         }
//         vTaskDelete(NULL);
//         return;
//     }
// }

void update_starting_callback() {
  // Nothing to do
}

void finished_callback(const bool & success) {
  if (success) {
    Serial.println("Downloading firmware successfull!");
    xTaskCreate(otaSDToFlashTask, "OTA_SD_TO_FLASH", FIRMWARE_PACKET_SIZE + 1024 * 1, NULL, 16, NULL);
    // esp_restart();
    return;
  }
  Serial.println("Downloading firmware failed");
}

void progress_callback(const size_t & current, const size_t & total) {
  Serial.println("Downwloading firmware progress %.2f%%", static_cast<float>(current * 100U) / total);
}

void OTAupdate (void* pvParameter) {
  // if (!currentFWSent) {
  //   currentFWSent = ota.Firmware_Send_Info(CURRENT_FIRMWARE_TITLE, CURRENT_FIRMWARE_VERSION);
  // }
  // if (!updateRequestSent) {
  //   const OTA_Update_Callback callback(CURRENT_FIRMWARE_TITLE, CURRENT_FIRMWARE_VERSION, &updater, &finished_callback, &progress_callback, &update_starting_callback, FIRMWARE_FAILURE_RETRIES, FIRMWARE_PACKET_SIZE);
  //   updateRequestSent = ota.Start_Firmware_Update(callback);
  // }



if (!requestedShared) {
  Serial.println("Requesting shared attributes...");
  const Attribute_Request_Callback<MAX_ATTRIBUTES> sharedCallback(&processSharedAttributeRequest, REQUEST_TIMEOUT_MICROSECONDS, &requestTimedOut, SHARED_ATTRIBUTES);
  requestedShared = attr_request.Shared_Attributes_Request(sharedCallback);
  if (!requestedShared) {
    Serial.println("Failed to request shared attributes");
  }
}

if (!shared_update_subscribed){
  Serial.println("Subscribing for shared attribute updates...");
  const Shared_Attribute_Callback<MAX_ATTRIBUTES> callback(&processSharedAttributeUpdate, SHARED_ATTRIBUTES);
  if (!shared_update.Shared_Attributes_Subscribe(callback)) {
  Serial.println("Failed to subscribe for shared attribute updates");
  // continue;
  }
  Serial.println("Subscribe done");
  shared_update_subscribed = true;
}

if (!currentFWSent) {
currentFWSent = ota.Firmware_Send_Info(CURRENT_FIRMWARE_TITLE, CURRENT_FIRMWARE_VERSION);
}

if (!updateRequestSent) {
  Serial.print(CURRENT_FIRMWARE_TITLE);
  Serial.println(CURRENT_FIRMWARE_VERSION);
  Serial.println("Firwmare Update ...");
  const OTA_Update_Callback callback(CURRENT_FIRMWARE_TITLE, CURRENT_FIRMWARE_VERSION, &updater, &finished_callback, &progress_callback, &update_starting_callback, FIRMWARE_FAILURE_RETRIES, FIRMWARE_PACKET_SIZE);
  updateRequestSent = ota.Start_Firmware_Update(callback);
  if(updateRequestSent) {
    delay(500);
    Serial.println("Firwmare Update Subscription...");
    updateRequestSent = ota.Subscribe_Firmware_Update(callback);
  }
}
  vTaskDelay(1000 / portTICK_PERIOD_MS);

}