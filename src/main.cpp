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
const float CALIBRATION_FACTOR = 189.00;
float offset = 0;

// Debounce button
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50;

// Status gerakan servo
bool isServoMoving = false;
bool lastButtonState = HIGH;

// WiFi credentials
const char* ssid     = "wefee"; 
const char* password = "wepaywefee";

// Batas kategori berat (g)
const float BERAT_C = 100.0;
const float BERAT_B = 200.0;
const float BERAT_A = 300.0;

void applicationTask0(void *param);
void applicationTask1(void *param);

float berat;
String kategori = "stay";
unsigned long lastMoveTime = 0;


float getWeight() {
  long sum = 0;
  for (int i = 0; i < 120; i++) {
      while (!scale.is_ready()) {};        // tunggu siap
      sum += scale.get_units();           // baca bobot
  }
  float beban = sum / 120.0;               // rata-rata 120 bacaan
  return beban;
}

void tareScale() {
  Serial.println("Proses tare...");
  scale.tare(120);
  offset = scale.get_offset();
}

void checkTareButton() {
  bool currentButtonState = digitalRead(BUTTON_PIN);

  if (currentButtonState != lastButtonState)
  {
    if (currentButtonState == LOW)
    {
      tareScale();
      Serial.println("Tare berhasil!");
      lastDebounceTime = millis();
    }
    lastButtonState = currentButtonState;
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
    delay(100);
  }
}

void executeGradeC() {
  // Hanya yaw
  moveServo(servoYaw, 0, 45, 5);
  moveServo(servoYaw, 45, 0, 5);
}

void executeGradeB() {
  // Pitch 0->60
  moveServo(servoPitch, 0, 60, 5);
  moveServo(servoYaw, 0, 45, 5);

  moveServo(servoYaw, 45, 0, 5);
  moveServo(servoPitch, 60, 0, 5);
}

void executeGradeA() {
  // Pitch 0->120
  moveServo(servoPitch, 0, 120, 5);
  moveServo(servoYaw, 0, 45, 5);

  moveServo(servoYaw, 45, 0, 5);
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

  // Koneksi WiFi
  TaskHandle_t task0;
  xTaskCreatePinnedToCore(
    applicationTask0,
    "applicationTask0",
    10000,
    NULL,
    1,
    &task0,
    0
  );

  TaskHandle_t task1;
  xTaskCreatePinnedToCore(
    applicationTask1,
    "applicationTask1",
    10000,
    NULL,
    1,
    &task0,
    1
  );
  
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(50));
}

void applicationTask0(void *param){
  Sheet.connectWiFi(ssid, password);
  while (true)
  {
      if (isServoMoving == true) {
        Sheet.sendData(String(berat) + "g", kategori);
        isServoMoving = false;
      }
    delay(50);
  }
}

void applicationTask1(void *param){
  enum State {
    IDLE,
    MEASURING,
    MOVING
  };
  State currentState = IDLE;
  tareScale();
  while (true)
  {
    checkTareButton();

    switch (currentState) {
      case IDLE:
        scale.power_up();
        berat = getWeight();
        kategori = tentukanKategori(berat);
  
        if (kategori != "stay") {
          isServoMoving = true;
          currentState = MOVING;
        }else{
          isServoMoving = false;
        }
        break;
  
      case MOVING:
        scale.power_down();
        if (kategori == "GradeC") {
          executeGradeC();
        } else if (kategori == "GradeB") {
          executeGradeB();
        } else if (kategori == "GradeA") {
          executeGradeA();
        }
        currentState = IDLE;
        break;
  
      default:
        currentState = IDLE;
        break;
    }
    delay(50);
  }
}
