#include <Arduino.h>
#include "driver/i2s.h"
#include <WiFi.h>
#include <WiFiUdp.h>

// ====== I2S Configuration ======
// I2S0 (Stereo: Mics 1 & 2)
#define I2S_SCK_0  32
#define I2S_WS_0   33
#define I2S_SD_0   27

// I2S1 (Stereo: Mics 3 & 4)
#define I2S_SCK_1  19
#define I2S_WS_1   18
#define I2S_SD_1   5


// ====== I2S Configuration ======
// I2S0 (Stereo: Mics 1 & 2)
// #define I2S_SCK_0  2
// #define I2S_WS_0   15
// #define I2S_SD_0   27

// // I2S1 (Stereo: Mics 3 & 4)
// #define I2S_SCK_1  0
// #define I2S_WS_1   4
// #define I2S_SD_1   13


#define SAMPLE_RATE     32000     // 16kHz bandwidth
#define BUFFER_SIZE     90      // Must be multiple of 6 (24b stereo frame)
#define SAMPLES_PER_PACKET (BUFFER_SIZE / 6 * 4) // Total samples per UDP packet

// ====== WiFi Configuration ======
const char* ssid = "CaptionGlasses";
const char* password = "12345678";
WiFiUDP udp;
const char* laptop_ip = "192.168.4.2";
const uint16_t udp_port = 3333;

// Buffers
uint32_t buffer0[BUFFER_SIZE];  // I2S0 (Mics 1 & 2)
uint32_t buffer1[BUFFER_SIZE];  // I2S1 (Mics 3 & 4)
uint8_t micBuffers[4][BUFFER_SIZE * 3 / 2];  // Split buffers for each mic

// ====== Data and time variables ======
int data_send = 0;
unsigned long int timeddd = 0;

void setupI2S() {
  i2s_config_t i2s_config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_24BIT,  // 24-bit mode
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,  // Stereo
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .dma_buf_count = 8,        // More buffers for stability
    .dma_buf_len = BUFFER_SIZE,        // Smaller buffers for lower latency
    .use_apll = false           // Better clock stability
  };

    i2s_config_t i2s_config1 = {
    .mode = i2s_mode_t(I2S_MODE_SLAVE | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_24BIT,  // 24-bit mode
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,  // Stereo
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .dma_buf_count = 8,        // More buffers for stability
    .dma_buf_len = BUFFER_SIZE,        // Smaller buffers for lower latency
    .use_apll = false           // Better clock stability.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
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
  // I2S1 Setup (Mics 3 & 4)
  i2s_pin_config_t pin1 = {
    .bck_io_num = I2S_SCK_1,
    .ws_io_num = I2S_WS_1,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD_1
  };
  i2s_driver_install(I2S_NUM_1, &i2s_config1, 0, NULL);
  i2s_set_pin(I2S_NUM_1, &pin1);
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

void splitBuffer(const uint32_t* stereoBuffer, uint8_t* left, uint8_t* right) {
    size_t leftIndex = 0;
    size_t rightIndex = 0;

    for (size_t i = 0; i < BUFFER_SIZE/6; ++i) {
        uint32_t sample = stereoBuffer[i];
        Serial.printf("%d\n", sample);

        // Extract 24-bit value: byte0 = LSB, byte1 = mid, byte2 = MSB
        uint8_t byte0 = (sample >>  0) & 0xFF;
        uint8_t byte1 = (sample >>  8) & 0xFF;
        uint8_t byte2 = (sample >> 16) & 0xFF;

        if (i % 2 == 0) {
            // Even index -> Left channel
            left[leftIndex++] = byte0;
            left[leftIndex++] = byte1;
            left[leftIndex++] = byte2;
        } else {
            // Odd index -> Right channel
            right[rightIndex++] = byte0;
            right[rightIndex++] = byte1;
            right[rightIndex++] = byte2;
        }

        // Debug output
        // Serial.printf("Sample %02zu: %#010x -> [%02x %02x %02x] => %s\n",
            // i, sample, byte2, byte1, byte0, (i % 2 == 0) ? "Left" : "Right");
    }
    // Serial.printf("l: %d, r: %d", leftIndex, rightIndex);
}

void captureAndStream() {
  size_t bytesRead0, bytesRead1;

  // Read from both I2S peripherals (blocking)
  esp_err_t err0 = i2s_read(I2S_NUM_0, buffer0, BUFFER_SIZE, &bytesRead0, portMAX_DELAY);
  esp_err_t err1 = i2s_read(I2S_NUM_1, buffer1, BUFFER_SIZE, &bytesRead1, portMAX_DELAY);
   if (err0 != ESP_OK || err1 != ESP_OK || bytesRead0 != BUFFER_SIZE || bytesRead1 != BUFFER_SIZE) {
    Serial.println("I2S Read Error!");
    return;
  }


  // Split stereo buffers into individual mic streams
  splitBuffer(buffer0, micBuffers[0], micBuffers[1]);  // Mics 1 & 2
  splitBuffer(buffer1, micBuffers[2], micBuffers[3]);  // Mics 3 & 4

  // Send via UDP
  udp.beginPacket(laptop_ip, udp_port);
  udp.write((const uint8_t*)"AUDI", 4);  // Header
  static uint8_t seq = 0;
  udp.write(seq++);  // Sequence number

  // Send all 4 mics' data (fixed size: 360 bytes each)
  for (int mic = 0; mic < 4; mic++) {
    udp.write('1' + mic);  // Mic ID ('1' to '4')

    udp.write(micBuffers[mic], BUFFER_SIZE / 2);
  }
  data_send += BUFFER_SIZE * 2;
  udp.endPacket();
  // for (int i = 0; i < BUFFER_SIZE / 2; i++) {
  //   Serial.printf("%ld\t%ld\t%ld\t%ld\n", micBuffers[0][i], micBuffers[1][i], micBuffers[2][i], micBuffers[3][i]);
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
    data_send = 8*data_send;
    Serial.print("Data send in 1 sec: ");
    Serial.println(data_send);
    data_send = 0;
    timeddd = millis();
  }
}