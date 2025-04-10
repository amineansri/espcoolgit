#include <Arduino.h>
#include "driver/i2s.h"
#include <WiFi.h>
#include <WiFiUdp.h>

// Square microphones
// ====== I2S Configuration ======
// I2S0 (Stereo: Mics 1 & 2)
// #define I2S_SCK_0  32
// #define I2S_WS_0   33
// #define I2S_SD_0   27


// Round microphones
// ====== I2S Configuration ======
// I2S0 (Stereo: Mics 1 & 2)
#define I2S_SCK_0  2
#define I2S_WS_0   15
#define I2S_SD_0   27

#define SAMPLE_RATE     32000     // 16kHz bandwidth
#define SAMPLES_PER_PACKET 120
// #define SAMPLES_PER_PACKET 480
#define MIC_BYTES_PER_PACKET SAMPLES_PER_PACKET*3
#define BUFFER_SIZE     SAMPLES_PER_PACKET*8      // Must be multiple of 6 (24b stereo frame)
// #define BUFFER_SIZE     SAMPLES_PER_PACKET*2      // Must be multiple of 6 (24b stereo frame)


// ====== WiFi Configuration ======
const char* ssid = "CaptionGlasses1";
const char* password = "12345678";
WiFiUDP udp;
const char* laptop_ip = "192.168.4.2";
const uint16_t udp_port = 3333;
static uint8_t seq = 0;

// Buffers
int32_t buffer0[BUFFER_SIZE];  // I2S0 (Mics 1 & 2)
uint8_t micBuffers[2][MIC_BYTES_PER_PACKET];  // Split buffers for each mic

// ====== Data and time variables ======
int data_send = 0;
unsigned long int timeddd = 0;

void setupI2S() {
  i2s_config_t i2s_config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_24BIT,  // 24-bit mode
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,  // Stereo
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .dma_buf_count = 4,        // More buffers for stability
    .dma_buf_len = BUFFER_SIZE,        // Smaller buffers for lower latency
    .use_apll = false           // Better clock stability
  };

  // I2S0 Setup (Mics 1 & 2)
  i2s_pin_config_t pin0 = {
    .bck_io_num = I2S_SCK_0,
    .ws_io_num = I2S_WS_0,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD_0
  };
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin0);
  }

// ====== Wi-Fi Setup ======
void setupWiFi() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  delay(1000);
  Serial.println("[WiFi] SoftAP Started");
  Serial.print("[WiFi] IP Address: ");
  Serial.println(WiFi.softAPIP());
}

// ====== UDP Setup ======
void setupUDP() {
  udp.begin(udp_port);
  Serial.println("[UDP] UDP Started");
}

void splitBuffer(const int32_t* stereoBuffer, uint8_t* left, uint8_t* right, int numsamples) {
    for (size_t i = 0; i < numsamples; i += 2) {
      size_t j = 3*(i/2);
      left[j    ] = (stereoBuffer[i] >> 8) & 0xFF;
      left[j + 1] = (stereoBuffer[i] >> 16) & 0xFF;
      left[j + 2] = (stereoBuffer[i] >> 24) & 0xFF;
      right[j    ] = (stereoBuffer[i + 1] >> 8) & 0xFF;
      right[j + 1] = (stereoBuffer[i + 1] >> 16) & 0xFF;
      right[j + 2] = (stereoBuffer[i + 1] >> 24) & 0xFF;
    }
}

void captureAndStream() {
  size_t bytesRead0;

  // Read from both I2S peripherals (blocking)
  esp_err_t err0 = i2s_read(I2S_NUM_0, buffer0, BUFFER_SIZE, &bytesRead0, portMAX_DELAY);
   if (err0 != ESP_OK || bytesRead0 != BUFFER_SIZE) {
    Serial.println("I2S Read Error!");
    return;
  }

  // Serial.printf("%d\n", bytesRead0);
  // Split stereo buffers into individual mic streams
  splitBuffer(buffer0, micBuffers[0], micBuffers[1], bytesRead0 / sizeof(int32_t));  // Mics 1 & 2

  // Send via UDP
  udp.beginPacket(laptop_ip, udp_port);
  udp.write((const uint8_t*)"AUDI", 4);  // Header
  udp.write(seq++);  // Sequence number

  // Send all 4 mics' data (fixed size: 360 bytes each)
  udp.write('1');  // Mic ID ('1' to '4')

  udp.write(micBuffers[0], MIC_BYTES_PER_PACKET);
  data_send += 1449;
  udp.endPacket();

  for (int i = 0; i < MIC_BYTES_PER_PACKET; i += 3) {
    int32_t m1 = (micBuffers[0][i+2] << 16 |  micBuffers[0][i+1] << 8 | micBuffers[0][i]) | (micBuffers[0][i+2] >> 7 == 0x01 ? 0xFF000000 : 0x00000000);
    Serial.printf("%d\n", m1);
    // Serial.printf("0x%08x\n", m1);
  }
  // for (int i = 0; i < MIC_BYTES_PER_PACKET; i++){
  //   Serial.printf("%d\n", micBuffers[0][i]);
  //   // Serial.printf("0x%08x\n", micBuffers[i]);
  // }
}

// ====== Main Program ======
void setup() {
  Serial.begin(115200);
  setupWiFi();  // Your existing WiFi setup
  setupI2S();
  udp.begin(udp_port);
  timeddd = millis();
}

void loop() {

  captureAndStream();
  unsigned long int timediff = (millis() - timeddd) / 1000;
  if (timediff >= 1){
    // Serial.printf("Data send in 1 sec: %d\n", data_send);
    // Serial.print("Data send in 1 sec: ");
    // Serial.println(data_send);
    data_send = 0;
    timeddd = millis();
  }
}