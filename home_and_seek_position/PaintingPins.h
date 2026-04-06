#pragma once
// #include AS5600.h

const int MTR_MIN = 20;
const int MTR_MAX = 40;
const float TARGET_VEL = 30;
const int CASCADE_DELAY = 500;
const float POSITION_TOLERANCE = .5;
enum class PaintingState {
  IDLE,
  HOMING,           // driving until home sensor triggers
  VELOCITY_MOVE,
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
    // For HOME_DELAY (cascade)
    uint32_t waitStartTime = 0;
    uint32_t waitDuration = 0;

    // ── Rolling-average velocity tracker ─────────────────────────────────────────
    // Add these to the private section of PaintingPins

    static const int VEL_BUF_SIZE = 8;       // samples in rolling window
    float   _velBuf[VEL_BUF_SIZE]  = {};     // encoder readings  (degrees)
    uint32_t _velTime[VEL_BUF_SIZE] = {};    // timestamps        (µs)
    int     _velIdx   = 0;
    bool    _velFull  = false;

    // Position PID state
    float _posIntegral  = 0.0f;
    float _posLastError = 0.0f;
    uint32_t _posLastTime = 0;

    // Velocity PID state
    float _velIntegral  = 0.0f;
    float _velLastError = 0.0f;
    uint32_t _velLastTime = 0;
    float _targetvel = 0;

    static const int DVEL_BUF_SIZE = 8;      // tune this — larger = smoother but more lag
    float    _dvelBuf[DVEL_BUF_SIZE]  = {};
    uint32_t _dvelTime[DVEL_BUF_SIZE] = {};
    int      _dvelIdx  = 0;
    bool     _dvelFull = false;


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
  void setIdle(){
    state = PaintingState::IDLE;
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
          encoderOffset = readEncoder();
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
    targetPos = pos_degrees;
    driveSpeed = speed;
    float error = pos_degrees - normalizedEncoder();
    Serial.println(error);
    if (abs(error)>tolerance){
      velocityPID(speed);
      state = PaintingState::DEGREE_MOVE;
    }
    else{
      Serial.println("move completed");
      drive(0);
      state = PaintingState::IDLE;
}
  }
// ── PID helpers ───────────────────────────────────────────────────────────────
// Add these to the public section of PaintingPins

// --------------------------------------------------------------------------
// positionPID()
//   Drives the motor toward `targetDeg` using PID on encoder position.
//   Call repeatedly from update() / degreeMove(); returns when close enough
//   (caller checks state). Non-blocking — computes one step per call.
//
//   Typical starting gains:  kp=1.5  ki=0.01  kd=0.05
//   iLimit clamps the integral to avoid windup.
// --------------------------------------------------------------------------
void positionPID(float targetDeg,
                 float kp      = 1.5f,
                 float ki      = 0.01f,
                 float kd      = 0.05f,
                 float iLimit  = 30.0f)
{
    uint32_t now = micros();
    float dt = (now - _posLastTime) / 1e6f;   // seconds
    if (_posLastTime == 0 || dt <= 0.0f) {     // first call guard
        _posLastTime = now;
        return;
    }
    _posLastTime = now;

    float pos   = normalizedEncoder();
    float error = targetDeg - pos;

    // Shortest-path correction across the 0/360 wrap
    if (error >  180.0f) error -= 360.0f;
    if (error < -180.0f) error += 360.0f;

    // Integral with windup clamp
    _posIntegral += error * dt;
    _posIntegral  = constrain(_posIntegral, -iLimit, iLimit);

    float derivative = (error - _posLastError) / dt;
    _posLastError    = error;

    float output = kp * error + ki * _posIntegral + kd * derivative;
    output = constrain(output, -MTR_MAX, MTR_MAX);

    // Dead-band: let the caller's tolerance check handle the final stop
    if (abs(output) < MTR_MIN && abs(error) > POSITION_TOLERANCE)
        output = (output >= 0) ? MTR_MIN : -MTR_MIN;

    drive((int)output);
}

void resetVelocityPID() {
    _velIntegral = 0; _velLastError = 0; _velLastTime = 0;
    _velIdx = 0; _velFull = false;
}

// --------------------------------------------------------------------------
// velocityPID()
//   Drives the motor toward `targetDegPerSec` using PID on measured velocity.
//   Velocity is derived from a circular buffer of (time, position) samples,
//   giving a robust rolling-average deg/s estimate.
//   Call repeatedly from update(). Non-blocking.
//
//   Typical starting gains:  kp=0.8  ki=0.05  kd=0.01
// --------------------------------------------------------------------------
void velocityPID(float targetDegPerSec,
                 float kp     = 0.80f,
                 float ki     = 0.02f,
                 float kd     = -0.000f,
                 float iLimit = 50.0f, 
                 float dLimit = 1000.0f,
                 float pLimit = 50.0f, 
                  float kff    = 0.65f)   // loaded
                //  float kp     = 0.6,
                //  float ki     = 0.0f,
                //  float kd     = 0.2,
                //  float iLimit = 50.0f, 
                //  float dLimit = 100.0f,
                //  float pLimit = 30.0f, 
                //   float kff    = .75f)   // unloaded

