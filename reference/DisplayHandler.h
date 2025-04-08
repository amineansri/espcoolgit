#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C

class Point {
public:
    float x;
    float y;
};

class DisplayHandler {
public:
    void init();
    void update(int direction);
    void updateWithIntensities(float* intensities);

private:
    Point center;
    Adafruit_SSD1306 display = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

    void drawIndicator(int d, int l, float theta, float x);

    int hold_counter = 0;
    const int hold_time = 5;
};