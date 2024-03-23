#include "esp_now.h"
#include <WiFi.h>
#define RXD2 16
#define TXD2 17

struct Packet {
  static const size_t length = 24;
  char validation[2];
  char node_id[2];
  char timestamp[20];
};

Packet* p;
volatile bool received_flag = false;

// for testing
void printPacket(Packet* packet) {
  Serial.println("Packet received: ");
  Serial.print("id = ");
  Serial.println(packet->node_id);
  Serial.print("time = ");
  Serial.println(packet->timestamp);
  Serial.println("");
}

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(p, incomingData, len);
  received_flag = true;
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);  // for UART

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    return;
  }

  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(OnDataRecv);

  p = (Packet*)malloc(Packet::length);
  Serial.println("Ready to receive");
}

void loop() {
  if (received_flag) {
    if (p->validation[0] == 0x00 && p->validation[1] == 0xFF) {
      printPacket(p);
      Serial2.write((uint8_t*)p, Packet::length);
    } else {
      Serial.println("Received invalid packet");
    }
    received_flag = 0;
  }
}
