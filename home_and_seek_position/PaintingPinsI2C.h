#pragma once
#include "PaintingPins.h"
#include "AS5600.h"
#include "Wire.h"

class PaintingPinsI2C : public PaintingPins {
private:
    AS5600 as5600;
    int sdaPin, sclPin, dirPin;
    float i2cEncoderOffset = 0;

public:
    PaintingPinsI2C(int fwd_pin, int rev_pin, int enc_pin, int home_pin,
                    int sda_pin, int scl_pin)
        : PaintingPins(fwd_pin, rev_pin, enc_pin, home_pin),
          as5600(&Wire2),
          sdaPin(sda_pin), sclPin(scl_pin) {}

    void begin() {
        PaintingPins::begin();
        Wire2.setSDA(sdaPin);
        Wire2.setSCL(sclPin);
        Wire2.begin();
        as5600.begin();
        as5600.setDirection(AS5600_CLOCK_WISE);
        if (!as5600.isConnected()) {
            Serial.println("WARNING: AS5600 not connected!");
        }
        else {Serial.println("Connected to as5600!");}
    }

    float readEncoder() override {
        return as5600.readAngle() * (360.0f/4096.0f);
    }

    void zeroI2C() {
        i2cEncoderOffset = as5600.readAngle() / 10.0f;
    }

    float analogEncoder() {
        return ((analogRead(encPin) + analogRead(encPin) + analogRead(encPin)) / (1024.0 * 3)) * 360;
    }

    bool isI2CConnected() {
        return as5600.isConnected();
    }
};