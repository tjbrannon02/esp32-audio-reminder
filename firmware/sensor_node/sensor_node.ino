// This program is written to read the light level from an LDR circuit
// and then if it is above a chosen threshold it sends a signal over
// espnow to a receiver (adafruit feather v2)

// -----------------------------------------------
// Required Libraries 
// -----------------------------------------------
#include <WiFi.h>
#include <esp_now.h>

// -----------------------------------------------
// Setting Pins and etc. 
// -----------------------------------------------
const int LDR_PIN = 4;
const int LED_PIN = LED_BUILTIN;

// define message structure to be sent using pointer
typedef struct {

  int trigger;

} Message;

// defining instance of structure
Message msg;

// set reciever MAC
uint8_t receiverMAC[] = {0x14, 0x33, 0x5c, 0x99, 0x22, 0x00};

int lastState = 0;

// -------------------------------------------------
// Setup
// -------------------------------------------------
void setup() {

  Serial.begin(115200);

  // setting led pin
  pinMode(LED_PIN, OUTPUT);

  WiFi.mode(WIFI_STA);  //required for ESP-NOW
  WiFi.setChannel(1);
  WiFi.disconnect();    // prevents WiFi interference

  // checking if initialization was succesful
  if (esp_now_init() != ESP_OK) {

    Serial.println("ESP-NOW init failed");

    return;

  }

  Serial.println("ESP-NOW ready");

  // tell sender where you are sending to
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // confirming peer registration succeded
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {

    Serial.println("Failed to add peer");

    return;

  }

}

// ----------------------------------------------------
// Main Loop 
// ----------------------------------------------------
void loop() {

  // read ldr pin
  int lightLevel = analogRead(LDR_PIN);

  int threshold = 2000;

  // check light level and print (sanity check)
  Serial.print("LDR: ");
  Serial.print(lightLevel);
  Serial.print("\n");

  if (lightLevel > threshold && lastState == 0) {

    msg.trigger = 1;

    esp_now_send(receiverMAC, (uint8_t *)&msg, sizeof(msg));

    lastState = 1;

    digitalWrite(LED_PIN, HIGH);

  }

  if (lightLevel <= threshold && lastState == 1) {

    msg.trigger = 0;

    esp_now_send(receiverMAC, (uint8_t *)&msg, sizeof(msg));

    lastState = 0;

    digitalWrite(LED_PIN, LOW);

  }

  delay(1000);

}