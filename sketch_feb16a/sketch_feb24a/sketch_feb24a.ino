// HELIOS X-1: TELEMETRY SIMULATOR (Mock Data Generator)
// ===============================================================
// - ไม่ต้องต่อเซ็นเซอร์ใดๆ ต่อแค่ NRF24L01 (CE=11, CSN=15)
// - สร้างคลื่นสมการคณิตศาสตร์เพื่อทดสอบ Dashboard ของ TCE SAT

#include "Arduino.h"
#include <SPI.h>
#include "RF24.h"

#define CE_PIN  11
#define CSN_PIN 15
RF24 radio(CE_PIN, CSN_PIN); 

const uint8_t RADIO_ADDR[6] = "EFEF0";
uint8_t csID = 0xA3;

// --- ตัวแปรเก็บค่าจำลอง ---
float simRoll = 0, simPitch = 0, simYaw = 0;
float simT1 = 25.0, simRH = 50.0;
float simBatV = 4.2, simBatPct = 100.0;
float simLat = 14.8724, simLng = 102.0220; // พิกัดเริ่มต้น
float simAlt = 400.0;
int simSats = 8;

// ==========================================
// CORE TRANSMISSION ENGINE (WITH CHECKSUM)
// ==========================================
void transmitNRF(const char* rawPayload) {
  char finalPacket[32]; 
  memset(finalPacket, 0, 32);
  
  uint8_t checksum = 0;
  for (int i = 0; i < strlen(rawPayload); i++) {
    checksum ^= rawPayload[i];
  }

  snprintf(finalPacket, 32, "%s*%02X", rawPayload, checksum);
  radio.write(&finalPacket, 32); 
  Serial.println(finalPacket); // พิมพ์ดูใน Serial Monitor ได้
}

void setup() {
  Serial.begin(9600); // ต้อง 9600 ให้ตรงกับ Ground Station
  
  // SPI Setup สำหรับ Pico
  SPI.setRX(16);
  SPI.setTX(19);
  SPI.setSCK(18);
  SPI.begin();
  
  // NRF24 Setup
  if (radio.begin(&SPI)) {
    radio.setChannel(62);
    radio.setPALevel(RF24_PA_LOW);
    radio.setDataRate(RF24_1MBPS);
    radio.setPayloadSize(32);
    radio.openWritingPipe(RADIO_ADDR);
    radio.stopListening();
    Serial.println("[SYS] Telemetry Simulator Ready.");
  } else {
    Serial.println("[ERR] NRF24 Radio Not Found!");
  }
}

unsigned long lastUpdate = 0;
unsigned long lastFastTX = 0;
unsigned long lastSlowTX = 0;

void loop() {
  unsigned long currentMillis = millis();

  // 1. อัปเดตค่าจำลองแบบ Real-time (สมการคณิตศาสตร์)
  if (currentMillis - lastUpdate >= 20) { // 50Hz Update
    // สร้างคลื่นให้โมเดล 3D หมุนไปมา
    simRoll = sin(currentMillis / 1000.0) * 180.0;  // แกว่ง -180 ถึง 180 องศา
    simPitch = cos(currentMillis / 1500.0) * 90.0;  // แกว่ง -90 ถึง 90 องศา
    simYaw = (currentMillis / 20) % 360;            // หมุนรอบตัวเอง 0-360 องศา

    // สร้างความผันผวนของเซ็นเซอร์สภาพแวดล้อม
    simT1 = 25.0 + (sin(currentMillis / 5000.0) * 5.0); // 20C - 30C
    simRH = 50.0 + (cos(currentMillis / 3000.0) * 10.0); // 40% - 60%

    // จำลองแบตเตอรี่ลดลง 100 -> 0 ทุกๆ 1 นาที
    simBatPct = 100.0 - ((currentMillis % 60000) / 600.0);
    simBatV = 3.0 + (simBatPct / 100.0) * 1.2; 

    // พิกัด GPS ขยับทีละนิด
    simLat = 14.8724 + (sin(currentMillis / 10000.0) * 0.005);
    simLng = 102.0220 + (cos(currentMillis / 10000.0) * 0.005);
    simAlt = 400.0 + (sin(currentMillis / 2000.0) * 50.0);

    lastUpdate = currentMillis;
  }

  // 2. ยิงข้อมูลความเร็วสูง (ท่าทาง 3D โมเดล) ทุกๆ 100ms (10Hz)
  if (currentMillis - lastFastTX >= 100) {
    char buf[28];
    snprintf(buf, sizeof(buf), "#C,%02X,%.1f,%.1f,%.1f,", csID, simRoll, simPitch, simYaw);
    transmitNRF(buf);
    lastFastTX = currentMillis;
  }

  // 3. ยิงข้อมูลความเร็วต่ำ (ระบบอื่นๆ) ทุกๆ 1000ms (1Hz)
  if (currentMillis - lastSlowTX >= 1000) {
    char buf[28];
    
    // GPS Main
    snprintf(buf, sizeof(buf), "#A,%02X,%.5f,%.5f,", csID, simLat, simLng);
    transmitNRF(buf);
    
    // GPS Aux
    snprintf(buf, sizeof(buf), "#B,%02X,%.1f,%d,%.1f,0.0,", csID, simAlt, simSats, 25.5); // ความเร็วจำลอง 25.5 m/s
    transmitNRF(buf);
    
    // BMS Power
    snprintf(buf, sizeof(buf), "#G,%02X,1,1,1,1,%.1f,%.0f,", csID, simBatV, simBatPct);
    transmitNRF(buf);

    lastSlowTX = currentMillis;
  }
}
