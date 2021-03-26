#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <LoRa.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>

#include "Settings.h"
#include "WifiSettings.h"
#include "Bitmaps.h"
#include "Fonts.h"
#include "version.h"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
WiFiClient espClient;
PubSubClient mqtt(espClient);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
SoftwareSerial HC12(RX_PIN, TX_PIN); // HC-12 TX Pin, HC-12 RX Pin

// Flags

bool hc12Flag = false;
bool receiveFlag = false;
bool NewDataFlag = false;
bool rssiLoraFlag = false;

int id = -1;
int rssi = -100;
int rssiLora = 0;
int counter = 0;
unsigned long lastUpdate = 0;
String dataTime;
String version = String(VERSION_SHORT);

// FUNCTIONS //

void DisplayWelcome()
{
  display.clearDisplay();
  display.drawBitmap(63, 0, Lizard, 65, 55, SSD1306_WHITE);
  display.setFont(&Dialog_plain_12);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 15);
  display.print("LORA @ HC12");
  display.setCursor(0, 30);
  display.print("GATEWAY");
  display.setCursor(0, 61);
  display.print("J.Jaszczur");
  display.setCursor(80, 61);
  display.print(version.substring(0, 6));
  display.display();

  delay(2000);
}

void DisplayBackground()
{
  display.clearDisplay();
  display.setFont(&Dialog_plain_12);
  display.setCursor(25, 30);
  display.print("MQTT");
  display.setCursor(92, 15);
  display.print("LORA");
  display.setCursor(92, 45);
  display.print("HC12");
  display.setFont(&Dialog_plain_10);
  display.setCursor(0, BOTTOM_TEXT_Y);
  display.print("L:");
  display.setCursor(70, BOTTOM_TEXT_Y);
  display.print("C:");
  display.setFont(&Dialog_plain_8);
  display.setCursor(0, 43);
  display.print("dBm");

  // Upper line
  display.drawLine(35, 13, 35, 10, SSD1306_WHITE);
  display.drawLine(35, 10, 40, 10, SSD1306_WHITE);
  // Lower line
  display.drawLine(35, 37, 35, 40, SSD1306_WHITE);
  display.drawLine(35, 40, 40, 40, SSD1306_WHITE);

  display.display();
}

void DisplayData()
{
  display.setFont(&Dialog_plain_12);
  DisplayBackground();

  // HC12 data
  if(hc12Flag)
  {
    display.setCursor(CURSOR_X, LOWER_ID_TEXT_Y);

    if(receiveFlag)
    {
      display.print("<       <");
    }
    else
    {
      display.print(">       >");
    }

    display.setCursor(ID_TEXT_X, LOWER_ID_TEXT_Y);
  }
    // HC12 data
  else
  {
    display.setCursor(CURSOR_X, UPPER_ID_TEXT_Y);

    if(receiveFlag)
    {
      display.print("<       <");
    }
    else
    {
      display.print(">       >");
    }

    display.setCursor(ID_TEXT_X, UPPER_ID_TEXT_Y);
  }

  display.setFont(&Dialog_plain_10);
  display.print(String(id));
  display.setCursor(TIME_X, BOTTOM_TEXT_Y);
  display.print(dataTime);
  display.setCursor(COUNTER_X, BOTTOM_TEXT_Y);
  display.print(counter);

  display.setFont(&Dialog_plain_8);
  display.setCursor(0, 12);
  display.print(rssi);

  if(rssi > -30)
  {
      display.drawBitmap(0, WIFI_ICON_Y, wifi4, 20, 20, SSD1306_WHITE);   //test
  }
  else if(rssi > -67)
  {
      display.drawBitmap(0, WIFI_ICON_Y, wifi3, 20, 20, SSD1306_WHITE);   //test
  }
  else if(rssi > -70)
  {
      display.drawBitmap(0, WIFI_ICON_Y, wifi2, 20, 20, SSD1306_WHITE);   //test
  }
  else if(rssi > -80)
  {
      display.drawBitmap(0, WIFI_ICON_Y, wifi1, 20, 20, SSD1306_WHITE);   //test
  }
  else
  {
      display.drawBitmap(0, WIFI_ICON_Y, wifi0, 20, 20, SSD1306_WHITE);   //test
  }

  display.display();
}

