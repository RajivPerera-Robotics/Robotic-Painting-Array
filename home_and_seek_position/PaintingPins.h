#pragma once
// #include AS5600.h

const int MTR_MIN = 20;
const int MTR_MAX = 50;

enum class PaintingState {
  IDLE,
  HOMING,           // driving until home sensor triggers
  TIMED_MOVE,       // moving for a fixed duration
  WAITING           // cascade delay before starting
};


class PaintingPins{
  private:
    int fwdPin, revPin, encPin ,homePin;
    int pinFreq=0;
    int encoderOffset = 0;
    int driveSpeed = 0;
    PaintingState state = PaintingState::IDLE;

    // For TIMED_MOVE
    uint32_t moveStartTime = 0;
    uint32_t moveDuration = 0;
    // For WAITING (cascade)
    uint32_t waitStartTime = 0;
    uint32_t waitDuration = 0;

  public:
  PaintingPins(int fwd_pin, int rev_pin, int encoder_pin, int home_pin)
  : fwdPin(fwd_pin), revPin(rev_pin), encPin(encoder_pin), homePin(home_pin){}
  PaintingPins(int fwd_pin, int rev_pin, int encoder_pin, int home_pin, int pin_freq)
  : fwdPin(fwd_pin), revPin(rev_pin), encPin(encoder_pin), homePin(home_pin), pinFreq(pin_freq){}
  
  void begin(){
    pinMode(fwdPin, OUTPUT);
    pinMode(revPin, OUTPUT);
    pinMode(encPin, INPUT);
    pinMode(homePin, INPUT);
    if (pinFreq!=0){
      analogWriteFrequency(fwdPin, pinFreq);
      analogWriteFrequency(revPin, pinFreq);
    }

  }
  void drive(int speed) {
    Serial.print("Driving to speed ");
    Serial.println(speed);
    Serial.print("Encoder: ");

    Serial.println(readEncoder());
    if (speed < -256 || speed > 256) {
        Serial.println("INVALID SPEED VALUE!!!");
        return;  // early exit on bad input
    }

    if (speed >= 0) {
        // Serial.println("Driving forward");
        analogWrite(fwdPin, speed);
        analogWrite(revPin, 0);
    } else {
        // Serial.println("Driving in reverse");
        // Serial.println(speed);
        // Serial.println(abs(speed));
        analogWrite(fwdPin, 0);
        analogWrite(revPin, abs(speed));
    }
}

  float avgReading(){
    float reading_deg = (analogRead(encPin) + analogRead(encPin) + analogRead(encPin))/(3*1024.0)*360.0;
    return reading_deg;
  }

  float readEncoder(){
    float adjustedReading = avgReading() - encoderOffset;
    if (adjustedReading < 0){
      adjustedReading = adjustedReading + 360;
    }
    return adjustedReading;
  }

  bool readHome(){
    return analogRead(homePin)<100;
  }

    bool isIdle() {
    return state == PaintingState::IDLE;
  }

  void zero(){
    encoderOffset = avgReading();
    Serial.print("encoder Offset set to ");
    Serial.println(encoderOffset);
  }
  void home(){
    Serial.print("hall effect sensor value");
    Serial.println(analogRead(homePin));
    while (!readHome()){
      drive(MTR_MIN);
    }
    Serial.println("Homing completed");
    drive(0);
    zero();
  }

  void startHoming(int speed, uint32_t delayMs = 0) {
    driveSpeed = speed;
    if (delayMs > 0) {
      waitStartTime = millis();
      waitDuration = delayMs;
      state = PaintingState::WAITING;
    } else {
      state = PaintingState::HOMING;
    }
  }

  void startTimedMove(int speed, uint32_t durationMs) {
    driveSpeed = speed;
    moveStartTime = millis();
    moveDuration = durationMs;
    state = PaintingState::TIMED_MOVE;
    drive(driveSpeed);
  }

  void moveTo(int pos_degrees, int speed, int tolerance){
    float error = pos_degrees - readEncoder();
    while (abs(error)>tolerance){
      drive(speed * (error / abs(error)));
      error = pos_degrees - readEncoder();
    }
    drive(0);
  }

  void update() {
    switch (state) {

      case PaintingState::WAITING:
        if (millis() - waitStartTime >= waitDuration) {
          // Delay elapsed — start the actual homing
          state = PaintingState::HOMING;
        }
        break;

      case PaintingState::HOMING:
        if (readHome()) {
          drive(0);
          state = PaintingState::IDLE;
          Serial.println("Homed.");
        } else {
          drive(driveSpeed);
        }
        break;

      case PaintingState::TIMED_MOVE:
        if (millis() - moveStartTime >= moveDuration) {
          drive(0);
          state = PaintingState::IDLE;
        }
        // else: keep driving — already set in startTimedMove()
        break;

      case PaintingState::IDLE:
      default:
        break;
    }
  }

};