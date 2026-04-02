#pragma once
#include "PaintingPins.h"
#include "TCA9548.h"

class PaintingManager {
private:
  PaintingPins** paintings;
  int numPaintings;

public:
  PaintingManager(PaintingPins* p[], int count)
    : numPaintings(count) {
    paintings = new PaintingPins*[count];
    for (int i = 0; i < count; i++) paintings[i] = p[i];
  }

  void begin() {
    for (int i = 0; i < numPaintings; i++) paintings[i]->begin();
  }

  // Call every loop — this is the heartbeat
  void update() {
    for (int i = 0; i < numPaintings; i++) paintings[i]->update();
  }

  bool allIdle() {
    for (int i = 0; i < numPaintings; i++) {
      if (!paintings[i]->isIdle()) return false;
    }
    return true;
  }

  // cascade home — each painting waits 500ms * index before starting
  void cascadeHome(int speed, uint32_t stepDelayMs = 500) {
    for (int i = 0; i < numPaintings; i++) {
      paintings[i]->delayHoming(speed, i * stepDelayMs);
    }
  }

  // move all simultaneously for a fixed duration
  // speed > 0 = forward, speed < 0 = reverse
  // durationMs controls how long they run (determines effective distance)
  void timedMoveAll(int speed, uint32_t durationMs) {
    for (int i = 0; i < numPaintings; i++) {
      paintings[i]->startTimedMove(speed, durationMs);
    }
  }
  void cascadeMoveAll(int speed, uint32_t durationMs, uint32_t delayMs) {
    for (int i = 0; i < numPaintings; i++) {
      paintings[i]->delayTimedMove(speed, durationMs, delayMs * i);
      Serial.println(delayMs * i);
    }
  }
  // home all simultaneously
  void homeAll(int speed) {
    for (int i = 0; i < numPaintings; i++) {
      paintings[i]->delayHoming(speed);
    }
  }
};
