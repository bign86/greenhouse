/********************************************************************************
 * GREENHOUSE CODE
 * v. 0.2

 * v0.1: sensors
 * v0.2: heater
 *******************************************************************************/

/********************************************************************************
 * PRELIMINARY DEFINITIONS
 */

// Includes
#include "Adafruit_SHT4x.h"
#include <PubSubClient.h>
#include <WiFi.h>
#include <Wire.h>

// Constants
#define PCAADDR 0x70
#define DEV_I2C Wire
#define CONNECTION_TIMEOUT 50
#define INTERVAL_MINS 5
#define SECONDS 60
#define RELAY_PIN 5
#define MIN_TEMP 5
#define MAX_TEMP 10
#define MAX_HUMD 80

// Network credentials
const char* ssid     = "<SSID>";
const char* password = "<NETWORK_PWD>";
const char* hostname = "greenhouse-automation";

// MQTT Broker
const char* mqtt_broker   = "<MQTT_IP>";
const char* topic         = "gh";
const char* mqtt_username = "<MQTT_USER>";
const char* mqtt_password = "<MQTT_PWD>";
const int   mqtt_port     = 1883;

// Clients
WiFiClient espClient;
PubSubClient client(espClient);

// Structures for sensors
Adafruit_SHT4x sht45_0 = Adafruit_SHT4x();
Adafruit_SHT4x sht45_1 = Adafruit_SHT4x();

// Running flags
bool is_heather_on = false;
bool is_fan_on = false;


/********************************************************************************
 * MAIN SETUP
 */

void setup() {

  // Wait for serial
  while (!Serial) delay(10);

  // Start systems
  Serial.begin(19200);
  Wire.begin();
  init_wifi();
  client.setServer(mqtt_broker, mqtt_port);

  // Run through ports to test sensors
  for (uint8_t t=0; t<4; t++) {
    pcaselect(t);
    Serial.print("PCA Port #");
    Serial.println(t);

    for (uint8_t addr = 0; addr<=127; addr++) {
      if (addr == PCAADDR) continue;

      Wire.beginTransmission(addr);
      if (!Wire.endTransmission()) {
        Serial.print("Found I2C 0x");
        Serial.println(addr, HEX);
      }
    }
  }

  // Set pinout for the relay
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  

  // Start first sensor
  pcaselect(0);
  if ( ! sht45_0.begin(&DEV_I2C) ) {
    Serial.println("Could not find Sht45_0");
  } else {
    sht45_0.setPrecision(SHT4X_HIGH_PRECISION);
    //sht45_0.setHeater(SHT4X_MED_HEATER_100MS);
    sht45_0.setHeater(SHT4X_NO_HEATER);
  }

  // Start second sensor
  pcaselect(1);
  if ( ! sht45_1.begin(&DEV_I2C) ) {
    Serial.println("Could not find Sht45_1");
  } else {
    sht45_1.setPrecision(SHT4X_HIGH_PRECISION);
    //sht45_1.setHeater(SHT4X_MED_HEATER_100MS);
    sht45_1.setHeater(SHT4X_NO_HEATER);
  }

  // Final delay
  delay(1000);

}


/********************************************************************************
 * MAIN LOOP
 */

void loop() {

  // Handle MQTT
  if ( ! client.connected() ) {
    mqtt_connect();
  }
  client.loop();

  // Variables
  sensors_event_t hum0, tmp0;
  sensors_event_t hum1, tmp1;

  // Read first sensor
  pcaselect(0);
  sht45_0.getEvent(&hum0, &tmp0);

  delay(10);

  // Read second sensor
  pcaselect(1);
  sht45_1.getEvent(&hum1, &tmp1);


  /**
   AUTOMATE
  */
  // If temp is low and heater is off, put it on
  if (tmp1.temperature < MIN_TEMP) {
    Serial.println("Temp LOW: ON!");
    digitalWrite(RELAY_PIN, HIGH);
    is_heather_on = true;
  }
  // If temp is high and heater is on, put it off
  else if (tmp1.temperature > MAX_TEMP) {
    Serial.println("Temp HIGH: OFF!");
    digitalWrite(RELAY_PIN, LOW);
    is_heather_on = false;
  }

  // Publish info
  String msg = String(tmp0.temperature) + "|" + String(tmp1.temperature) + "|" + String(hum0.relative_humidity) + "|" + String(hum1.relative_humidity) + "|" + String(is_heather_on);
  client.publish(topic, msg.c_str());

  // Wait INTERVAL_MINS minutes before looping
  delay(INTERVAL_MINS*SECONDS*1000);
  
}


/********************************************************************************
 * MULTIPLEXER UTILITIES
 */

void pcaselect(uint8_t i) {
  if (i > 3) return;

  Wire.beginTransmission(PCAADDR);
  Wire.write(1 << i);
  Wire.endTransmission();
}


/********************************************************************************
 * WIFI UTILITIES
 */

void init_wifi() {
  // Configure Wifi events
  WiFi.onEvent(wifi_connected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(wifi_got_IP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(wifi_disconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

  // Configure hostname
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.setHostname(hostname);

  // Connect to Wifi
  WiFi.mode(WIFI_STA);
  wifi_connect();
}

void wifi_connect() {
  int timeout_counter = 0;

  // STart the Wifi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wifi.");

  // Keep trying to connect until expiration
  while( WiFi.status() != WL_CONNECTED ) {

    // If not connected after too much time, restart the board
    if ( timeout_counter >= CONNECTION_TIMEOUT ) {
      Serial.println("Can't establish WiFi connection");
      Serial.println("Rebooting the board...");
      ESP.restart();
    }

    Serial.print(".");
    timeout_counter++;
    delay(200);
  }
}

void wifi_connected(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.println("Wifi connected!");
}

void wifi_disconnected(WiFiEvent_t event, WiFiEventInfo_t info){
  // Shut off the heater
  digitalWrite(RELAY_PIN, LOW);

  Serial.println("\nDisconnected from WiFi access point. Reason: ");
  Serial.println(info.wifi_sta_disconnected.reason);
  Serial.println("Trying to Reconnect");
  wifi_connect();
}

void wifi_got_IP(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.print("Local Arduino ESP32 IP: ");
  Serial.println(WiFi.localIP());
  Serial.println((String)"[+] RSSI : " + WiFi.RSSI() + " dB");
}


/********************************************************************************
 * MQTT UTILITIES
 */

void mqtt_connect() {
  Serial.print("Connecting to MQTT Broker...");

  // Shut off the heater
  digitalWrite(RELAY_PIN, LOW);

  while ( ! client.connected() ) {
    // Attempt to connect
    if (client.connect(hostname, mqtt_username, mqtt_password)) {
      Serial.println(" connected");
    } else {
      Serial.print(".");
      delay(200);
    }
  }
}
