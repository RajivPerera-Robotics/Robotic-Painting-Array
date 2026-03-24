#include "PaintingPins.h"
#include "PaintingManager.h"
#include "Wire.h"

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
        Serial.println(painting1->normalizedEncoder());
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
      if (input == "s"){ // stop
        painting1->drive(0);
        return;
      }
      if (isValidNumber(input)) {
        Serial.print("moving to ");
        Serial.print(input);
        Serial.println(" degrees");
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
    if (input == "cascade home") manager->cascadeHome(MTR_MIN);
    else if (input == "cascade fwd") manager->cascadeMoveAll(MTR_MIN, 2000, 2000);
    else if (input == "cascade rev") manager->cascadeMoveAll(-MTR_MIN, 2000, 2000);
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

// one painting on the breadboard
// PaintingPins p1(0, 1, 99, 20);

// painting (fwd_pin, rev_pin, encoder_pin, hall_pin)1
PaintingPins p1(0,1,24,21);
PaintingPins p2(2,3,99,20);
PaintingPins p3(4,5,99,19);



const int numPaintings = 3;
PaintingPins* allPaintings[numPaintings] = { &p1, &p2, &p3};
PaintingManager manager(allPaintings, numPaintings);


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
    // readInputPainting(&p1);
  }


  }

