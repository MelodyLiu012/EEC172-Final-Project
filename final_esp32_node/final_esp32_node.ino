#include "WiFi.h"
#include "time.h"
#include "esp_now.h"
#define IR_PIN 13
#define NODE_ID 'A'

struct Packet {
  static const size_t length = 24;
  char validation[2];
  char node_id[2];
  char timestamp[20];
}; 

const char WIFI_SSID[] = "Shrimp";
const char WIFI_PASSWORD[] = "lunchtime";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -8 * 3600;
const int   daylightOffset_sec = 3600;
struct tm timeinfo;

esp_now_peer_info_t peerInfo;

// REPLACE WITH YOUR RECEIVER MAC Address
// 24:6F:28:7A:93:44 esp32 labeled '2'
// 94:3C:C6:34:6E:68 esp32 unlabeled
uint8_t broadcastAddress[] = {0x94, 0x3C, 0xC6, 0x34, 0x6E, 0x68};
// uint8_t broadcastAddress[] = {0x24, 0x6F, 0x28, 0x7A, 0x93, 0x44};

// buffer for sending packets
uint8_t data[Packet::length];
Packet* p;
bool detected = false;

volatile bool sent_flag = false;
volatile esp_now_send_status_t s;

// for testing
void printPacket(Packet* packet) {
  Serial.print("id = ");
  Serial.println(packet->node_id);
  Serial.print("time = ");
  Serial.println(packet->timestamp);
}

void connectWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
 
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Connecting to WiFi...");
    delay(500);
  }
  Serial.println("WiFi Connected");
}

void disconnectWiFi()
{
  WiFi.disconnect(true);
  Serial.println("Disconnected from WiFi");
}

void setupTime()
{
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  time_t now = time(nullptr);
  while (now < 24 * 3600) {
    Serial.println("Waiting for NTP time sync...");
    delay(500);
    now = time(nullptr);
  }

  if (getLocalTime(&timeinfo)) {
    char timeBuffer[20];
    strftime(timeBuffer, sizeof(timeBuffer), "%m/%d/%Y %H:%M:%S", &timeinfo);
    Serial.print("Current Time: ");
    Serial.println(timeBuffer);
  } else {
    Serial.println("Failed to obtain time");
  }
}

void setupESPNow()
{
  WiFi.mode(WIFI_STA);
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of transmitted packets
  esp_now_register_send_cb(OnDataSent);
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  Serial.println("ESPNow initialized");
}

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  s = status;
  sent_flag = true;
}

void setup()
{
  Serial.begin(115200);  // for debugging
  connectWiFi();
  setupTime();
  disconnectWiFi();
  setupESPNow();
  pinMode(IR_PIN, INPUT);

  p = (Packet*)malloc(Packet::length);
  p->validation[0] = 0x00;
  p->validation[1] = 0xFF;
  p->node_id[0] = NODE_ID;
  p->node_id[1] = '\0';
}

void loop()
{
  detected = !digitalRead(IR_PIN);
  if (detected) {
    Serial.println("Object detected");
    if (getLocalTime(&timeinfo)) {
      strftime(p->timestamp, sizeof(p->timestamp), "%m/%d/%Y %H:%M:%S", &timeinfo);
      esp_err_t result = esp_now_send(broadcastAddress, (uint8_t*)p, Packet::length);
      Serial.println(result == ESP_OK ? "Sent with success" : "Error sending the data");
      printPacket(p);
    } else {
      Serial.println("Failed to obtain time");
    }
  } else {
    Serial.println("Nothing detected");
  }
  if (sent_flag) {
    Serial.print("\r\nLast Packet Delivery Status:\t");
    Serial.println(s == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
    sent_flag = false;
  }
  Serial.println("");
  delay(500);
}
