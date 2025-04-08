#include "DisplayHandler.h"

// Call this before using display
void DisplayHandler::init() {
    display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
    display.clearDisplay();
    display.display();

    center.x = SCREEN_WIDTH / 2;
    center.y = SCREEN_HEIGHT / 2;

    display.drawPixel(0, 0, WHITE);
    display.display();
    delay(1000);
}

// Direction [integer between 0 and 8, 0 is off, 1 is forward, then clockwise up to 8]
void DisplayHandler::update(int direction) {
    if (direction == 0 && hold_counter < hold_time) {
        hold_counter++;
    }
    else {
        hold_counter = 0;
        display.clearDisplay();

        for (int i = 7; i >= 0; i--) {
            drawIndicator(15, 4, 2 * PI / 8 * i - PI / 2, ((direction - 1) == i) * 10);
        }
        display.display();
    }
}

// Intensities [intensities is float[8], sets bar i at intensities[i] ]
void DisplayHandler::updateWithIntensities(float* intensities) {
    display.clearDisplay();
    for (uint8_t i = 0; i < 8; i++) {
        drawIndicator(15, 4, 2 * PI / 8 * i, intensities[i]);
    }
    display.display();
}

void DisplayHandler::drawIndicator(int d, int l, float theta, float x) {
    for (float dd = d; dd <= d + x; dd += 0.1) {
        Point PA, PB;
        PA.x = center.x + cos(theta) * dd - sin(theta) * l;
        PA.y = center.y + sin(theta) * dd + cos(theta) * l;

        PB.x = center.x + cos(theta) * dd + sin(theta) * l;
        PB.y = center.y + sin(theta) * dd - cos(theta) * l;

        display.drawLine(PA.x, PA.y, PB.x, PB.y, WHITE);
    }
}
