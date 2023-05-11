#include <Arduino.h>
#include <Adafruit_SGP30.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>

// SGP30 sensor setup
#define SDA 21 // SDA = GPIO21
#define SCL 22 // SCL = GPIO22
Adafruit_SGP30 sgp;

// DHT sensor setup
#define DHTPIN 4          // DHT22 signal pin is connected to GPIO 4
#define DHTTYPE DHT22     // DHT22 (AM2302)
DHT dht(DHTPIN, DHTTYPE); // Initialize DHT sensor

// Replace with your network credentials
const char *ssid = "your_SSID";
const char *password = "your_PASSWORD";

// MQTT setup
const char *mqtt_server = "your_MQTT_server_IP";
const char *mqtt_username = "your_MQTT_username";
const char *mqtt_password = "your_MQTT_password";
const char *mqtt_topic_DHT = "DATA/ESP32/DHT22";
const char *mqtt_topic_SGP30 = "DATA/ESP32/SGP30";

WiFiClient espClient;
PubSubClient client(espClient);

void setup()
{
  Serial.begin(9600);

  // Connect to WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Connect to MQTT broker
  client.setServer(mqtt_server, 1883);
  while (!client.connected())
  {
    Serial.println("Connecting to MQTT broker...");
    if (client.connect("ESP32Client", mqtt_username, mqtt_password))
    {
      Serial.println("MQTT broker connected");
    }
    else
    {
      Serial.print("MQTT connection failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying...");
      delay(5000);
    }
  }

  // Initialize SGP30 sensor
  Wire.begin(SDA, SCL);

  if (!sgp.begin())
  {
    Serial.println("Sensor not found :(");
    while (1)
      ;
  }

  Serial.println("SGP30 sensor found");

  // Initialize DHT sensor
  dht.begin();
}

void loop()
{
  // Wait a few seconds between measurements
  delay(2000);

  // Read temperature and humidity data
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // Check if any reading failed
  if (isnan(h) || isnan(t))
  {
    Serial.println("Failed to read data from DHT22 sensor");
    return;
  }

  // Print temperature and humidity values
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.print("°C  Humidity: ");
  Serial.print(h);
  Serial.println("%");

  // Read data from SGP30
  if (!sgp.IAQmeasure())
  {
    Serial.println("Measurement failed");
    return;
  }

  Serial.print("eCO2: ");
  Serial.print(sgp.eCO2);
  Serial.print(" ppm\tTVOC: ");
  Serial.print(sgp.TVOC);
  Serial.println(" ppb");

  // Publish temperature and humidity values to MQTT broker
  String payload = "{\"temperature\": " + String(t) + ", \"humidity\": " + String(h) + "}";
  client.publish(mqtt_topic_DHT, payload.c_str());

  // Publish eCO2 and TVOC values to MQTT broker
  payload = "{\"eCO2\": " + String(sgp.eCO2) + ", \"TVOC\": " + String(sgp.TVOC) + "}";
  client.publish(mqtt_topic_SGP30, payload.c_str());
}
