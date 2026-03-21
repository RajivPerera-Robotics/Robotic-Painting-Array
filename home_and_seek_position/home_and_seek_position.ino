#include "AS5600.h"
#include "Wire.h"
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
    Serial.println("Homeing completed");
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

class PaintingManager {
private:
  PaintingPins** paintings;
  int numPaintings;

public:
  PaintingManager(PaintingPins* p[], int count) : numPaintings(count) {
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

  // Behavior 1: cascade home — each painting waits 500ms * index before starting
  void cascadeHome(int speed, uint32_t stepDelayMs = 500) {
    for (int i = 0; i < numPaintings; i++) {
      paintings[i]->startHoming(speed, i * stepDelayMs);
    }
  }

  // Behavior 2: move all simultaneously for a fixed duration
  // speed > 0 = forward, speed < 0 = reverse
  // durationMs controls how long they run (determines effective distance)
  void timedMoveAll(int speed, uint32_t durationMs) {
    for (int i = 0; i < numPaintings; i++) {
      paintings[i]->startTimedMove(speed, durationMs);
    }
  }

  // Behavior 3: home all simultaneously
  void homeAll(int speed) {
    for (int i = 0; i < numPaintings; i++) {
      paintings[i]->startHoming(speed);
    }
  }
};



bool isValidNumber(String str) {
  if (str.length() == 0) return false;
  int start = 0;
  if (str.charAt(0) == '-') {
    if (str.length() == 1) return false;
    start = 1;
  }
  for (int i = start; i < str.length(); i++) {
    if (!isDigit(str.charAt(i))) return false;
  }
  int value = str.toInt();
  int boundary = 360;
  if (value < -1*boundary || value > boundary) {
    Serial.print("Out of range: ");
    Serial.print(value);
    Serial.print(" is not between");
    Serial.print(" -"); Serial.print(boundary); Serial.print(" and -");
    Serial.println(boundary); 
    return false;
  }
  return true;
}

void readInputPainting(PaintingPins* painting1){
    if (Serial.available() > 0) {
      String input = Serial.readStringUntil('\n');
      input.trim();
      if (input == "zero"){
        painting1->zero();
        return;
      }
      if (input == "home"){
        painting1->home();
        return;
      }

      if (input == "e"){
        Serial.print("Encoder value: ");
        Serial.println(painting1->readEncoder());
        return;
      }
      if (input == "h"){
        Serial.print("Hall value: ");
        Serial.println(painting1->readHome());
        return;
      }
      if (input == "df"){ // drive forward
        painting1->drive(20);
        return;
      }
      if (input == "dr"){ // drive reverse
        painting1->drive(-20);
        return;
      }
      if (input == "stop"){
        painting1->drive(0);
        return;
      }
      if (isValidNumber(input)) {
        Serial.print("Driving motor at ");
        Serial.println(input);
        painting1->moveTo(input.toInt(), 20, 1);
        return;
      } 
      else {
        Serial.println("Invalid input: please enter a whole number between -256 and 256.");
      }
    }
}

void readInputManager(PaintingManager* manager){
    manager->update();
    if (Serial.available() > 0) {
      String input = Serial.readStringUntil('\n');
      input.trim();
    if (input == "cascade") manager->cascadeHome(MTR_MIN);
    else if (input == "homeall") manager->homeAll(MTR_MIN);
    else if (input == "fwd") manager->timedMoveAll(MTR_MIN, 2000);
    else if (input == "rev") manager->timedMoveAll(-MTR_MIN, 2000);
    }
}

const int encoder_pin = 23;
const int fwd_pin = 0;
const int rev_pin = 1;
const int hall_pin = 20;
const int boundary = 256;
// painting (fwd_pin, rev_pin, encoder_pin, hall_pin)
PaintingPins p1(0, 1, 23, 20);

PaintingPins* allPaintings[1] = { &p1};
PaintingManager manager(allPaintings, 1);


void setup() {
  Serial.begin(115200);
  manager.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  static uint32_t lastTime = 0;
  if (millis() - lastTime >= 100)
  {
    lastTime = millis();
    readInputManager(&manager);
    // Serial.print("Encoder value; ");
    // Serial.println(painting1.readEncoder());
  }


  }

