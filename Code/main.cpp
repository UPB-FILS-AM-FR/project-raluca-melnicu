#include <Arduino.h> 
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <BleMouse.h>

Adafruit_MPU6050 mpu;
BleMouse bleMouse("ESP32 Air Mouse", "ESP32", 100);

#define BOOT_BUTTON 0
bool lastButtonState = HIGH;

unsigned long lastMovementTime = 0;
bool sleeping = false;
bool scrollMode = false;
unsigned long lastTapTime = 0;

void setup() {

    Serial.begin(115200);

    pinMode(BOOT_BUTTON, INPUT_PULLUP);

    if (!mpu.begin()) {
        Serial.println("MPU6050 not found!");
        while (1);
    }

    mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

    bleMouse.begin();

    lastMovementTime = millis();

    Serial.println("Air Mouse has started!!!");
}

void loop() {

    if (bleMouse.isConnected()) {

        sensors_event_t a, g, temp;
        mpu.getEvent(&a, &g, &temp);

        float movement = abs(g.gyro.x) + abs(g.gyro.y) + abs(g.gyro.z);

        if (movement > 0.3) {
            lastMovementTime = millis();

            if (sleeping) {
                sleeping = false;
                Serial.println("Air Mouse is AWAKE!!! ^.^");
            }
        }

        if (millis() - lastMovementTime > 10000) {
            sleeping = true;
            Serial.println("Air Mouse is ASLEEP!-.-");
        }

        if (sleeping) {
            Serial.println("Still SLEEPING!-.-");
            delay(200);
            return;
        }

        bool buttonPressed = digitalRead(BOOT_BUTTON) == LOW;

        bool currentButtonState = digitalRead(BOOT_BUTTON);

        if (lastButtonState == HIGH && currentButtonState == LOW) {

            unsigned long now = millis();

            if (now - lastTapTime < 300) {
                scrollMode = !scrollMode;
                Serial.println(scrollMode ? "SCROLL MODE ON" : "SCROLL MODE OFF");
            }

            lastTapTime = now;
        }

        lastButtonState = currentButtonState;

        static bool dragging = false;

        if (buttonPressed && !dragging) {
            bleMouse.press(MOUSE_LEFT);
            dragging = true;
        }

        if (!buttonPressed && dragging) {
            bleMouse.release(MOUSE_LEFT);
            dragging = false;
        }

        static float smoothX = 0;
        static float smoothY = 0;

        float rawX = g.gyro.z * 8;
        float rawY = g.gyro.x * 8;

        smoothX = smoothX * 0.8 + rawX * 0.2;
        smoothY = smoothY * 0.8 + rawY * 0.2;

        int moveX = (int)smoothX;
        int moveY = (int)smoothY;

        if (abs(moveX) < 2) moveX = 0;
        if (abs(moveY) < 2) moveY = 0;

        if (scrollMode) {

            int scrollAmount = g.gyro.x * 3;

            if (abs(scrollAmount) < 2) scrollAmount = 0;

            bleMouse.move(0, 0, scrollAmount);

        } else {

            bleMouse.move(moveX, moveY);
        }

        delay(10);
    }
}
