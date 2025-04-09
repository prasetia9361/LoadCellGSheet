#include "GSheet.h"
GSheet Sheet("AKfycbxqj7fMJl7hUGovIReBfjli15bJ_kMah4bY0xiHRtijnv3v-53E98SMB3K7tur6x_My");

#include <HX711.h>
#include <ESP32Servo.h>

// Definisi pin
const int LOADCELL_DOUT_PIN = 2;
const int LOADCELL_SCK_PIN = 4;
const int BUTTON_PIN = 15;    
const int servo1Pin = 13;  
const int servo2Pin = 14;  

// Inisialisasi objek
HX711 scale;
Servo myservo1;
Servo myservo2;

// Konstanta kalibrasi
const float CALIBRATION_FACTOR = -450.0;
float offset = 0;

// Variabel untuk debouncing
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50;

// Variabel status
bool isServoMoving = false;

// Konfigurasi WiFi
const char* ssid = "your WiFi ssid"; 
const char* password = "your WiFi password";

// Konstanta untuk kategori berat
const float BERAT_MIN = 20.0;
const float BERAT_C = 100.0;
const float BERAT_B = 200.0;
const float BERAT_A = 300.0;

float getWeight() {
  return scale.wait_ready_timeout(200) ? abs(scale.get_units(5)) : -1.0;
}

void tareScale() {
  Serial.println("Proses tare...");
  scale.tare(10);
  offset = scale.get_offset();
  Serial.println("Offset baru: " + String(offset));
}

void checkTareButton() {
  if (digitalRead(BUTTON_PIN) == LOW && (millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    tareScale();
    Serial.println("Tare berhasil!");
    lastDebounceTime = millis();
  }
}

String tentukanKategori(float berat) {
  if (berat < BERAT_MIN) return "stay";
  if (berat < BERAT_C) return "GradeC";
  if (berat < BERAT_B) return "GradeB"; 
  if (berat < BERAT_A) return "GradeA";
  return "Tidak Dikenali";
}

void movePitch(int awal, int akhir, int langkah) {
  int increment = (akhir > awal) ? langkah : -langkah;
  for(int pos = awal; (increment > 0) ? pos <= akhir : pos >= akhir; pos += increment) {
    myservo2.write(pos);
    delay(10);
  }
}

void moveYaw(int awal, int akhir, int langkah) {
  int increment = (akhir > awal) ? langkah : -langkah;
  for(int pos = awal; (increment > 0) ? pos <= akhir : pos >= akhir; pos += increment) {
    myservo1.write(pos);
    delay(10);
  }
}

void setup() {
  Serial.begin(115200);

  // Setup load cell
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(CALIBRATION_FACTOR);
  
  // Setup pin dan servo
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  myservo1.attach(servo1Pin);
  myservo2.attach(servo2Pin);
  myservo1.write(0);
  myservo2.write(0);
  
  tareScale();

  // Setup WiFi
  Sheet.connectWiFi(ssid, password);
  Sheet.clearData();
}

void loop() {
  if (!isServoMoving) {
    float berat = getWeight();
    String kategori = tentukanKategori(berat);
    
    if (kategori != "stay") {
      Sheet.sendData(String(berat), kategori);
      if (Sheet.getHttpCode() > 0)
      {
        isServoMoving = true;
      
        movePitch(0, 180, 5);  // Gerakan maju
        moveYaw(0, 45, 5);
        delay(500);                 // Waktu tunggu dikurangi
        movePitch(180, 0, 5);  // Gerakan mundur
        moveYaw(45, 0, 5);
        
        isServoMoving = false;
      }
    }
  }
  
  checkTareButton();
}