                //  float kp     = 0.55f,
                //  float ki     = 0.00f,
                //  float kd     = 0.0,
                //  float iLimit = 50.0f, 
                //  float dLimit = 100.0f,
                //   float kff    = .75f)   // drive at set speed

                //  float kp     = .4f,
                //  float ki     = 1.0f,
                //  float kd     = 0.08f,
                //  float iLimit = 50.0f, 

{
    // ── push new sample into circular buffer ─────────────────────────────
    state=PaintingState::VELOCITY_MOVE;
    _targetvel = targetDegPerSec;
    uint32_t now = micros();
    _velBuf [_velIdx] = normalizedEncoder();
    _velTime[_velIdx] = now;
    _velIdx = (_velIdx + 1) % VEL_BUF_SIZE;
    if (_velIdx == 0) _velFull = true;

    // Need at least 2 samples before we can estimate velocity
    int samples = _velFull ? VEL_BUF_SIZE : _velIdx;
    if (samples < 2) return;

    // ── rolling-average velocity over the whole buffer window ────────────
    // oldest sample is _velIdx itself if full, else index 0
    int oldestIdx  = _velFull ? _velIdx : 0;
    int newestIdx  = (_velIdx - 1 + VEL_BUF_SIZE) % VEL_BUF_SIZE;

    float  dPos = _velBuf [newestIdx] - _velBuf [oldestIdx];
    uint32_t dT = _velTime[newestIdx] - _velTime[oldestIdx];

    // Wrap correction (position crossed 0/360 boundary inside the window)
    if (dPos >  180.0f) dPos -= 360.0f;
    if (dPos < -180.0f) dPos += 360.0f;

    float measuredVel = (dT > 0) ? (dPos / (dT / 1e6f)) : 0.0f;  // deg/s

    // ── PID on velocity ──────────────────────────────────────────────────
    uint32_t pidNow = micros();
    float dt = (pidNow - _velLastTime) / 1e6f;
    if (_velLastTime == 0 || dt <= 0.0f) {
        _velLastTime = pidNow;
        return;
    }
    _velLastTime = pidNow;

    float error      = targetDegPerSec - measuredVel;
    error = constrain(error, -pLimit, pLimit);
    _velIntegral    += error * dt;
    _velIntegral     = constrain(_velIntegral, -iLimit, iLimit);
    
    // float derivative = (error - _velLastError) / dt;
    
    // ── push measured velocity into derivative buffer ────────────────────────
    _dvelBuf [_dvelIdx] = measuredVel;
    _dvelTime[_dvelIdx] = pidNow;
    _dvelIdx = (_dvelIdx + 1) % DVEL_BUF_SIZE;
    if (_dvelIdx == 0) _dvelFull = true;

    // ── compute derivative as slope across the full derivative window ────────
    float derivative = 0.0f;
    int dSamples = _dvelFull ? DVEL_BUF_SIZE : _dvelIdx;
    if (dSamples >= 2) {
        int dOldest = _dvelFull ? _dvelIdx : 0;
        int dNewest = (_dvelIdx - 1 + DVEL_BUF_SIZE) % DVEL_BUF_SIZE;

        float  dVel = _dvelBuf [dNewest] - _dvelBuf [dOldest];
        uint32_t dT = _dvelTime[dNewest] - _dvelTime[dOldest];

        derivative = (dT > 0) ? (dVel / (dT / 1e6f)) : 0.0f;  // deg/s²
    }


    derivative = constrain(derivative, -dLimit, dLimit);
    _velLastError    = error;

    float feedforward = kff * targetDegPerSec;
    float output = feedforward + kp * error + ki * _velIntegral + kd * derivative;
    Serial.print("Kp: ");
    Serial.print(kp);
    Serial.print(" * ");
    Serial.println(error);
    Serial.print("Ki: ");
    Serial.print(ki);
    Serial.print(" * ");
    Serial.println(_velIntegral);

    Serial.print("Kd: ");
    Serial.print(kd);
    Serial.print(" * ");
    Serial.println(derivative);

    Serial.print("raw output: ");
    Serial.println(output);
    output = constrain(output, -MTR_MAX, MTR_MAX);
    Serial.print("constrained output: ");
    Serial.println(output);
    Serial.print("Measured Velocity: ");
    Serial.println(measuredVel);
    drive((int)output);
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
        degreeMove(targetPos, driveSpeed, POSITION_TOLERANCE);
        // else: keep driving — already set in startTimedMove()
        break;
      case PaintingState::VELOCITY_MOVE:
        velocityPID(_targetvel);
      case PaintingState::IDLE:
        resetVelocityPID();
      default:
        break;
    }
  }

};