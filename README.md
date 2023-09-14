![JavaScript](https://img.shields.io/badge/javascript-%23323330.svg?style=for-the-badge&logo=javascript&logoColor=%23F7DF1E)
![Node-RED](https://img.shields.io/badge/Node--RED-%238F0000.svg?style=for-the-badge&logo=node-red&logoColor=white)

# IOT_Air_Quality
Develop an IOT device that monitors the indoor air quality and notifies the users of the air quality level
It includes a interactive dashboard implemented using Node-RED.

Currently the first prototype is created with a TVOC and CO2 sensor, Temperature and Humidity sensor and a Dust sensor.

## Components used 💾
- ESP32 (Micro Controller)
- Plantower PMS7003 (Dust Sensor)
- DHT22 (Temperature and Humidity Sensor)
- SGP30 (TVOC and CO2 Sensor)

## System Architecture 🔩
The prototype uses the ESP32 microcontroller and the above sensors to take measurements of air quality. 
These measurements are then sent to a server using the MQTT protocol under seperate topics.
These readings are displayed on a Node-Red dashboard that subcribes to these topics and the data is shown in charts

## Future improvements ✨
- Develop a PCB for the embedded device and develop the GUI interface further to make it more appealing and easthetic
- Develop a machine learning model to analyze the data acquired from the sensors and predict further trends and inferences
- Develop the system to integrate multiple nodes

