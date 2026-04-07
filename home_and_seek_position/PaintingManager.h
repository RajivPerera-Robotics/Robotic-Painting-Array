#pragma once
#include "PaintingPins.h"
#include "TCA9548.h"

class PaintingManager {
private:
  PaintingPinsI2C** paintings;
  int numPaintings;

public:
  PaintingManager(PaintingPinsI2C* p[], int count)
    : numPaintings(count) {
    paintings = new PaintingPinsI2C*[count];
    for (int i = 0; i < count; i++) paintings[i] = p[i];
  }

  float enforce360(float input){
    float output;
    if (input<0.0){
      output = input+360;
    }
    else if (input>360.0){
      output = input-360;
    }
    else{
      output = input;
    }
    return output;
  }
  void begin() {
    for (int i = 0; i < numPaintings; i++) paintings[i]->begin();
  }

  void update() {
    for (int i = 0; i < numPaintings; i++) paintings[i]->update();
  }

  bool allIdle() {
    for (int i = 0; i < numPaintings; i++) {
      if (!paintings[i]->isIdle()) return false;
    }
    return true;
  }

  void cascadeHome(uint32_t stepDelayMs = 500, int speed=MTR_MIN) {
    for (int i = 0; i < numPaintings; i++) {
      paintings[i]->delayHoming(speed, i * stepDelayMs);
    }
  }

  void degreeMoveAll( float targetPos, bool relative=false, int speed=MTR_MIN) {
    float current_pos;
    for (int i = 0; i < numPaintings; i++) {
      if (relative){
        current_pos = paintings[i]->normalizedEncoder();
        paintings[i]->degreeMove(enforce360(targetPos+current_pos), speed);

      }
      paintings[i]->degreeMove(targetPos, speed);
    }
  }
  void cascadeMoveAll(uint32_t targetPos,  bool relative=false, uint32_t delayMs=500, int speed=MTR_MIN) {
    float current_pos;
    for (int i = 0; i < numPaintings; i++) {
      if (relative){
          current_pos = paintings[i]->normalizedEncoder();
          paintings[i]->delayDegreeMove(enforce360(targetPos+current_pos), delayMs * i, speed);
      }
      else{paintings[i]->delayDegreeMove(targetPos, delayMs * i, speed);}
    }
  }
  void stop(){
    for (int i = 0; i < numPaintings; i++) {
      paintings[i]->setIdle();
    }
  }
  void readEncoder(){
    for (int i = 0; i < numPaintings; i++) {
      Serial.print(i);
      Serial.print(" encoder: ");
      Serial.println(paintings[i]->normalizedEncoder());
    }

  }
  void homeAll(int speed) {
    for (int i = 0; i < numPaintings; i++) {
      paintings[i]->delayHoming(speed);
    }
  }
};
