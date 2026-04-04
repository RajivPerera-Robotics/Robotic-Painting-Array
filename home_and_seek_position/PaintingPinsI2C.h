#pragma once
#include "PaintingPins.h"
#include "AS5600.h"
#include "TCA9548.h"
#include "Wire.h"

class PaintingPinsI2C : public PaintingPins {
private:
    AS5600 as5600;
    PCA9546& multiplexer;
    int sdaPin, sclPin, dirPin, multChannel;
    
    float i2cEncoderOffset = 0;

public:
    PaintingPinsI2C(int fwd_pin, int rev_pin, int enc_pin, int home_pin,
                    int sda_pin, int scl_pin, PCA9546& multi_plexer, int mult_channel)
        : PaintingPins(fwd_pin, rev_pin, enc_pin, home_pin),
          as5600(&Wire2), multiplexer(multi_plexer),
          sdaPin(sda_pin), sclPin(scl_pin),
          multChannel(mult_channel) {}

    void begin() {
        PaintingPins::begin();
        Wire2.setSDA(sdaPin);
        Wire2.setSCL(sclPin);
        Wire2.begin();
        if (multiplexer.begin()==false){
            Serial.print("Multiplexer not connected");
            Serial.print(" for board on multiplexer ");
            Serial.print(multChannel);
        }
        multiplexer.enableChannel(multChannel);
        as5600.begin();
        as5600.setDirection(AS5600_CLOCK_WISE);
        if (!as5600.isConnected()) {
            Serial.println("WARNING: AS5600 not connected!");
        }
        else {Serial.println("Connected to as5600!");}
        Serial.print("Setup complete for painting on multichannel");
        Serial.println(multChannel);
    }

    float readEncoder() override {
        multiplexer.enableChannel(multChannel);
        return as5600.readAngle() * (360.0f/4096.0f);
    }

    void zeroI2C() {
        multiplexer.enableChannel(multChannel);
        i2cEncoderOffset = as5600.readAngle() / 10.0f;
    }

    float analogEncoder() {
        return ((analogRead(encPin) + analogRead(encPin) + analogRead(encPin)) / (1024.0 * 3)) * 360;
    }

    bool isI2CConnected() {
        multiplexer.enableChannel(multChannel);
        return as5600.isConnected();
    }
};