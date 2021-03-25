// OLED Display
#define OLED_SCL            5		// GPIO5 / D1
#define OLED_SDA            4		// GPIO4 / D2
#define OLED_RESET          -1      // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_WIDTH        128     // OLED display width, in pixels
#define SCREEN_HEIGHT       64      // OLED display height, in pixels
#define SCREEN_ADDRESS      0x3C    // See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

// Lora
#define SFACTOR             10  //spreading factor
#define DEFAULT_PIN_SS      16          // GPIO16, D0
#define DEFAULT_PIN_DIO0    15          // GPIO15, D8
#define DEFAULT_PIN_RST     NOT_A_PIN   // Unused

// HC-12
#define RX_PIN              2  // D4
#define TX_PIN              0  // D3

// Misc
#define FIRMWARE_VERSION    "v1.1.0"
#define DEVICE_NAME         "Gateway"
#define RSSI_UPDATE_TIME    3

// MQTT
#define MQTT_MAX_PACKET_SIZE 256
#define LORA_TOPIC          "home/lora"
#define HC12_TOPIC          "home/433"
#define LORA_SEND_TOPIC     "gateway/send/lora"
#define HC12_SEND_TOPIC     "gateway/send/433"

// Interface
#define BOTTOM_TEXT_Y       62
#define ID_TEXT_X           52
#define UPPER_ID_TEXT_Y     14
#define LOWER_ID_TEXT_Y     44
#define CURSOR_X            40
#define TIME_X              15
#define COUNTER_X           85
#define WIFI_ICON_Y         15

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


