#include <esp_now.h>
#include <WiFi.h>
#include <math.h>

// --- MOTOR PINS ---
const int ENA = 7;  const int IN1 = 5;  const int IN2 = 6;
const int ENB = 18; const int IN3 = 8;  const int IN4 = 9;

// --- SLOW-MO TUNING ---
const int SLOW_POWER = 135; 
const int MOVE_MS = 35;   
const int WAIT_MS = 200;  

struct Point {
  int16_t x; int16_t y; bool penDown; 
};

struct PathPacket {
  int packetId; int count; Point points[50];  
};

Point receivedPath[200];
int pathTotalPoints = 0;
bool startDriving = false;
float currentAngle = 0; // The car starts facing "Forward" (0 degrees)
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
  Serial.printf("Received chunk %d. Points: %d\n", packet->packetId, pathTotalPoints);
}

void setup() {
  Serial.begin(115200);
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  ledcAttach(ENA, 1000, 8); ledcAttach(ENB, 1000, 8);

  WiFi.mode(WIFI_STA);
  esp_now_init();
  esp_now_register_recv_cb(OnDataRecv);
}

// --- DRIVING HELPERS ---

void pulseStop() {
  ledcWrite(ENA, 0); ledcWrite(ENB, 0);
  delay(WAIT_MS);
}

void setDirection(int leftDir, int rightDir) {
  // Uses your fixed directions from the previous step
  digitalWrite(IN1, leftDir > 0 ? HIGH : LOW);
  digitalWrite(IN2, leftDir > 0 ? LOW : HIGH);
  digitalWrite(IN3, rightDir > 0 ? HIGH : LOW);
  digitalWrite(IN4, rightDir > 0 ? LOW : HIGH);
}
void loop() {
  if (pathTotalPoints > 0 && (millis() - lastPacketTime > 500)) {
    startDriving = true;
  }

  if (startDriving) {
    currentAngle = 0; // Reset for the new drawing

    for (int i = 0; i < pathTotalPoints - 1; i++) {
      // FLIPPED DX: Added the minus sign to fix Left/Right being swapped
      float dx = -(receivedPath[i+1].x - receivedPath[i].x);
      float dy = receivedPath[i+1].y - receivedPath[i].y;

      float distance = sqrt(dx*dx + dy*dy);
      if (distance < 1) continue; 

      // 1. TURN TO TARGET
      float targetAngle = atan2(dx, -dy) * 180 / M_PI; 
      float turnNeeded = targetAngle - currentAngle;
      
      while (turnNeeded > 180) turnNeeded -= 360;
      while (turnNeeded < -180) turnNeeded += 360;

      if (abs(turnNeeded) > 5) {
        if (turnNeeded > 0) setDirection(1, -1); 
        else setDirection(-1, 1);
        
        // CALIBRATED: Changed 8 to 6.5 to make turns LARGER
        // If it still doesn't turn enough, make 6.5 even SMALLER (e.g., 5.5)
        int turnPulses = abs(turnNeeded) / 7; 
        for (int p = 0; p < turnPulses; p++) {
          ledcWrite(ENA, SLOW_POWER); ledcWrite(ENB, SLOW_POWER);
          delay(MOVE_MS); pulseStop();
        }
      }

      // 2. DRIVE FORWARD
      setDirection(1, 1);
      int drivePulses = distance / 2; 
      for (int p = 0; p < drivePulses; p++) {
        ledcWrite(ENA, SLOW_POWER); ledcWrite(ENB, SLOW_POWER);
        delay(MOVE_MS); pulseStop();
      }

      currentAngle = targetAngle; 
    }

    pathTotalPoints = 0;
    startDriving = false;
    Serial.println("Path Complete. Calibrated.");
  }
}