#include "GSheet.h"
#include <HX711.h>
#include <ESP32Servo.h>

// GSheet untuk Google Sheets
GSheet Sheet("AKfycbxqj7fMJl7hUGovIReBfjli15bJ_kMah4bY0xiHRtijnv3v-53E98SMB3K7tur6x_My");

// Definisi pin
const int LOADCELL_DOUT_PIN = 2;
const int LOADCELL_SCK_PIN  = 4;
const int BUTTON_PIN        = 15;    
const int SERVO_YAW_PIN     = 13;  // servo1
const int SERVO_PITCH_PIN   = 14;  // servo2

// Inisialisasi objek
HX711 scale;
Servo servoYaw;
Servo servoPitch;

// Kalibrasi timbangan
const float CALIBRATION_FACTOR = 189.00;//60.88
float offset = 0;

// Debounce button
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50;

// Status gerakan servo
bool isServoMoving = false;

// WiFi credentials
const char* ssid     = "wefee"; 
const char* password = "wepaywefee";

// Batas kategori berat (Kg)
const float BERAT_C = 100.0;
const float BERAT_B = 200.0;
const float BERAT_A = 300.0;

float berat;
String kategori;

enum State {
  IDLE,
  MEASURING,
  MOVING
};

State currentState = IDLE;
unsigned long lastMoveTime = 0;


float getWeight() {
  // if (!scale.wait_ready_timeout(200)) return -1.0;
  // return abs(scale.get_units(10));
  long sum = 0;
  for (int i = 0; i < 100; i++) {
      while (!scale.is_ready()) {};        // tunggu siap
      sum += scale.get_units();           // baca bobot
  }
  float beban = sum / 100.0;               // rata-rata 10 bacaan
  return beban;
}

void tareScale() {
  Serial.println("Proses tare...");
  scale.tare(100);
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

String tentukanKategori(float w) {
  if (w > 11 && w <= BERAT_C) {
    return "GradeC";
  } else if (w > BERAT_C && w <= BERAT_B) {
    return "GradeB";
  } else if (w > BERAT_B && w <= BERAT_A) {
    return "GradeA";
  } else {
    return "stay";
  }
}

// Gerak servo dengan step
void moveServo(Servo &sv, int startDeg, int endDeg, int stepDeg) {
  int dir = (endDeg > startDeg) ? 1 : -1;
  int step = abs(stepDeg) * dir;
  for (int pos = startDeg; (dir > 0 ? pos <= endDeg : pos >= endDeg); pos += step) {
    sv.write(pos);
    // isServoMoving = true;
    delay(100);
  }
}

void executeGradeC() {
  // Hanya yaw
  moveServo(servoYaw, 0, 45, 5);
  // delay(500);
  moveServo(servoYaw, 45, 0, 5);
}

void executeGradeB() {
  // Pitch 0->60
  moveServo(servoPitch, 0, 60, 5);
  // Setelah pitch selesai, yaw 0->45
  moveServo(servoYaw, 0, 45, 5);
  // delay(1000);
  // Kembali yaw 45->0
  moveServo(servoYaw, 45, 0, 5);
  // Kembali pitch 60->0
  moveServo(servoPitch, 60, 0, 5);
}

void executeGradeA() {
  // Pitch 0->120
  moveServo(servoPitch, 0, 120, 5);
  // Setelah pitch selesai, yaw 0->45
  moveServo(servoYaw, 0, 45, 5);
  // delay(1000);
  // Kembali yaw 45->0
  moveServo(servoYaw, 45, 0, 5);
  // Kembali pitch 120->0
  moveServo(servoPitch, 120, 0, 5);
}

void setup() {
  Serial.begin(115200);

  // Inisialisasi timbangan
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(CALIBRATION_FACTOR);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  servoYaw.attach(SERVO_YAW_PIN);
  servoPitch.attach(SERVO_PITCH_PIN);
  servoYaw.write(0);
  servoPitch.write(0);

  // tareScale();

  // Koneksi WiFi
  Sheet.connectWiFi(ssid, password);
}

void loop() {
  checkTareButton();

  switch (currentState) {
    case IDLE:
      scale.power_up();
      berat = getWeight();
      kategori = tentukanKategori(berat);

      if (kategori != "stay") {
        Serial.println("Berat: " + String(berat) + " Kg, Kategori: " + kategori);
        Sheet.sendData(String(berat) + "Kg", kategori);

        if (Sheet.getHttpCode() > 0) {
          scale.power_down();
          currentState = MOVING;
        }
      }
      break;

    case MOVING:
      if (kategori == "GradeC") {
        executeGradeC();
      } else if (kategori == "GradeB") {
        executeGradeB();
      } else if (kategori == "GradeA") {
        executeGradeA();
      }
      currentState = IDLE;
      Serial.println("loadcell ready");
      break;

    default:
      currentState = IDLE;
      break;
  }

  delay(50);
}