void ConnectToWifi()
{
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  display.clearDisplay();
  display.setCursor(0, 20);
  display.print("Connecting to:");
  display.setCursor(0, 40);
  display.print((String)ssid);
  display.display();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.hostname(DEVICE_NAME);

  int cursor = 0;

  while(WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
    display.setCursor(cursor, 60);
    display.print(".");
    display.display();
    cursor = cursor + 2;
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("");

  display.clearDisplay();
  display.setCursor(0, 20);
  display.print("WiFi connected");
  display.setCursor(0, 40);
  display.print(WiFi.localIP().toString().c_str());
  display.display();
}

void GatewaySend(String data, bool hc12)
{
  if(hc12)
  {
    HC12.println(data);
    Serial.println("MQTT -> HC12 sent!");
  }
  else
  {
    LoRa.beginPacket();
    LoRa.print(data);
    LoRa.endPacket(true);
    Serial.println("MQTT -> Lora sent!");
  }   
}

int CheckSenderId(String message)
{
  const size_t capacity = JSON_OBJECT_SIZE(12) + 80;
  DynamicJsonDocument data(capacity);
  int senderid = -1;

  deserializeJson(data, message);

  if(data.containsKey("id"))
  {
    senderid = data["id"];
  }

  return senderid;
}

void HandleService(String serviceMessage)
{
  String output;

  if(serviceMessage == "STATUS")
  {

  const size_t capacity = JSON_OBJECT_SIZE(5) + 30;
  DynamicJsonDocument data(capacity);
  
  data["last"] = id;
  data["time"] = dataTime;
  data["rcv"] = receiveFlag;
  data["cnt"] = counter;
  data["rssi"] = rssi;
  
  serializeJson(data, output);

  }
  
  char msg[250];

  output.toCharArray(msg, sizeof(msg));
  mqtt.publish(DEBUG_TOPIC, msg);
}

void callback(char* topic, byte* payload, unsigned int length) 
{
  String data = "";

  Serial.print("Message arrived from MQTT [");
  Serial.print(topic);
  Serial.print("] ");

  for (uint i = 0; i < length; i++)
  {
    data += (char)payload[i];
  }
  
  Serial.println(data);
  dataTime = timeClient.getFormattedTime();

  if(String(topic) == HC12_SEND_TOPIC)
  {
    hc12Flag = true;
    receiveFlag = false;
    NewDataFlag = true;
  }

  if(String(topic) == LORA_SEND_TOPIC)
  {
    hc12Flag = false;
    receiveFlag = false;
    NewDataFlag = true;
  }
    if(String(topic) == SERVICE_TOPIC)
  {
    HandleService(data);
  }

  if(NewDataFlag)
  {
    counter++;
    timeClient.update();
    id = CheckSenderId(data);
    GatewaySend(data, hc12Flag);
    DisplayData();
    NewDataFlag = false;
  }
}

void ConnectToMQTT() 
{
  display.clearDisplay();
  display.setCursor(0, 20);
  display.print("Connecting to:");
  display.setCursor(0, 40);
  display.print(mqtt_server);
  display.display();

  int cursor = 0;
  // Loop until we're reconnected
  while (!mqtt.connected()) 
  {
    mqtt.setServer(mqtt_server, 1883);
    mqtt.setCallback(callback);

    Serial.print("Attempting MQTT connection...");
    
    display.setCursor(cursor, 60);
    display.print(".");
    display.display();
    cursor = cursor + 2;
    delay(1000);

    // Attempt to connect
    if(mqtt.connect(DEVICE_NAME, mqtt_user, mqtt_password)) 
    {
      Serial.println("Connected");

      mqtt.subscribe(LORA_SEND_TOPIC);
      mqtt.subscribe(HC12_SEND_TOPIC);
      mqtt.subscribe(SERVICE_TOPIC);

      display.clearDisplay();
      display.setCursor(0, 20);
      display.print("MQTT connected");
      display.display();
    } 
    else 
    {
      Serial.print("Failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" Try again in 1 second");
      // Wait 5 seconds before retrying
    }
  }
}

bool CheckMqttConnection()
{
  if(!mqtt.connected()) 
  {
    ConnectToMQTT();
    DisplayBackground();
  }

  return true;
}

void MqttSend(String data, bool hc12)
{ 
  CheckMqttConnection();

  char message[200];
  data.toCharArray(message, sizeof(message));

  if(hc12)
  {
    mqtt.publish(HC12_TOPIC, message);
    Serial.println("HC12 -> MQTT sent!");
  }
  else
  {
    mqtt.publish(LORA_TOPIC, message);
    Serial.println("Lora -> MQTT sent!");
  }   
}

void LoraConfigure()
{
  LoRa.setPins(DEFAULT_PIN_SS, DEFAULT_PIN_RST, DEFAULT_PIN_DIO0);

  if (!LoRa.begin(freq)) 
  {
    Serial.println("Starting LoRa failed!");
  }
  LoRa.setSpreadingFactor(SFACTOR);           // ranges from 6-12,default 7 see API docs
}

String LoraReadData()
{
  String data;
  // read packet
  while (LoRa.available()) {
      data += (char)LoRa.read();
  }

  rssiLora = LoRa.packetRssi();
  rssiLoraFlag = true;
  Serial.println("Received data from Lora:");
  Serial.println(data);
  return data;
}

String Hc12ReadData()
{
  String data = HC12.readString();
  Serial.println("Received data from HC-12:");
  Serial.println(data);
  return data;
}

String AddRssiToData(String message)
{
  const size_t capacity = JSON_OBJECT_SIZE(12) + 80;
  DynamicJsonDocument data(capacity);
  deserializeJson(data, message);

  data["rssi"] = rssiLora;
  
  String output;
  serializeJson(data, output);

  return output;
}

// MAIN CODE //

void setup() 
{
  Serial.begin(115200);
  HC12.begin(9600);                 // Serial port to HC12
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  DisplayWelcome();
  ConnectToWifi();
  ConnectToMQTT();
  DisplayBackground();
  timeClient.begin();
  timeClient.setTimeOffset(3600);   // GMT+1
  LoraConfigure();
}

void loop() 
{
  String incoming = "";
  unsigned long actualTime = millis();

  // Check Lora
  if(LoRa.parsePacket()) 
  {
    incoming = LoraReadData();
    hc12Flag = false;
    receiveFlag = true;
    NewDataFlag = true;
  }

  mqtt.loop();

  // Check HC-12
  if(HC12.available()) 
  {        // If HC-12 has data
    incoming = Hc12ReadData();
    hc12Flag = true;
    receiveFlag = true;
    NewDataFlag = true;
  }

  mqtt.loop();

  if(NewDataFlag)
  {
    counter++;
    timeClient.update();
    dataTime = timeClient.getFormattedTime();
    id = CheckSenderId(incoming);

    if(rssiLoraFlag)
    {
      incoming = AddRssiToData(incoming);
      rssiLoraFlag = false;
    }

    MqttSend(incoming, hc12Flag);
    DisplayData();
    NewDataFlag = false;
  }
  
  mqtt.loop();

  if(WiFi.status() != WL_CONNECTED)
  {
    ConnectToWifi();
    DisplayBackground();
    DisplayData();
  }
 
  if(actualTime - lastUpdate > RSSI_UPDATE_TIME * 1000)
  {
    lastUpdate = actualTime;
    rssi = WiFi.RSSI();
    DisplayData();
  }
}