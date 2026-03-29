// =============================================
// ESP32 Sine Wave Generator
// Adjustable period and amplitude
// =============================================

// --- Parameters (adjust these) ---
float amplitude = 35.0;   // Peak value (e.g., motor units, PWM, etc.)
float period    = 20*1000.0;  // Period in milliseconds (time for one full cycle)

// pins
const int fwd_pin = 15;
const int rev_pin = 14;

// --- Internal state ---
unsigned long startTime;

// =============================================
// Compute sine wave value at a given time
//
// Returns a float in the range [-amplitude, +amplitude]
// t_ms     : current time in milliseconds
// amplitude: peak value of the wave
// period   : duration of one full cycle in ms
// =============================================
float sineWave(unsigned long t_ms, float amplitude, float period) {
  float t = (float)t_ms;
  float radians = (2.0 * PI * t) / period;
  return amplitude * sin(radians);
}

// =============================================
// Print the current sine value to Serial
// in a plotter-friendly format
// =============================================
void printSineValue(float value) {
  // Uncomment the label line below if using Serial Monitor instead of Plotter
  // Serial.print("Sine:");
  Serial.println(value);
}

void drive(int motor_pwm){
  if (motor_pwm > 0){
    analogWrite(fwd_pin, motor_pwm);
    analogWrite(rev_pin, 0);
    return;
  }
  if (motor_pwm < 0){
    analogWrite(rev_pin, abs(motor_pwm));
    analogWrite(fwd_pin, 0);
    return;
  }
}


// =============================================
// Setup
// =============================================
void setup() {
  Serial.begin(115200);
  startTime = millis();
  pinMode(fwd_pin, OUTPUT);
  pinMode(rev_pin, OUTPUT);

}


// =============================================
// Loop
// =============================================
void loop() {
  unsigned long elapsed = millis() - startTime;

  float output = sineWave(elapsed, amplitude, period);
  printSineValue(output);
  drive((int)output);

  delay(250); // ~50 samples/sec — adjust for smoother/faster plots
}