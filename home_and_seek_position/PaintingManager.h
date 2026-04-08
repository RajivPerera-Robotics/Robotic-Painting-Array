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

  void cascadeHome(uint32_t stepDelayMs = 500, int speed=MTR_MIN, bool random=false) {
    int order[numPaintings];
    for (int i = 0; i < numPaintings; i++) order[i] = i;
    if (random) {
      for (int i = numPaintings - 1; i > 0; i--) {
        int j = ::random(i + 1);
        int tmp = order[i]; order[i] = order[j]; order[j] = tmp;
      }
    }
    for (int i = 0; i < numPaintings; i++) {
      paintings[order[i]]->delayHoming(speed, i * stepDelayMs);
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
  void cascadeMoveAll(int speed, uint32_t durationMs, uint32_t delayMs, bool random=false){
    int order[numPaintings];
    for (int i = 0; i < numPaintings; i++) order[i] = i;
    if (random) {
      for (int i = numPaintings - 1; i > 0; i--) {
        int j = ::random(i + 1);
        int tmp = order[i]; order[i] = order[j]; order[j] = tmp;
      }
    }
    for (int i = 0; i < numPaintings; i++){
      paintings[order[i]]->delayTimedMove(speed, durationMs, delayMs*i);      
      Serial.println(delayMs*i);
    }
  }

  void cascadeTimedMoveThenHome(int speed, uint32_t durationMs, uint32_t delayMs, bool random=false) {
    int order[numPaintings];
    for (int i = 0; i < numPaintings; i++) order[i] = i;
    if (random) {
      for (int i = numPaintings - 1; i > 0; i--) {
        int j = ::random(i + 1);
        int tmp = order[i]; order[i] = order[j]; order[j] = tmp;
      }
    }
    for (int i = 0; i < numPaintings; i++) {
      paintings[order[i]]->delayTimedMoveThenHome(speed, durationMs, delayMs * i);
    }
  }

  void timedMoveAll(int speed, uint32_t durationMs) {
    for (int i = 0; i < numPaintings; i++) {
      paintings[i]->startTimedMove(speed, durationMs);
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
  void readHome(){
    for (int i = 0; i < numPaintings; i++) {
        Serial.print(i);
        Serial.print(" : ");
        Serial.println(paintings[i]->readHome());
    }

  }
  void homeAll(int speed) {
    for (int i = 0; i < numPaintings; i++) {
      paintings[i]->delayHoming(speed);
    }
  }
};
