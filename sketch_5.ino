#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// Wi-Fi credentials
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// KiwisIoT MQTT settings
const char* mqtt_server = "YOUR_MQTT_BROKER";
const int mqtt_port = YOUR_MQTT_PORT;
const char* mqtt_topic = "YOUR_DEVICE_TOPIC";

// Pin configuration
#define DHT_PIN 4
#define RELAY_PIN 18
#define DHT_TYPE DHT22

// Temperature limits
#define TEMP_ON 30.0
#define TEMP_OFF 28.0

// Set according to your relay module
#define RELAY_ON LOW
#define RELAY_OFF HIGH

DHT dht(DHT_PIN, DHT_TYPE);

WiFiClient espClient;
PubSubClient client(espClient);

bool fanState = false;
unsigned long lastRead = 0;

void connectWiFi() {
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting WiFi...");
  }

  Serial.println("WiFi Connected");
}

void reconnectMQTT() {
  while (!client.connected()) {
    if (client.connect("RoomTemperatureController")) {
      Serial.println("MQTT Connected");
    } else {
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF);

  dht.begin();

  connectWiFi();

  client.setServer(mqtt_server, mqtt_port);
}

void loop() {

  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!client.connected()) {
    reconnectMQTT();
  }

  client.loop();

  if (millis() - lastRead >= 2500) {

    lastRead = millis();

    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("Sensor reading failed");

      // Fail-safe: switch fan OFF
      fanState = false;
      digitalWrite(RELAY_PIN, RELAY_OFF);
      return;
    }

    // Automatic fan control
    if (temperature >= TEMP_ON) {
      fanState = true;
    }
    else if (temperature <= TEMP_OFF) {
      fanState = false;
    }

    digitalWrite(
      RELAY_PIN,
      fanState ? RELAY_ON : RELAY_OFF
    );

    Serial.print("Temperature: ");
    Serial.println(temperature);

    Serial.print("Humidity: ");
    Serial.println(humidity);

    Serial.print("Fan Status: ");
    Serial.println(fanState ? "ON" : "OFF");

    char payload[160];

    snprintf(payload, sizeof(payload),
      "{\"temperature\":%.2f,"
      "\"humidity\":%.2f,"
      "\"fan_status\":\"%s\"}",
      temperature,
      humidity,
      fanState ? "ON" : "OFF"
    );

    client.publish(mqtt_topic, payload);
  }
}