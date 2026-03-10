#include "AS5600.h"
#include "Wire.h"
const int MTR_MIN = 20;
const int MTR_MAX = 50;

class PaintingPins{
  private:
    int fwdPin;
    int revPin;
    int encPin; 
    int homePin;
    int pinFreq=0;
    int encoderOffset = 0;
    float P = .5;
    int D = -2;
  public:
  PaintingPins(int fwd_pin, int rev_pin, int encoder_pin, int home_pin): fwdPin(fwd_pin), revPin(rev_pin), encPin(encoder_pin), homePin(home_pin){}
  PaintingPins(int fwd_pin, int rev_pin, int encoder_pin, int home_pin, int pin_freq): fwdPin(fwd_pin), revPin(rev_pin), encPin(encoder_pin), homePin(home_pin), pinFreq(pin_freq){}
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
  float readHall(){
    return analogRead(homePin);
  }
  void zero(){
    encoderOffset = avgReading();
    Serial.print("encoder Offset set to ");
    Serial.println(encoderOffset);
  }
  void home(){
    Serial.print("hall effect sensor value");
    Serial.println(analogRead(homePin));
    while (analogRead(homePin)>100){
      Serial.println(analogRead(homePin));
      drive(MTR_MIN);
    }
    drive(0);
    zero();
  }
  // void moveTo(int pos_degrees, int tolerance){
  //   float error = pos_degrees - readEncoder();
  //   float last_error = error;
  //   float PID = 0.0;
  //   while (abs(error)>tolerance){
  //     float d_term = error - last_error;
  //     Serial.print("P: "); Serial.print(P); 
  //     Serial.print(" E:"); Serial.print(error);
  //     Serial.print(" D: "); Serial.print(D);
  //     Serial.println(d_term);
  //     PID = P*error + D*(d_term);
  //     if (abs(PID) < MTR_MIN){
  //       PID = MTR_MIN * (PID / abs(PID));
  //     }
  //     PID = (PID > MTR_MAX)*MTR_MAX + (PID < -MTR_MAX)*(-MTR_MAX) + (PID > -MTR_MAX && PID < MTR_MAX)*PID;
  //     drive(PID);
  //     last_error = error;
  //     error = pos_degrees - readEncoder();
  //   }
  //   drive(0);
  // }

  void moveTo(int pos_degrees, int speed, int tolerance){
    float error = pos_degrees - readEncoder();
    while (abs(error)>tolerance){
      drive(speed * (error / abs(error)));
      error = pos_degrees - readEncoder();
    }
    drive(0);
  }
};

const int encoder_pin = 23;
const int fwd_pin = 0;
const int rev_pin = 1;
const int hall_pin = 20;
const int boundary = 256;
PaintingPins painting1(fwd_pin, rev_pin, encoder_pin, hall_pin);

void setup() {
  Serial.begin(115200);
  painting1.begin();
  // put your setup code here, to run once:
  pinMode(fwd_pin, OUTPUT);
  pinMode(rev_pin, OUTPUT);
}

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
  if (value < -360 || value > 360) {
    Serial.print("Out of range: ");
    Serial.print(value);
    Serial.print(" is not between");
    Serial.print(" -"); Serial.print(boundary); Serial.print(" and -");
    Serial.println(boundary); 
    return false;
  }

  return true;
}
void readInput(){
    if (Serial.available() > 0) {
      String input = Serial.readStringUntil('\n');
      input.trim();
      if (input == "zero"){
        painting1.zero();
        return;
      }
      if (input == "home"){
        painting1.home();
        return;
      }

      if (input == "e"){
        Serial.print("Encoder value: ");
        Serial.println(painting1.readEncoder());
        return;
      }
      if (input == "h"){
        Serial.print("Hall value: ");
        Serial.println(painting1.readHall());
        return;
      }
      if (input == "df"){ // drive forward
        painting1.drive(20);
      }
      if (input == "dr"){ // drive reverse
        painting1.drive(-20);
      }
      if (input == "stop"){
        painting1.drive(0);
      }
      if (isValidNumber(input)) {
        Serial.print("Driving motor at ");
        Serial.println(input);
        painting1.moveTo(input.toInt(), 20, 1);
      } 
      else {
        Serial.println("Invalid input: please enter a whole number between -256 and 256.");
      }
    }
}

void loop() {
  // put your main code here, to run repeatedly:
  static uint32_t lastTime = 0;
  if (millis() - lastTime >= 100)
  {
    lastTime = millis();
    readInput();
    // Serial.print("Encoder value; ");
    // Serial.println(painting1.readEncoder());
  }


  }

