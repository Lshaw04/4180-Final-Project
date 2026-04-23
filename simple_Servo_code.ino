#include <ESP32Servo.h>

// Create a servo object
Servo myServo;

// Define the pin
const int servoPin = 3;

// Servo properties
int minUs = 500;    // Standard min pulse width (0 degrees)
int maxUs = 2400;   // Standard max pulse width (180 degrees)

void setup() {
  Serial.begin(115200);

  // Allow allocation of all timers
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  // Set the frequency for the servo (standard is 50Hz)
  myServo.setPeriodHertz(50);

  // Attach the servo to pin 3
  if (myServo.attach(servoPin, minUs, maxUs)) {
    Serial.println("Servo attached successfully to Pin 3");
  } else {
    Serial.println("Servo attachment failed!");
  }
}

void loop() {
  // Move to 0 degrees
  Serial.println("Moving to 0 degrees");
  myServo.write(0);
  delay(1000);

  // Move to 90 degrees (Center)
  Serial.println("Moving to 90 degrees");
  myServo.write(90);
  delay(1000);

  // Move to 180 degrees
  Serial.println("Moving to 180 degrees");
  myServo.write(180);
  delay(1000);

  // Smooth sweep
  Serial.println("Starting sweep...");
  for (int pos = 0; pos <= 180; pos += 1) {
    myServo.write(pos);
    delay(15);
  }
  for (int pos = 180; pos >= 0; pos -= 1) {
    myServo.write(pos);
    delay(15);
  }
}