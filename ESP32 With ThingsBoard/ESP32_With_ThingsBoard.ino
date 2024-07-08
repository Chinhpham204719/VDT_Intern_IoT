#include <WiFi.h>
#include <PubSubClient.h>

// WiFi and MQTT Server Settings
const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "demo.thingsboard.io";
const char* access_token = "tdR4PSlA01JXER1GGU0X";

// GPIO Pins
#define NTC_PIN 5       // Analog pin for NTC
#define SWITCH_PIN 7    // Digital pin for switch

// MQTT Client
WiFiClient espClient;
PubSubClient client(espClient);

// Variables
bool deviceState = false;
unsigned long lastMsg = 0;
unsigned long lastAttributeCheck = 0;
unsigned long attributeInterval = 60000; // 60 seconds
int thresholdNTC = 42, count = 0;
int ntcValue;
float ntcVoltage;
float temperatureNTC;

void setup() {
  Serial.begin(115200);
  
  // Initialize WiFi
  setup_wifi();

  // Initialize MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(mqtt_callback);

  // Initialize GPIO
  pinMode(NTC_PIN, INPUT);
  pinMode(SWITCH_PIN, INPUT);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Read sensors every 500ms
  if (millis() - lastMsg > 500){
    ReadSensors();
    count++;
    if (count == 10) {
      PublishSensors(); // Publish sensors every 5s
      count = 0; // Reset count
    }
    lastMsg = millis(); // Update lastMsg after reading sensors
  }

  // Read attribute every 60 seconds
  if (millis() - lastAttributeCheck > attributeInterval) {
    lastAttributeCheck = millis();
    client.subscribe("v1/devices/me/attributes/response/+");
    client.publish("v1/devices/me/attributes/request/1", "{\"sharedKeys\":\"Threshold\"}");
  }

  // Check switch state
  deviceState = digitalRead(SWITCH_PIN) == LOW;
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client", access_token, NULL)) {
      Serial.println("connected");
      client.subscribe("v1/devices/me/attributes");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(message);

  if (String(topic).endsWith("attributes/response/1")) {
    int newValue = message.toInt();
    if (newValue > 0 && newValue != thresholdNTC) {
      if (String(topic).indexOf("Threshold") != -1) {
        thresholdNTC = newValue;
      }
    }
  }
}

void ReadSensors() {
  // Read NTC
  ntcValue = analogRead(NTC_PIN);
  ntcVoltage = ntcValue * (3.3 / 4095.0);
  temperatureNTC = (ntcVoltage - 0.5) * 100.0;
}

void PublishSensors() {
  ReadSensors();
  // Create JSON payload
  String payload = "{";
  payload += "\"NTC\":" + String(temperatureNTC) + ",";
  payload += "\"state\":\"" + String(deviceState ? "On" : "Off") + "\"";
  payload += "}";

  // Publish payload
  client.publish("v1/devices/me/telemetry", payload.c_str());
  Serial.println(payload);
}
