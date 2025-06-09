// =======================
// Arduino Loadcell + HX711 Demo with Calibration Factor via Serial and MAV filter for rate
// Update setiap 50ms, perintah T via Serial untuk TARE/Zeroing
// Perintah C diikuti angka untuk kalibrasi
// =======================

#include "HX711.h"

// --- PIN DEFINISI (ganti sesuai wiring Anda) ---
#define LOADCELL_DOUT_PIN  8
#define LOADCELL_SCK_PIN   9

// --- Inisialisasi HX711 ---
HX711 scale;

// Set calibration factor. Anda HARUS atur sesuai sensor/loadcell Anda!
// Lakukan kalibrasi manual: misal mulai dengan 420, sesuaikan agar berat aktual sesuai.
long calibration_factor = 198;

// --- Batas waktu pembacaan ---
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 50; // ms

// Variabel untuk laju perubahan berat
float lastWeight = 0.0;
unsigned long lastWeightTime = 0;

// --- MAV filter variables ---
const int MAV_SIZE = 10;
float rateBuffer[MAV_SIZE];
int rateIndex = 0;
bool rateBufferFilled = false;

void setup() {
  Serial.begin(115200);
  while (!Serial); // Tunggu sampai Serial siap (untuk board dengan native USB)
  
  Serial.println();
  Serial.println("== LOADCELL + HX711 Demo with Calibration & MAV Filter ==");
  Serial.println("Perintah Serial: ");
  Serial.println("  Kirim 'T' untuk TARE/Zeroing");
  Serial.println("  Kirim 'C' diikuti angka untuk mengatur kalibrasi (misal: C420)");
  Serial.println("------------------------------------");
  
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(calibration_factor); // Set faktor kalibrasi loadcell
  scale.tare(); // Zeroing awal
  
  Serial.println("Tare... Berat offset di-nolkan.");
  delay(500);

  // Inisialisasi variabel laju perubahan
  lastWeight = abs(scale.get_units(1));
  lastWeightTime = millis();

  // Inisialisasi buffer rate ke 0
  for (int i = 0; i < MAV_SIZE; i++) {
    rateBuffer[i] = 0.0;
  }
}

void loop() {
  // Cek ada data di Serial (perintah TARE atau kalibrasi)
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 'T' || cmd == 't') {
      Serial.println("\nPerintah TARE diterima, meng-nol-kan berat...");
      scale.tare();
      Serial.println("Berat dinolkan!");
      // Reset variabel laju perubahan dan buffer MAV
      lastWeight = 0.0;
      lastWeightTime = millis();
      for (int i = 0; i < MAV_SIZE; i++) {
        rateBuffer[i] = 0.0;
      }
      rateIndex = 0;
      rateBufferFilled = false;
    }
    else if (cmd == 'C' || cmd == 'c') {
      // Tunggu hingga ada angka setelah 'C'
      while (Serial.available() == 0); // Tunggu input
      String input = Serial.readStringUntil('\n'); // Baca input hingga newline
      long newCalibrationFactor = input.toInt(); // Konversi ke long
      if (newCalibrationFactor != 0) { // Pastikan input valid
        calibration_factor = newCalibrationFactor;
        scale.set_scale(calibration_factor); // Set faktor kalibrasi baru
        Serial.print("Faktor kalibrasi diatur ke: ");
        Serial.println(calibration_factor);
      } else {
        Serial.println("Input tidak valid. Pastikan memasukkan angka.");
      }
    }
    // Kirim instruksi ulang jika input tidak dikenal
    else {
      Serial.println("Perintah tidak dikenal. Kirim 'T' untuk tare/zero atau 'C' untuk kalibrasi.");
    }
  }

  // Pembacaan berat periodik
  unsigned long now = millis();
  if (now - lastUpdate >= updateInterval) {
    lastUpdate = now;
    float weight = abs(scale.get_units(1)); // rata-rata 1 kali, bisa dibuat lebih smooth jika perlu

    // Hitung laju perubahan berat (gram per detik)
    unsigned long dt = now - lastWeightTime; // ms
    float rate = 0.0;
    if (dt > 0) {
      rate = abs((weight - lastWeight) / (dt / 1000.0)); // gram per detik (fixed correct dt to seconds)
    }

    // Tambahkan rate ke buffer MAV untuk filter
    rateBuffer[rateIndex] = rate;
    rateIndex++;
    if (rateIndex >= MAV_SIZE) {
      rateIndex = 0;
      rateBufferFilled = true;
    }

    // Hitung rata-rata dari buffer MAV
    int count = rateBufferFilled ? MAV_SIZE : rateIndex;
    float rateSum = 0.0;
    for (int i = 0; i < count; i++) {
      rateSum += rateBuffer[i];
    }
    float filteredRate = (count > 0) ? (rateSum / count) : 0.0;

    Serial.print(filteredRate, 2); // dua desimal
    Serial.print(",");
    Serial.println(weight, 2); // dua desimal

    // Simpan nilai untuk iterasi berikutnya
    lastWeight = weight;
    lastWeightTime = now;
  }
}
