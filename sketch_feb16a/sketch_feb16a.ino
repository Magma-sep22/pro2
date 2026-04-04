// CubeSat v2 MCB - ADVANCED SIMULATOR (WITH OBJECT DETECTION)
// ===========================================================
// Target: Raspberry Pi Pico / ESP32
// Purpose: Ground Station Testing with Object Detection Scenario

#include <Arduino.h>
#include <math.h>

// --- Configuration ---
const uint8_t CS_ID = 0xA3;
#define PACKET_SIZE 128 

// --- Data Structures ---
struct SensorData {
  double lat, lng;
  float alt, speed, heading;
  float ax, ay, az;
  float gx, gy, gz;
  float mx, my, mz;
  float roll, pitch, yaw;
  float temp_int, temp_ext;
  int rssi;
};

struct PowerSystem {
  float batVoltage;
  float batCurrent;
  float batPercent;
  bool isCharging;
};

// ** NEW: โครงสร้างข้อมูลสำหรับวัตถุที่เจอ **
struct ObjectTracker {
  int type;           // 0=None, 1=Satellite, 2=Debris, 3=Unknown
  float distance;     // เมตร
  float closingSpeed; // เมตร/วินาที
  float azimuth;      // องศา
  unsigned long detectTime;
  bool active;
};

// Global Instances
SensorData sensors;
PowerSystem power;
ObjectTracker radar; // สร้างตัวแปรเรดาร์

// --- Physics Variables ---
double orbitTheta = 0.0;
unsigned long lastUpdate = 0;
int packetCycle = 0;

// --- Function Prototypes ---
void updatePhysics(float dt);
void updateRadarSimulation(float dt); // ฟังก์ชันใหม่
void sendNextPacket();
float noise(float range);

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Initial Values
  sensors.lat = 14.881;
  sensors.lng = 102.020;
  sensors.alt = 400.0;
  power.batPercent = 85.0;
  randomSeed(analogRead(0));
}

void loop() {
  unsigned long currentMillis = millis();
  float dt = (currentMillis - lastUpdate) / 1000.0;

  if (dt >= 0.05) { 
    updatePhysics(dt);
    updateRadarSimulation(dt); // คำนวณการเจอวัตถุ
    lastUpdate = currentMillis;
  }

  static unsigned long lastTx = 0;
  if (currentMillis - lastTx >= 100) { 
    sendNextPacket();
    lastTx = currentMillis;
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }

  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    if (cmd.trim() == "RESET") setup();
  }
}

// ==========================================
//            PHYSICS ENGINE
// ==========================================
void updatePhysics(float dt) {
  // 1. Orbit
  orbitTheta += 0.0005; 
  if (orbitTheta > 6.28) orbitTheta = 0;
  sensors.lat = 14.881 + sin(orbitTheta) * 0.8; 
  sensors.lng = 102.020 + cos(orbitTheta) * 0.8;
  sensors.alt = 400.0 + sin(orbitTheta * 5) * 5.0;
  sensors.speed = 7.6 + noise(0.05);
  
  // 2. Attitude
  sensors.roll  = sin(millis() / 5000.0) * 15.0;
  sensors.pitch = cos(millis() / 4000.0) * 10.0;
  sensors.yaw   += 0.5; if(sensors.yaw > 360) sensors.yaw = 0;
  
  // 3. Power
  bool inSun = sin(orbitTheta) > -0.2;
  power.isCharging = inSun;
  float drain = 0.15;
  float charge = inSun ? 0.6 : 0.0;
  power.batPercent += (charge - drain) * dt * 2.0;
  if (power.batPercent > 100) power.batPercent = 100;
  if (power.batPercent < 0) power.batPercent = 0;
  power.batVoltage = 3.3 + (power.batPercent / 100.0) * 0.9;
  
  // 4. RSSI & Temp
  sensors.rssi = -90 + (int)(sin(orbitTheta * 2) * 20) + (int)noise(2);
  sensors.temp_ext = inSun ? 55.0 : -15.0;
}

// ==========================================
//       ** NEW: RADAR SIMULATION **
// ==========================================
void updateRadarSimulation(float dt) {
  // ถ้ายังไม่เจออะไร ให้สุ่มโอกาสเจอ (โอกาส 1% ต่อรอบการคำนวณ)
  if (!radar.active) {
    if (random(0, 500) < 5) { // เจอวัตถุ!
      radar.active = true;
      radar.type = random(1, 4); // 1=Sat, 2=Debris, 3=Unknown
      radar.distance = random(2000, 5000); // เริ่มที่ 2-5 km
      radar.closingSpeed = random(10, 500); // ความเร็วเข้าหา
      radar.azimuth = random(0, 360);
      radar.detectTime = millis();
    }
  } else {
    // ถ้ากำลัง track วัตถุอยู่ ให้คำนวณระยะทางที่ลดลง
    radar.distance -= radar.closingSpeed * dt;
    
    // เปลี่ยนมุมเล็กน้อย (Simulate การเคลื่อนที่ผ่าน)
    radar.azimuth += 0.1;

    // ถ้าวัตถุผ่านไปแล้ว (ระยะติดลบ) หรือนานเกินไป ให้เคลียร์ค่า
    if (radar.distance <= 0 || (millis() - radar.detectTime > 15000)) {
      radar.active = false;
      radar.type = 0;
      radar.distance = 0;
      radar.closingSpeed = 0;
    }
  }
}

// ==========================================
//            PACKET GENERATOR
// ==========================================
void sendNextPacket() {
  char packet[PACKET_SIZE];
  memset(packet, 0, PACKET_SIZE);

  switch (packetCycle) {
    case 0: // GPS
      sprintf(packet, "#A,%02X,%.5f,%.5f,*", CS_ID, sensors.lat, sensors.lng);
      break;
    case 1: // Nav
      sprintf(packet, "#B,%02X,%.1f,%d,%.2f,*", CS_ID, sensors.alt, 8, sensors.speed);
      break;
    case 2: // Attitude
      sprintf(packet, "#C,%02X,%.2f,%.2f,%.2f,*", CS_ID, sensors.roll, sensors.pitch, sensors.yaw);
      break;
    // ... (ละ IMU บางตัวเพื่อประหยัดรอบ) ...
    case 3: // Power
      sprintf(packet, "#G,%02X,%d,%.2f,%.1f,*", CS_ID, power.isCharging, power.batVoltage, power.batPercent);
      break;
    case 4: // Environment
      sprintf(packet, "#H,%02X,%.2f,%.2f,%d,*", CS_ID, sensors.temp_int, sensors.temp_ext, sensors.rssi);
      break;
      
    // ** NEW: LIDAR / OBJECT PACKET **
    // Format: #L, [Type], [Distance], [Speed], [Azimuth], *
    case 5: 
      if (radar.active) {
        sprintf(packet, "#L,%d,%.1f,%.1f,%.1f,*", radar.type, radar.distance, radar.closingSpeed, radar.azimuth);
      } else {
        // Heartbeat ถ้าไม่เจออะไร
        sprintf(packet, "#L,0,0.0,0.0,0.0,*");
      }
      break;
  }

  if (strlen(packet) > 0) Serial.println(packet);

  packetCycle++;
  if (packetCycle > 5) packetCycle = 0; // วนรอบสั้นลง
}

float noise(float range) { return ((float)random(0, 1000) / 500.0 - 1.0) * range; }
