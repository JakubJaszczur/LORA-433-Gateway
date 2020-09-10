#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include "SSD1306.h"
#include <ArduinoJson.h>
#include <SoftwareSerial.h>

#define DEFAULT_PIN_SS    16          // GPIO16, D0
#define DEFAULT_PIN_DIO0  15          // GPIO15, D8
#define DEFAULT_PIN_RST   NOT_A_PIN   // Unused
#define OLED_SCL 5							// GPIO5 / D1
#define OLED_SDA 4							// GPIO4 / D2
#define WIFI_ON
#define MQTT_MAX_PACKET_SIZE 256
#define SFACTOR 10  //spreading factor
#define RX_PIN  2  // D4
#define TX_PIN  0  // D3

#ifdef WIFI_ON

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

//WiFi Settings
const char* ssid = "TP-LINK_7B10";
const char* password = "dzesi1234";

//MQTT Settings
const char* mqtt_server = "192.168.0.113";
//const char* mqtt_user = "admin";
//const char* mqtt_password = "sunrise2010";
#endif

double timeLast = 0;

//LoRa frequency
uint32_t  freq = 868100000; 					// Channel 0, 868.1 MHz
//uint32_t  freq = 868300000; 					// Channel 1, 868.3 MHz
//uint32_t  freq = 868500000; 					// in Mhz! (868.5)
//uint32_t  freq = 867100000; 					// in Mhz! (867.1)
//uint32_t  freq = 867300000; 					// in Mhz! (867.3)
//uint32_t  freq = 867500000; 					// in Mhz! (867.5)
//uint32_t  freq = 867700000; 					// in Mhz! (867.7)
//uint32_t  freq = 867900000; 					// in Mhz! (867.9)
//uint32_t  freq = 868800000; 					// in Mhz! (868.8)
//uint32_t  freq = 869525000; 					// in Mhz! (869.525)

SSD1306  display(0x3c, OLED_SDA, OLED_SCL);		// (i2c address of display(0x3c or 0x3d), SDA, SCL) on wemos
SoftwareSerial HC12(RX_PIN, TX_PIN); // HC-12 TX Pin, HC-12 RX Pin

#ifdef WIFI_ON
WiFiClient espClient;
PubSubClient client(espClient);

String rssiToSend = "";
static int counter = 0;
int senderID = 0;

void DisplayData(String text)
{ 
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 0, "ID: " );
  display.drawString(22, 0, String(senderID));
  display.drawString(55, 0, "No: " );
  display.drawString(82, 0, String(counter));
  display.drawString(0, 20, "RSSI: " );
  display.drawString(45, 20, rssiToSend);
  display.drawString(0, 40, text);
  display.display();
}

void ConnectToWifi()
{
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 0, "Connecting to:" );
  display.drawString(0, 20, ssid);
  display.display();

  WiFi.begin(ssid, password);
  WiFi.softAPdisconnect (true); //disable hotspot mode

  int cursor = 0;

  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
    display.drawString(cursor, 40, "." );
    display.display();
    cursor = cursor + 2;
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  display.clear();
  display.drawString(0, 0, "WiFi connected");
  display.display();
}

void reconnect() 
{
  // Loop until we're reconnected
  while (!client.connected()) 
  {
    Serial.print("Attempting MQTT connection...");

    // Create a random client ID
    String clientId = "D1_MINI";
    
    // Attempt to connect
    if (client.connect(clientId.c_str())) 
    {
      Serial.println("connected");
      //client.subscribe("basic/time/time");
    } 

    else 
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println("try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
#endif

void setup() 
{
  Serial.begin(9600);
  HC12.begin(9600);               // Serial port to HC12

  pinMode(D3, INPUT);

  //display.init();
  //display.flipScreenVertically();
  LoRa.setPins(DEFAULT_PIN_SS, DEFAULT_PIN_RST, DEFAULT_PIN_DIO0);

  Serial.println("LoRa Receiver");

  if (!LoRa.begin(freq)) {
    Serial.println("Starting LoRa failed!");
    //while (1);
  }

  LoRa.setSpreadingFactor(SFACTOR);           // ranges from 6-12,default 7 see API docs

  ConnectToWifi();
  client.setServer(mqtt_server, 1883);
}

void loop() 
{
  double actualTime = millis();
  char message [200];
  String incoming = "";

  const size_t capacity = JSON_OBJECT_SIZE(12) + 80;
  DynamicJsonDocument data(capacity);

  if((actualTime - timeLast) > 3500)
  {
    Serial.println("Waiting for message...");
    timeLast = actualTime;

    DisplayData("Waiting...");
  }

  // try to parse packet
  if(LoRa.parsePacket()) 
  {
    // read packet
    while (LoRa.available()) {
      incoming += (char)LoRa.read();
    }

    counter ++;

    rssiToSend = String(LoRa.packetRssi());
    Serial.println(incoming);

    deserializeJson(data, incoming);

    senderID = data["id"];
    // received a packet
    Serial.print("Data received from ");
    Serial.print(senderID);
    // print RSSI of packet
    Serial.print(" with RSSI ");
    Serial.println(rssiToSend);

    data["rssi"] = rssiToSend.toInt();
    String msg = "";
    serializeJson(data, msg);
    Serial.println(msg);
    msg.toCharArray(message, sizeof(message));

    //Check MQTT connection
    if (!client.connected()) 
    {
      reconnect();
    }

    client.publish("home/lora", message);
    
    DisplayData("Lora sent!");
  }

  if(HC12.available()) 
  {        // If HC-12 has data
    incoming = HC12.readString();
    Serial.print(incoming);      // Send the data to Serial monitor

    deserializeJson(data, incoming);

    senderID = data["id"];
    // received a packet
    Serial.print("Data received from ");
    Serial.print(senderID);
    Serial.print(" ");
    rssiToSend = "";

    incoming.toCharArray(message, sizeof(message));

    Serial.print(message);

    //Check MQTT connection
    if (!client.connected()) 
    {
      reconnect();
    }

    client.publish("home/433", message);

    DisplayData("433MHz sent!");

    counter ++;
  }
}