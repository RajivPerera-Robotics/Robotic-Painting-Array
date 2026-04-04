#pragma once
// #include AS5600.h

const int MTR_MIN = 30;
const int MTR_MAX = 50;
const int CASCADE_DELAY = 500;
const float POSITION_TOLERANCE = .5;
enum class PaintingState {
  IDLE,
  HOMING,           // driving until home sensor triggers
  DEGREE_MOVE,       // moving for a fixed duration
  HOME_DELAY,       // cascade delay before starting
  MOVE_DELAY        // cascade delay before a degree move
};


class PaintingPins{
  protected:
    int fwdPin, revPin, encPin, homePin;
    int encoderOffset = 0;

  private:
    int driveSpeed = MTR_MIN;
    float targetPos = 0;
    PaintingState state = PaintingState::IDLE;

    // For TIMED_MOVE
    uint32_t moveStartTime = 0;
    uint32_t moveDuration = 0;
    // For HOME_DELAY (cascade)
    uint32_t waitStartTime = 0;
    uint32_t waitDuration = 0;

  public:
  PaintingPins(int fwd_pin, int rev_pin, int encoder_pin, int home_pin)
  : fwdPin(fwd_pin), revPin(rev_pin), encPin(encoder_pin), homePin(home_pin){}
  
  void begin(){
    pinMode(fwdPin, OUTPUT);
    pinMode(revPin, OUTPUT);
    pinMode(encPin, INPUT);
    pinMode(homePin, INPUT);

  }
  void drive(int speed) {

    // Serial.println(readEncoder());
    if (speed < -256 || speed > 256) {
        Serial.println("INVALID SPEED VALUE!!!");
        return;  // early exit on bad input
    }

    if (speed >= 0) {
        analogWrite(fwdPin, speed);
        analogWrite(revPin, 0);
    } else {
        analogWrite(fwdPin, 0);
        analogWrite(revPin, abs(speed));
    }
}

  virtual float readEncoder(){
    float reading_deg = ((analogRead(encPin)+analogRead(encPin)+analogRead(encPin))/(1024.0*3))*360;
    return reading_deg;
  }

  virtual float normalizedEncoder(){
    float adjustedReading =  readEncoder() - encoderOffset;
    if (adjustedReading < 0){
      adjustedReading = adjustedReading + 360;
    }
    return adjustedReading;
  }

  bool readHome(){
    return analogRead(homePin)<200;
  }

  bool isIdle() {
    return state == PaintingState::IDLE;
  }

  void zero(){
    encoderOffset = readEncoder();
    Serial.print("encoder Offset set to ");
    Serial.println(encoderOffset);
  }


  void delayHoming(int speed, uint32_t delayMs = 0) {
    driveSpeed = speed;
    if (delayMs > 0) {
      waitStartTime = millis();
      waitDuration = delayMs;
      state = PaintingState::HOME_DELAY;
    } else {
      state = PaintingState::HOMING;
    }
  }

  void home(){
      state = PaintingState::HOMING;
      if (readHome()) {
          drive(0);
          state = PaintingState::IDLE;
          Serial.println("Homed.");
        } 
      else {
          drive(driveSpeed);
        }
  }

  void delayDegreeMove(uint32_t targetPos,  uint32_t delayMs, int speed=MTR_MIN, float tolerance=POSITION_TOLERANCE){
    driveSpeed = speed;
    targetPos = targetPos;
    if (delayMs > 0){
      waitStartTime = millis();
      waitDuration = delayMs;
      state = PaintingState::MOVE_DELAY;
    }
    else{
      degreeMove(targetPos, speed, tolerance);
    }
    

  }

  void degreeMove(float pos_degrees, float speed=MTR_MIN, float tolerance=POSITION_TOLERANCE){
    // state will be PaintingState::TimedMove
    state = PaintingState::DEGREE_MOVE;
    float error = pos_degrees - normalizedEncoder();
    if (abs(error)>tolerance){
      drive(speed * (error / abs(error)));
    }
    else{
      drive(0);
      state = PaintingState::IDLE;
}
  }

  void update() {
    switch (state) {

      case PaintingState::HOME_DELAY:
        if (millis() - waitStartTime >= waitDuration) {
          state = PaintingState::HOMING;
        }
        break;

      case PaintingState::MOVE_DELAY:
        if (millis()- waitStartTime >= waitDuration){
          degreeMove(targetPos, MTR_MIN, POSITION_TOLERANCE);
        }
        break;
      case PaintingState::HOMING:
        home();
        break;

      case PaintingState::DEGREE_MOVE:
        degreeMove(targetPos, MTR_MIN, POSITION_TOLERANCE);
        // else: keep driving — already set in startTimedMove()
        break;

      case PaintingState::IDLE:
      default:
        break;
    }
  }

};