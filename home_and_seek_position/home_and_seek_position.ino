#include "PaintingPins.h"
#include "PaintingPinsI2C.h"
#include "PaintingManager.h"
#include "Wire.h"
#include "TCA9548.h"

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

void readInputPainting(PaintingPins* painting){
    if (Serial.available() > 0) {
      String input = Serial.readStringUntil('\n');
      input.trim();
      if (input == "z") painting->zero();
      else if (input == "home") painting->home();
      else if (input == "e"){
        Serial.print("Encoder value: ");
        Serial.println(painting->normalizedEncoder());
      }
      else if (input == "h"){
        Serial.print("Hall value: ");
        Serial.println(painting->readHome());
      }
      else if (input == "df") painting->drive(MTR_MIN);// drive forward
      else if (input == "dr") painting->drive(-MTR_MIN); // drive reverse
      else if (input == "s") painting->drive(0); // stop
      else if (isValidNumber(input)) {
        Serial.print("moving to ");
        Serial.print(input);
        Serial.println(" degrees");
        painting->moveTo(input.toInt(), 20, 1);
      } 
      else Serial.println("Invalid input: please enter a whole number between -256 and 256.");
    }
}

void readInputManager(PaintingManager* manager, Stream& serial){
    manager->update();
    if (serial.available() > 0) {
      String input = serial.readStringUntil('\n');
      input.trim();
    if (input == "cascade home") manager->cascadeHome(MTR_MIN);
    else if (input == "cascade 90 rel") manager->cascadeMoveAll(90, true, CASCADE_DELAY);
    else if (input == "cascade 90 abs") manager->cascadeMoveAll(90, false, CASCADE_DELAY);
    else if (input == "cascade 45 rel") manager->cascadeMoveAll(45, true, CASCADE_DELAY);
    else if (input == "cascade 45 abs") manager->cascadeMoveAll(45, false, CASCADE_DELAY);
    else if (input == "cascade rev") manager->cascadeMoveAll(45, 1000, CASCADE_DELAY);
    else if (input == "homeall") manager->homeAll(MTR_MIN);
    else if (input == "fwd 90 rel") manager->degreeMoveAll(90, true);
    else if (input == "fwd 90 abs") manager->degreeMoveAll(90, false);
    else if (input == "fwd 45 rel") manager->degreeMoveAll(45, true);
    else if (input == "fwd 45 abs") manager->degreeMoveAll(45, false);
    else if (input == "rev 90 rel") manager->degreeMoveAll(90, true, -MTR_MIN);
    else if (input == "rev 90 abs") manager->degreeMoveAll(90, false, -MTR_MIN);
    else if (input == "rev 45 rel") manager->degreeMoveAll(45, true, -MTR_MIN);
    else if (input == "rev 45 abs") manager->degreeMoveAll(45, false, -MTR_MIN);
    }
}


const int encoder_pin = 23;
const int fwd_pin = 0;
const int rev_pin = 1;
const int hall_pin = 20;
const int boundary = 256;


  // PaintingPins(int fwd_pin, int rev_pin, int encoder_pin, int home_pin)

const int numPaintings = 3;
// how to define paintings
// PaintingPins p1(1,2,3,4) // example
// PaintingPins* allPaintings[numPaintings] = { &p1, &p2, &p3};
// PaintingManager manager(allPaintings, numPaintings);

  // PaintingPinsI2C(int fwd_pin, int rev_pin, int enc_pin, int home_pin,
  //               int sda_pin, int scl_pin, int dir_pin = 4)

  // PaintingPinsI2C(int fwd_pin, int rev_pin, int enc_pin, int home_pin,
  //                 int sda_pin, int scl_pin, PCA9546& multi_plexer, int mult_channel)


PCA9546 MP(0x70, &Wire2);
PCA9546 MP2(0x71, &Wire2)

PaintingPinsI2c p1(1, 0, 99, 19, 25, 24, MP, 0);
PaintingPinsI2c p2(3, 2, 99, 18, 25, 24, MP, 1);
PaintingPinsI2c p3(5, 4, 99, 17, 25, 24, MP, 2);

int numPaintings = 3;
PaintingPins* allPaintings[numPaintings] = {&p1, &p2, &p3};
PaintingManager manager(allPaintings, numPaintings);

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);
  // manager.begin();
  paintingi2c.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  static uint32_t lastTime = 0;
  if (millis() - lastTime >= 100)
  {
    lastTime = millis();
    readInputManager(&manager, &Serial1);
  }


  }

