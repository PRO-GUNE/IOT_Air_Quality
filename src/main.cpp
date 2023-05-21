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
const char *ssid = "AndroidAPBD06";
const char *password = "1234567890";

// MQTT setup
const char *mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;

const char *mqtt_topic_DHT = "DATA/ESP32/DHT22";
const char *mqtt_topic_SGP30 = "DATA/ESP32/SGP30";

WiFiClient espClient;
PubSubClient client(espClient);

long last_time = millis(), now, frequency = 10;

void callback(char *topic, byte *payload, unsigned int length);
void measure();
void reconnect();

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
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  // Connect to MQTT broker
  while (!client.connected())
  {
    if (client.connect("ESP32Client"))
    {
      Serial.println("Connected to MQTT broker");
    }
    else
    {
      Serial.print("MQTT connection failed, error code: ");
      Serial.println(client.state());
      delay(2000);
    }
  }

  // Initialize SGP30 sensor
  Wire.begin(SDA, SCL);

  while (!sgp.begin())
  {
    Serial.println("Sensor not found :(");
  }

  Serial.println("SGP30 sensor found");

  // Initialize DHT sensor
  dht.begin();
}

void loop()
{
  // MQTT client loop
  if (client.connected())
  {
    client.loop();
    Serial.println("MQTT client loop");

    // Wait a few seconds between measurements
    now = millis();

    if (now - last_time > frequency)
    {
      last_time = now;
      measure();
    }
  }
  else
  {
    reconnect();
  }
}

void reconnect()
{
  // reconnect the client
  while (!client.connected())
  {
    Serial.println("Connecting to MQTT broker...");
    if (client.connect("ESP32Client"))
    {
      Serial.println("Connected to MQTT broker");
    }
    else
    {
      Serial.print("MQTT connection failed, error code: ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void measure()
{
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
  String payload = "temperature: " + String(t) + ", humidity: " + String(h);
  client.publish(mqtt_topic_DHT, payload.c_str());

  // Publish eCO2 and TVOC values to MQTT broker
  payload = "eCO2: " + String(sgp.eCO2) + ", TVOC: " + String(sgp.TVOC);
  client.publish(mqtt_topic_SGP30, payload.c_str());
}

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Callback - ");
  Serial.print("Message:");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
}