#include <esp_now.h>
#include <WiFi.h>
#include <math.h>
const int ENA = 7;  const int IN1 = 5;  const int IN2 = 6;
const int ENB = 18; const int IN3 = 8;  const int IN4 = 9;
const int LEFT_ENC = 11; const int RIGHT_ENC = 10;
volatile unsigned long leftPulses = 0;
volatile unsigned long rightPulses = 0;
void IRAM_ATTR handleLeft() { leftPulses++; }
void IRAM_ATTR handleRight() { rightPulses++; }
const int BASE_POWER = 155; 
const int MOVE_MS = 40;    
const int WAIT_MS = 40;     
const float Kp = 4.0;       
const int ERR_THRESHOLD = 5; 

struct Point {
  int16_t x; int16_t y; bool penDown; 
};
struct PathPacket {
  int packetId; int count; Point points[50];  
};
Point receivedPath[200];
int pathTotalPoints = 0;
bool startDriving = false;
float currentAngle = 0; 
unsigned long lastPacketTime = 0;
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  PathPacket *packet = (PathPacket*)data;
  for (int i = 0; i < packet->count; i++) {
    if (pathTotalPoints < 200) {
      receivedPath[pathTotalPoints] = packet->points[i];
      pathTotalPoints++;
    }
  }
  lastPacketTime = millis();
}
void setDirection(int leftDir, int rightDir) {
  digitalWrite(IN1, leftDir > 0 ? HIGH : LOW);
  digitalWrite(IN2, leftDir > 0 ? LOW : HIGH);
  digitalWrite(IN3, rightDir > 0 ? HIGH : LOW);
  digitalWrite(IN4, rightDir > 0 ? LOW : HIGH);
}
void driveWithEncoders(int leftDir, int rightDir, unsigned long target) {
  leftPulses = 0; 
  rightPulses = 0;
  setDirection(leftDir, rightDir);
  while (leftPulses < target || rightPulses < target) {
    int leftPWM = BASE_POWER;
    int rightPWM = BASE_POWER;
    if (leftDir == rightDir) {
      long error = (long)leftPulses - (long)rightPulses;
      if (abs(error) >= ERR_THRESHOLD) {
        int adj = (int)(error * Kp);
        leftPWM  = BASE_POWER - adj;
        rightPWM = BASE_POWER + adj;
      }
    }
    ledcWrite(ENA, constrain(leftPWM, 130, 210));
    ledcWrite(ENB, constrain(rightPWM, 130, 210));
    delay(MOVE_MS);
    ledcWrite(ENA, 0); ledcWrite(ENB, 0);
    delay(WAIT_MS);
    yield(); 
  }
}
void setup() {
  Serial.begin(115200);
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  ledcAttach(ENA, 1000, 8); ledcAttach(ENB, 1000, 8);
  pinMode(LEFT_ENC, INPUT_PULLUP);
  pinMode(RIGHT_ENC, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(LEFT_ENC), handleLeft, RISING);
  attachInterrupt(digitalPinToInterrupt(RIGHT_ENC), handleRight, RISING);
  WiFi.mode(WIFI_STA);
  esp_now_init();
  esp_now_register_recv_cb(OnDataRecv);
}
void loop() {
  if (pathTotalPoints > 0 && (millis() - lastPacketTime > 500)) {
    startDriving = true;
  }
  if (startDriving) {
    currentAngle = 0; 
    for (int i = 0; i < pathTotalPoints - 1; i++) {
      float dx = -(receivedPath[i+1].x - receivedPath[i].x);
      float dy = receivedPath[i+1].y - receivedPath[i].y;
      float distance = sqrt(dx*dx + dy*dy);
      if (distance < 1) continue; 
      float targetAngle = atan2(dx, -dy) * 180 / M_PI; 
      float turnNeeded = targetAngle - currentAngle;
      while (turnNeeded > 180) turnNeeded -= 360;
      while (turnNeeded < -180) turnNeeded += 360;
      if (abs(turnNeeded) > 5) {
        int leftDir = (turnNeeded > 0) ? 1 : -1;
        int rightDir = (turnNeeded > 0) ? -1 : 1;
        // Calibration: How many encoder pulses per degree of turn?
        // Adjust the 7.0 divider based on your floor friction
        unsigned long turnPulses = abs(turnNeeded) / 7.0; 
        driveWithEncoders(leftDir, rightDir, turnPulses);
      }
      unsigned long driveTarget = distance / 1.5; 
      driveWithEncoders(1, 1, driveTarget);
      currentAngle = targetAngle; 
    }
    pathTotalPoints = 0;
    startDriving = false;
    Serial.println("Path Complete.");
  }
}
