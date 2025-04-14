#include "DFRobot_GDL.h"
#include "driver/i2s.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <WiFiAP.h>

#define TFT_DC  D2
#define TFT_CS  D6
#define TFT_RST D3
#define TFT_BL  D7
#define I2S_SCK_0  12
#define I2S_WS_0   15
#define I2S_SD_0   4
#define SAMPLE_RATE     44000     // 16kHz bandwidth
#define SAMPLES_PER_PACKET 120
#define MIC_BYTES_PER_PACKET SAMPLES_PER_PACKET*3
#define BUFFER_SIZE     SAMPLES_PER_PACKET*8     // Must be multiple of 6 (24b stereo frame)

// UDP configuration
#define UDP_TX_PORT 3333       // Port for sending audio data
#define UDP_RX_PORT 3334       // Port for receiving text data
#define MAX_UDP_PACKET_SIZE 255 // Maximum UDP packet size for text

int32_t buffer0[BUFFER_SIZE];  // I2S0 (Mics 1 & 2)
uint8_t micBuffers[2][MIC_BYTES_PER_PACKET];  // Split buffers for each mic

DFRobot_ST7789_240x320_HW_SPI screen(/*dc=*/TFT_DC,/*cs=*/TFT_CS,/*rst=*/TFT_RST,/*bl=*/TFT_BL);

String inputText = ""; 
const char *ssid = "D-Scribe glasses";
const char* password = "password";
WiFiUDP txUdp;  // UDP instance for transmitting audio
WiFiUDP rxUdp;  // UDP instance for receiving text
const char* laptop_ip = "192.168.4.2";
static uint8_t seq = 0;
uint16_t iter = 0;
int data_send = 0;
unsigned long int timeddd = 0;
char packetBuffer[MAX_UDP_PACKET_SIZE]; // Buffer to hold incoming UDP packet

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

void setupWiFi() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  delay(1000);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
}

// ====== UDP Setup ======
void setupUDP() {
  txUdp.begin(UDP_TX_PORT);
  rxUdp.begin(UDP_RX_PORT);
  Serial.println("[UDP] TX UDP Started on port " + String(UDP_TX_PORT));
  Serial.println("[UDP] RX UDP Started on port " + String(UDP_RX_PORT));
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
  iter++;

  // Read from I2S peripheral (blocking but with short timeout)
  esp_err_t err0 = i2s_read(I2S_NUM_0, buffer0, BUFFER_SIZE, &bytesRead0, portMAX_DELAY);
  if (err0 != ESP_OK || bytesRead0 != BUFFER_SIZE) {
    Serial.printf("%d\t%d\t%d\n", err0, bytesRead0, BUFFER_SIZE);
    return;
  }
  splitBuffer(buffer0, micBuffers[0], micBuffers[1], bytesRead0 / sizeof(int32_t));  // Mics 1 & 2

  // Send via UDP
  txUdp.beginPacket(laptop_ip, UDP_TX_PORT);
  txUdp.write((const uint8_t*)"AUDI", 4);  // Header
  txUdp.write(seq++);  // Sequence number
  
  // Send mic data
  txUdp.write('1');  // Mic ID ('1')
  txUdp.write(micBuffers[0], MIC_BYTES_PER_PACKET);
  data_send += 1449;
  txUdp.endPacket();
}

void checkForIncomingText() {
  // Check if there's data available to read from UDP
  int packetSize = rxUdp.parsePacket();
  if (packetSize) {
    // We've received a packet, read the data
    int len = rxUdp.read(packetBuffer, MAX_UDP_PACKET_SIZE);
    if (len > 0) {
      packetBuffer[len] = 0; // Null-terminate the string
      
      // Process the incoming text
      String receivedText = String(packetBuffer);
      Serial.println(receivedText);
      
      // Add to inputText
      inputText += receivedText;
      
      // Update display
      displayText();
    }
  }
}

void setup() {
  Serial.begin(115200);
  setupWiFi();
  setupI2S();
  setupUDP();
  
  timeddd = micros();
  
  screen.begin();
  screen.setRotation(3);  // Set screen rotation to 270 degrees
  screen.fillScreen(COLOR_RGB565_BLACK);
  screen.setTextSize(1);  // Text size
  screen.setFont(&FreeMono12pt7b);  // Font size
  screen.setCursor(8, 50);  // Text position, x=8 y=50
  screen.setTextColor(COLOR_RGB565_GREEN);  // Text color
  screen.setTextWrap(true);  // Enable text wrapping
  screen.print("D-Scribe");  // Display the full input text
}

void loop() {
  // Handle audio capture and streaming
  captureAndStream();
  
  // Check for incoming text messages
  checkForIncomingText();
  
  // Calculate and log timing information (if needed)
  unsigned long int timediff = (micros() - timeddd);
  data_send = 0;
  timeddd = micros();
}

void displayText() {
  screen.fillRect(0, 0, screen.width(), screen.height(), COLOR_RGB565_BLACK);

  // Redraw headers
  screen.setTextSize(1);
  screen.setFont(&FreeMono12pt7b);
  screen.setCursor(8, 50);
  screen.setTextColor(COLOR_RGB565_GREEN);
  screen.setTextWrap(true);
  screen.print("D-Scribe");
  screen.setCursor(140, 50);
  screen.setTextColor(COLOR_RGB565_YELLOW);

  // Process and wrap input text
  String wrappedText = "";
  String word = "";
  String currentLine = "";
  int charCount = inputText.length();

  for (unsigned int i = 0; i < inputText.length(); i++) { 
    char c = inputText.charAt(i); //hyphenation of the words

    if (c == ' ' || c == '\n') {
      // Check if adding the word would exceed 22 characters
      if ((currentLine.length() + word.length() + 1) > 22) {
        wrappedText += currentLine + "\n";
        currentLine = word;
      } else {
        if (currentLine.length() > 0) currentLine += " ";
        currentLine += word;
      }
      word = "";  // Reset word
    } else {
      word += c;  // Build the word
    }
  }

  // Add the last word
  if ((currentLine.length() + word.length() + 1) > 22) {
    wrappedText += currentLine + "\n" + word;
  } else {
    if (currentLine.length() > 0) currentLine += " ";
    currentLine += word;
    wrappedText += currentLine;
  }

  // Display wrapped text
  screen.setCursor(0, 69);
  screen.setTextColor(COLOR_RGB565_WHITE);
  screen.setTextWrap(true);
  screen.print(wrappedText);
  
  // Clear screen if too many characters
  if (charCount > 80) {
    delay(6000);
    screen.fillRect(0, 55, screen.width(), screen.height() - 65, COLOR_RGB565_BLACK);
    inputText = ""; // Reset the current set of words
  }
}