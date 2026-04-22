// Pin Definitions for L298N
const int ENA = 7;  // PWM pin for Motor A
const int IN1 = 5;  // Direction Pin 1
const int IN2 = 6;  // Direction Pin 2

const int ENB = 18; // PWM pin for Motor B (Example pin)
const int IN3 = 8;  // Direction Pin 3
const int IN4 = 9;  // Direction Pin 4

// PWM Properties
uint32_t PWM_FREQ = 20000;   // 20 kHz
uint8_t  PWM_RES  = 8;       // 8-bit (0–255)

void setup() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Initialize PWM for both Enable pins
  ledcAttach(ENA, PWM_FREQ, PWM_RES);
  ledcAttach(ENB, PWM_FREQ, PWM_RES);

  Serial.begin(115200);
  Serial.println("L298N Motor Control Initiated");
}

void loop() {
  // ===== FORWARD =====
  Serial.println("Moving Forward...");
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  
  ledcWrite(ENA, 200); // Speed 0-255
  ledcWrite(ENB, 200);
  delay(3000);

  // ===== STOP =====
  Serial.println("Stopped");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  
  ledcWrite(ENA, 0);
  ledcWrite(ENB, 0);
  delay(1000);

  // ===== REVERSE =====
  Serial.println("Moving Reverse...");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  
  ledcWrite(ENA, 200);
  ledcWrite(ENB, 200);
  delay(3000);
}