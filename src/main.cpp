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
String SGP30payload; // String to hold SGP30 sensor data

// DHT sensor setup
#define DHTPIN 4          // DHT22 signal pin is connected to GPIO 4
#define DHTTYPE DHT22     // DHT22 (AM2302)
DHT dht(DHTPIN, DHTTYPE); // Initialize DHT sensor
float h, t;               // Variables to hold sensor readings
String DHTpayload;        // String to hold DHT sensor data

// Replace with your network credentials
const char *ssid = "AndroidAPBD06";
const char *password = "1234567890";

// MQTT setup
const char *mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;

// Publish topics
const char *mqtt_topic_DHT = "DATA/ESP32/DHT22";
const char *mqtt_topic_SGP30 = "DATA/ESP32/SGP30";

// Subscribe topics
const char *mqtt_topic_frequency = "CONTROL/ESP32/FREQUENCY";

WiFiClient espClient;
PubSubClient client(espClient);

long last_time = millis(), now, frequency = 10;

void callback(char *topic, byte *payload, unsigned int length);
void measure();
void MQTTConnect();
void WiFiConnect();
void SGP30Init();
void DHTInit();

void setup()
{
  Serial.begin(9600);

  // Initialize sensors
  SGP30Init();
  DHTInit();

  // Wait for few seconds to initialize the sensors
  delay(5000);
}

void loop()
{
  // Read the measurements from the sensors
  measure();

  // MQTT client loop
  if (client.connected())
  {
    client.loop();
    // Publish data to MQTT broker
    client.publish(mqtt_topic_DHT, DHTpayload.c_str());
    client.publish(mqtt_topic_SGP30, SGP30payload.c_str());
    client.subscribe(mqtt_topic_frequency);
  }
  else
  {
    // Connect to WiFi network
    WiFiConnect();

    // Connect to MQTT broker
    MQTTConnect();
  }
}

void DHTInit()
{
  // Initialize DHT sensor
  dht.begin();
}
void measureDHT()
{
  // Read temperature and humidity data
  h = dht.readHumidity();
  t = dht.readTemperature();

  // Check if any reading failed
  if (isnan(h) || isnan(t))
  {
    Serial.println("Failed to read data from DHT22 sensor");
    return;
  }
  DHTpayload = "temperature: " + String(t) + ", humidity: " + String(h);
}
void debugPrintDHT()
{
  // Print temperature and humidity values to Serial Monitor
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t Temperature: ");
  Serial.print(t);
  Serial.println(" *C");
}

void SGP30Init()
{
  // Initialize SGP30 sensor
  Wire.begin(SDA, SCL);

  while (!sgp.begin())
  {
    Serial.println("Sensor not found :(");
  }

  Serial.println("SGP30 sensor found");
}
void measureSGP30()
{
  // Read data from SGP30
  if (!sgp.IAQmeasure())
  {
    Serial.println("Measurement failed");
    return;
  }
  SGP30payload = "eCO2: " + String(sgp.eCO2) + " ppm, TVOC: " + String(sgp.TVOC) + " ppb";
}
void debugPrintSGP30()
{
  // Print SGP30 values to Serial Monitor
  Serial.print("eCO2: ");
  Serial.print(sgp.eCO2);
  Serial.print(" ppm\tTVOC: ");
  Serial.print(sgp.TVOC);
  Serial.println(" ppb");
}

void WiFiConnect()
{
  // Check if the WiFi is already connected
  if (WiFi.status() == WL_CONNECTED)
    return;

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
}

void MQTTConnect()
{
  // Set the server and port
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  // Loop until we're reconnected
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
  // Get current time
  now = millis();
  if (now - last_time <= frequency)
    return;

  // Update the last time measurement
  last_time = now;

  // Read data from DHT22
  measureDHT();

  // Read data from SGP30
  measureSGP30();
}

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Callback - ");
  Serial.print("Message:");

  // Update the frequency after checking the topic
  if (strcmp(topic, mqtt_topic_frequency) == 0)
  {
    // set the frequency in minutes
    frequency = atoi((char *)payload) * 60000;
    Serial.print("Frequency: ");
    Serial.println(frequency);
  }
}