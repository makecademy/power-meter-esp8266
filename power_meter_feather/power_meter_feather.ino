// Libraries
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_INA219.h>

// WiFi credentials
const char* ssid     = "wifi-name";
const char* password = "wifi-pass";
 
// Adafruit IO
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "username"
#define AIO_KEY         "key"
 
// Functions
void connect();
 
// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Store the MQTT server, client ID, username, and password in flash memory.
// This is required for using the Adafruit MQTT library.
const char MQTT_SERVER[] PROGMEM    = AIO_SERVER;
// Set a unique MQTT client ID using the AIO key + the date and time the sketch
// was compiled (so this should be unique across multiple devices for a user,
// alternatively you can manually set this to a GUID or other random value).
const char MQTT_CLIENTID[] PROGMEM  = AIO_KEY __DATE__ __TIME__;
const char MQTT_USERNAME[] PROGMEM  = AIO_USERNAME;
const char MQTT_PASSWORD[] PROGMEM  = AIO_KEY;
 
// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, AIO_SERVERPORT, MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD);

/****************************** Feeds ***************************************/
 
// Setup a feed called 'power' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
const char POWER_FEED[] PROGMEM = AIO_USERNAME "/feeds/power";
Adafruit_MQTT_Publish power = Adafruit_MQTT_Publish(&mqtt, POWER_FEED);

// INA sensor
Adafruit_INA219 ina219;

// OLED screen
#define OLED_RESET 3
Adafruit_SSD1306 display(OLED_RESET);
#if (SSD1306_LCDHEIGHT != 32)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

// Measurement variables
float current_mA;
float power_mW;

void setup()   {     
             
  Serial.begin(115200);

  // Init INA219
  ina219.begin();
  ina219.setCalibration_16V_400mA();

  // We start by connecting to a WiFi network
  Serial.println();
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

  // Init OLED
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  

  // Display welcome message
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,16);
  display.println("Ready!");
  display.display();
  delay(1000);

  // LED
  pinMode(14, OUTPUT);
  
  // Connect to Adafruit IO
  connect();

}


void loop() {
 
  // ping adafruit io a few times to make sure we remain connected
  if(! mqtt.ping(3)) {
    // reconnect to adafruit io
    if(! mqtt.connected())
      connect();
  }

  // LED OFF
  digitalWrite(14, LOW);

  // Measure
  current_mA = measureCurrent();
  power_mW = measurePower();

  // Publish data
  if (! power.publish(power_mW)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }

  // Display data
  displayData(current_mA, power_mW);
  delay(2000);

  // LED ON
  digitalWrite(14, HIGH);

  // Measure
  current_mA = measureCurrent();
  power_mW = measurePower();

  // Publish data
  if (! power.publish(power_mW)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }

  // Display data
  displayData(current_mA, power_mW);
  delay(2000);
  
}

// Function to measure current
float measureCurrent() {

  // Measure
  float shuntvoltage = ina219.getShuntVoltage_mV();
  float busvoltage = ina219.getBusVoltage_V();
  float current_mA = ina219.getCurrent_mA();
  float loadvoltage = busvoltage + (shuntvoltage / 1000);
  
  Serial.print("Bus Voltage:   "); Serial.print(busvoltage); Serial.println(" V");
  Serial.print("Shunt Voltage: "); Serial.print(shuntvoltage); Serial.println(" mV");
  Serial.print("Load Voltage:  "); Serial.print(loadvoltage); Serial.println(" V");
  Serial.print("Current:       "); Serial.print(current_mA); Serial.println(" mA");
  Serial.println("");

  // If negative, set to zero
  if (current_mA < 0) {
    current_mA = 0.0; 
  }
 
  return current_mA;
  
}

// Function to measure power
float measurePower() {

  // Measure
  float shuntvoltage = ina219.getShuntVoltage_mV();
  float busvoltage = ina219.getBusVoltage_V();
  float current_mA = ina219.getCurrent_mA();
  float loadvoltage = busvoltage + (shuntvoltage / 1000);
  
  Serial.print("Bus Voltage:   "); Serial.print(busvoltage); Serial.println(" V");
  Serial.print("Shunt Voltage: "); Serial.print(shuntvoltage); Serial.println(" mV");
  Serial.print("Load Voltage:  "); Serial.print(loadvoltage); Serial.println(" V");
  Serial.print("Current:       "); Serial.print(current_mA); Serial.println(" mA");
  Serial.println("");

  // If negative, set to zero
  if (current_mA < 0) {
    current_mA = 0.0; 
  }
 
  return current_mA * loadvoltage;
  
}

// Display measurement data
void displayData(float current, float power) {

  // Clear
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);

  // Power
  display.setCursor(0,0);
  display.println("Power: ");
  display.print(power);
  display.println(" mW");

  // Displays
  display.display();
  
}

// connect to adafruit io via MQTT
void connect() {
 
  Serial.print(F("Connecting to Adafruit IO... "));
 
  int8_t ret;
 
  while ((ret = mqtt.connect()) != 0) {
 
    switch (ret) {
      case 1: Serial.println(F("Wrong protocol")); break;
      case 2: Serial.println(F("ID rejected")); break;
      case 3: Serial.println(F("Server unavail")); break;
      case 4: Serial.println(F("Bad user/pass")); break;
      case 5: Serial.println(F("Not authed")); break;
      case 6: Serial.println(F("Failed to subscribe")); break;
      default: Serial.println(F("Connection failed")); break;
    }
 
    if(ret >= 0)
      mqtt.disconnect();
 
    Serial.println(F("Retrying connection..."));
    delay(5000);
 
  }
 
  Serial.println(F("Adafruit IO Connected!"));
 
}
