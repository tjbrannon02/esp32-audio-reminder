/* Play Audio File from flash mem using BackgroundAudio */

// ---------------------------------------------------
// Required libraries
// ---------------------------------------------------
#include <LittleFS.h>         // Library to use flash memory on adafruit to store audio files
#include <BackgroundAudio.h>  // Audio library used to send I2S audio data
#include <ESP32I2SAudio.h>    // Custom BackgroundAudio wrapper required for ESP32 I2S
#include <esp_now.h>          // Needed for espnow communication
#include <WiFi.h>             // Needed for espnow as well as clearing state to ensure no interruption of the espnow communication

// ---------------------------------------------------
// Define pinout for Adafruit Feather v2 ESP32
// ---------------------------------------------------
const int PIN_BCLK = 32;
const int PIN_LRC  = 33;
const int PIN_DOUT = 27;

const int REED_PIN = 14;

// ---------------------------------------------------
// Object creation
// ---------------------------------------------------
ESP32I2SAudio audioOut(PIN_BCLK, PIN_LRC, PIN_DOUT);
BackgroundAudioWAV wavDecoder(audioOut);

// File handle for playback tracking
File audioFile;
uint8_t fileBuffer[512]; // Buffer chunk size to read from LittleFS
bool playbackFinishedReported = true;

// Tracking state of reed switch
int lastState = LOW;

// Structure for message from espnow
typedef struct {

  int trigger;

} Message;

// Instance of the message structure
Message msg;

// ----------------------------------------------------
// Callback function to run on data recieve
// ----------------------------------------------------
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {

  memcpy(&msg, incomingData, sizeof(msg));

  Serial.println("State of Trigger: ");

  Serial.println(msg.trigger);

}

// ----------------------------------------------------
// Setup
// ----------------------------------------------------
void setup() {

  Serial.begin(115200);

  delay(1000);

  // Set as a wifi station
  WiFi.mode(WIFI_STA);

  // Set wifi channel to 1
  WiFi.setChannel(1);

  // Set pin mode for reed pin
  pinMode(REED_PIN, INPUT_PULLUP);

  // Check if ESP-NOW initialized correctly
  if (esp_now_init() != ESP_OK) {

    Serial.println("Error Initializing ESP-NOW");

    return;

  }

  Serial.println("Mounting LittleFS...");

  // Confirm successful mounting making flash accessible
  if (!LittleFS.begin()) {

    Serial.println("LittleFS mount failed");

    return;

  }

  Serial.println("LittleFS OK");

  // ----------------------------------------------------
  // Listing files
  // ----------------------------------------------------
  Serial.println("Listing files:");

  File root = LittleFS.open("/");

  File f = root.openNextFile();

  while (f) {

    Serial.print("FILE: ");

    Serial.println(f.name());

    f = root.openNextFile();

  }

  Serial.println("\n");

  // Set initial gain/volume (0.0 to 1.0 range typical)
  wavDecoder.setGain(0.8);

  // Initialize the background audio processing architecture
  wavDecoder.begin();

  // Read last state of reed pin
  lastState = digitalRead(REED_PIN);

  // call back on data recived
  esp_now_register_recv_cb(OnDataRecv);

}

void loop() {

  // Read current state of reed pin
  int currentState = digitalRead(REED_PIN);

  if (currentState == HIGH && lastState == LOW) {

    // Checking to make sure we are not already playing a track and if not create the audio files
    if (!audioFile) {

      Serial.println("Magnet removed! Opening Audio File... ");

      // Choose audio file based on packet from C6
      if (msg.trigger == 0) {

        // Open chosen audi file using littleFS
        audioFile = LittleFS.open("/LightsOff.wav", "r");

      }
      else {

        audioFile = LittleFS.open("/DRLightOn.wav", "r");

      }

      // check for failure in opening audio file
      if (!audioFile) {

        Serial.println("Failed to open audio file!");

      }
      else {

        Serial.println("Playback Triggered Succesfully!");

        // set playback finished trigger to false
        playbackFinishedReported = false;

      }

    }

  }

  // update last state
  lastState = currentState;

  // If the file is open, feed the background decoder's buffer
  if (audioFile) {

    // Check if the background buffer has space for another sector chunk
    while (audioFile && wavDecoder.availableForWrite() >= sizeof(fileBuffer)) {

      // Read bytes from audio file into the buffer
      int bytesRead = audioFile.read(fileBuffer, sizeof(fileBuffer));
      
      // If there is data in the buffer 
      if (bytesRead > 0) {

        // Write data from buffer into the wav decoder
        wavDecoder.write(fileBuffer, bytesRead);

      }

      // Check for End-of-File (EOF) or short reads
      if (bytesRead < sizeof(fileBuffer)) {

        // closing the audio file
        audioFile.close();

        Serial.println("File completely read and sent to buffer.");

      }

    }

  }

  // Check if the background task is fully done executing the remaining queued audio
  if (!audioFile && wavDecoder.done() && !playbackFinishedReported) {

    Serial.println("Playback completely finished!");

    playbackFinishedReported = true;

  }

  // Small delay to ease CPU usage
  delay(10);

}