#include "Goldelox_Serial_4DLib.h"
#include "Goldelox_Const4D.h"
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h> // Required for channel locking
#define DisplaySerial Serial1 
Goldelox_Serial_4DLib Display(&DisplaySerial);

#define RESET_PIN  1
#define TX_PIN     18
#define RX_PIN     19

#define UP_PIN     20
#define CENTER_PIN 21
#define LEFT_PIN   22
#define DOWN_PIN   23
#define RIGHT_PIN  15

uint8_t carAddress[] = {0xA0, 0x85, 0xE3, 0xDA, 0xBE, 0x68}; 

struct Point {
  int16_t x;
  int16_t y;
};
struct PathPacket {
  int packetId;      
  int count;         
  Point points[50];  
};
struct Telemetry {
  int step;
  float turn;
  float dist;
};
Point path[200]; 
int pathIndex = 0;
int curX = 64, curY = 64, prevX = 64, prevY = 64;
int moveSpeed = 2; 

void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
  Serial.print("\r\nPacket Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
void OnTelemetryRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len == sizeof(Telemetry)) {
    Telemetry* report = (Telemetry*)data;
    Serial.printf("\n[CAR TELEMETRY] Step: %d | Turn: %.1f deg | Dist: %.1f units\n", 
                  report->step, report->turn, report->dist);
  }
}
void setup() {
  Serial.begin(115200); 
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE); // Ensure channel is 1

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnTelemetryRecv); // REGISTER THE NEW CALLBACK
  
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, carAddress, 6);
  peerInfo.channel = 1; 
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  pinMode(UP_PIN, INPUT_PULLUP);
  pinMode(CENTER_PIN, INPUT_PULLUP); 
  pinMode(LEFT_PIN, INPUT_PULLUP);
  pinMode(DOWN_PIN, INPUT_PULLUP);
  pinMode(RIGHT_PIN, INPUT_PULLUP);
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, LOW); delay(200);
  digitalWrite(RESET_PIN, HIGH); delay(3000); 

  DisplaySerial.begin(9600, SERIAL_8N1, TX_PIN, RX_PIN);
  Display.gfx_Cls();
  
  Serial.println("System Online. Remote Listening for Car...");
}

void loop() {
  prevX = curX;
  prevY = curY;
  if (!digitalRead(UP_PIN))    curY -= moveSpeed;
  if (!digitalRead(DOWN_PIN))  curY += moveSpeed;
  if (!digitalRead(LEFT_PIN))  curX -= moveSpeed;
  if (!digitalRead(RIGHT_PIN)) curX += moveSpeed;

  curX = constrain(curX, 0, 127);
  curY = constrain(curY, 0, 127);
  if (prevX != curX || prevY != curY) {
    Display.gfx_Line(prevX, prevY, curX, curY, 0xCE60); 
    if (pathIndex < 200) {
      path[pathIndex] = { (int16_t)curX, (int16_t)curY };
      pathIndex++;
    }
  }
  if (!digitalRead(CENTER_PIN)) {
    if (pathIndex > 0) {
      Serial.println("\n===========================================");
      Serial.printf("STARTING TRANSMISSION: %d Total Points\n", pathIndex);
      Serial.println("===========================================");
      
      for (int i = 0; i < 4; i++) {
        int chunkStart = i * 50;
        if (chunkStart >= pathIndex) break; 
        
        PathPacket packet;
        packet.packetId = i; 
        int remaining = pathIndex - chunkStart;
        packet.count = (remaining > 50) ? 50 : remaining;
        
        memcpy(packet.points, &path[chunkStart], packet.count * sizeof(Point));

        esp_err_t result = esp_now_send(carAddress, (uint8_t *) &packet, sizeof(packet));
        
        if (result == ESP_OK) {
          Serial.printf("Status: Packet %d sent.\n", i);
        }
        delay(300); 
      }
      Serial.println("\n--- Transmission Finished. Waiting for Car to start... ---");
    }
    delay(1000); 
  }

  delay(20); 
}
