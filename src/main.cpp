#include <Arduino.h>
#include <Adafruit_SGP30.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Plantower_PMS7003.h>

// SGP30 sensor setup
#define SDA 21 // SDA = GPIO21
#define SCL 22 // SCL = GPIO22

// PMS7003 sensor setup
#define PMS70003SerialRX 16 // PMS7003 RX is connected to GPIO 16
#define PMS70003SerialTX 17 // PMS7003 TX is connected to GPIO 17

// DHT sensor setup
#define DHTPIN 4      // DHT22 signal pin is connected to GPIO 4
#define DHTTYPE DHT22 // DHT22 (AM2302)

// Objects declaration

Adafruit_SGP30 sgp;                              // define a SGP30 object
HardwareSerial PMS70003Serial(2);                // define a Serial for UART1
Plantower_PMS7003 pms7003 = Plantower_PMS7003(); // define a PMS7003 object
DHT dht(DHTPIN, DHTTYPE);                        // define a DHT object

// Varibles to hold sensor readings

String PMS7003payload;
String SGP30payload; // String to hold SGP30 sensor data
String DHTpayload;   // String to hold DHT sensor data
float h, t;          // Variables to hold sensor readings

// Library Initialization

WiFiClient espClient;
PubSubClient client(espClient);

// Variable Initialization with default values

const char *ssid = "Hello";
const char *password = "abcdefghi";
String clientId = "ESP32Client"; // Your WiFi password

const char *mqtt_server = "public.mqtthq.com"; // MQTT broker IP
const int mqtt_port = 1883;                    // MQTT broker port

const char *mqtt_topic_DHT = "DATA/ESP32/DHT22";       // MQTT topic for DHT sensor
const char *mqtt_topic_SGP30 = "DATA/ESP32/SGP30";     // MQTT topic for SGP30 sensor
const char *mqtt_topic_PMS7003 = "DATA/ESP32/PMS7003"; // MQTT topic for PMS7003 sensor

const char *mqtt_topic_frequency = "CONTROL/ESP32/FREQUENCY"; // MQTT topic for frequency control

long last_time = millis(), now, frequency = 10; // Variable to hold last time measurement, current time and frequency

// Function declaration

void callback(char *topic, byte *payload, unsigned int length); // MQTT callback function
void measure();                                                 // Function to read data from sensors
void MQTTConnect();                                             // Function to connect to MQTT broker
void WiFiConnect();                                             // Function to connect to WiFi network
void SGP30Init();                                               // Function to initialize SGP30 sensor
void DHTInit();                                                 // Function to initialize DHT sensor
void PMS70003Init();
// Function to initialize PMS7003 sensor

void setup()
{
  Serial.begin(112500);
  WiFiConnect();
  PMS70003Serial.begin(9600, SERIAL_8N1, PMS70003SerialRX, PMS70003SerialTX);
  SGP30Init();
  DHTInit();
  PMS70003Init();
  MQTTConnect();
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
    Serial.println("DHT Sensor data has published : " + String(DHTpayload));
    client.publish(mqtt_topic_SGP30, SGP30payload.c_str());
    Serial.println("SGP30 Sensor data has published : " + String(SGP30payload));

    client.publish(mqtt_topic_PMS7003, PMS7003payload.c_str());
    Serial.println("PMS7003 Sensor data has published : " + String(PMS7003payload));

    client.subscribe(mqtt_topic_frequency);
  }
  else if (!client.connected())
  {
    MQTTConnect();
  }
  else if (WiFi.status() != WL_CONNECTED)
  {
    WiFiConnect();
  }
  else
  {
    Serial.println("Error Occured Cannot connect to any of ...");
  }
}

void PMS70003Init()
{
  // Initialize PMS7003 sensor
  pms7003.init(&PMS70003Serial);
}

void measurePMS7003()
{
  // Read data from PMS7003
  pms7003.updateFrame();
  if (pms7003.hasNewData())
  {
    PMS7003payload = "PM1.0: " + String(pms7003.getPM_1_0()) + " ug/m3, PM2.5: " + String(pms7003.getPM_2_5()) + " ug/m3, PM10: " + String(pms7003.getPM_10_0()) + " ug/m3";
  }
}

void debugPrintPMS7003()
{
  Serial.print("PM1.0 (ug/m3): ");
  Serial.println(pms7003.getPM_1_0());
  Serial.print("PM2.5 (ug/m3): ");
  Serial.println(pms7003.getPM_2_5());
  Serial.print("PPM10  (ug/m3): ");
  Serial.println(pms7003.getPM_10_0());
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

  measurePMS7003();
}

void callback(char *topic, byte *payload, unsigned int length)
{
  // Update the frequency after checking the topic
  if (strcmp(topic, mqtt_topic_frequency) == 0)
  {
    // set the frequency in minutes
    frequency = atoi((char *)payload) * 60000;
  }
}