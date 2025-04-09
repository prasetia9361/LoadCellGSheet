#include "GSheet.h"
GSheet Sheet("AKfycbxqj7fMJl7hUGovIReBfjli15bJ_kMah4bY0xiHRtijnv3v-53E98SMB3K7tur6x_My");

#include <HX711.h>
const int LOADCELL_DOUT_PIN = 2;
const int LOADCELL_SCK_PIN = 4;
const int BUTTON_PIN = 15;    // GPIO pin untuk tombol tare
HX711 scale;

// Kalibrasi sensor (sesuaikan dengan sistem anda)
float CALIBRATION_FACTOR = -450.0; // Ubah sesuai kalibrasi
float OFFSET = 0;          // Ubah sesuai offset
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

// Flag untuk menandai servo sedang bergerak
bool isServoMoving = false;

#include <ESP32Servo.h>
const int servo1Pin = 13;  // GPIO pin untuk servo1
const int servo2Pin = 14;  // GPIO pin untuk servo2
Servo myservo1;
Servo myservo2;

const char* ssid = "your WiFi ssid"; 
const char* password = "your WiFi password";

float getWeight() {
  if (scale.wait_ready_timeout(200)) {
    return abs(scale.get_units(10)); // Baca 5 sample dan rata-rata
  }
  return -1.0; // Return error
}

void checkTareButton() {
  int buttonState = digitalRead(BUTTON_PIN);
  
  if (buttonState == LOW) { // Tombol ditekan (karena PULLUP)
    if ((millis() - lastDebounceTime) > debounceDelay) {
      tareScale();
      Serial.println("Tare berhasil!");
    }
    lastDebounceTime = millis();
  }
}

void tareScale() {
  Serial.println("Proses tare...");
  
  // Ambil 10 pembacaan untuk baseline
  scale.tare(10);
  OFFSET = scale.get_offset();
  
  Serial.print("Offset baru: ");
  Serial.println(OFFSET);
}

String tentukanKategori(float berat) {
  if (berat < 20) {
    return "stay";
  } else if (berat >= 20 && berat < 100) {
    return "GradeC";
  } else if (berat >= 100 && berat < 200) {
    return "GradeB";
  } else if (berat >= 200 && berat < 300) {
    return "GradeA";
  }
  return "Tidak Dikenali";
}

void setup() {
  Serial.begin(115200);

  // Inisialisasi load cell
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(CALIBRATION_FACTOR);
  
  // Inisialisasi tombol
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Auto-tare awal
  tareScale();

  myservo1.attach(servo1Pin);  // Attach servo1 ke pin 13
  myservo2.attach(servo2Pin);  // Attach servo2 ke pin 14
  
  // Inisialisasi posisi awal
  myservo1.write(0);         // Set ke posisi 0 derajat
  myservo2.write(0);         // Set ke posisi 0 derajat

  Sheet.connectWiFi(ssid, password);
  Sheet.clearData(); // function to delete all data in the sheet
  // Add // if you don't want it to be used
}

void loop() {
  // Hanya jalankan pengukuran dan pengiriman jika servo tidak sedang bergerak
  if (!isServoMoving) {
    float berat = getWeight();
    String kategori = tentukanKategori(berat);
    if (kategori != "stay")
    {
      Sheet.sendData(String(berat), kategori);
      if (Sheet.getHttpCode() > 0)
      {
        isServoMoving = true; // Set flag bahwa servo mulai bergerak
        
        // Gerakan dari 0° ke 180° dengan langkah lebih besar
        for(int pos = 0; pos <= 180; pos += 5) {
          myservo1.write(pos);
          myservo2.write(pos);
          delay(10);  // Kecepatan rotasi lebih cepat
        }
        
        delay(1000); // Waktu tunggu lebih singkat
        
        // Gerakan kembali dari 180° ke 0° dengan langkah lebih besar
        for(int pos = 180; pos >= 0; pos -= 5) {
          myservo1.write(pos);
          myservo2.write(pos);
          delay(10);
        }
        
        isServoMoving = false; // Reset flag setelah servo selesai bergerak
      }
    }
  }
  
  // Cek tombol tare
  checkTareButton();
}