// =======================
// Arduino Loadcell + HX711 Demo
// Update setiap 50ms, perintah T via Serial untuk TARE/Zeroing
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

void setup() {
  Serial.begin(115200);
  while (!Serial); // Tunggu sampai Serial siap (untuk board dengan native USB)
  
  Serial.println();
  Serial.println("== LOADCELL + HX711 Demo ==");
  Serial.println("Perintah Serial: ");
  Serial.println("  Kirim 'T' untuk TARE/Zeroing");
  Serial.println("------------------------------------");
  
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(calibration_factor); // Set faktor kalibrasi loadcell
  scale.tare(); // Zeroing awal
  
  Serial.print("Tare... Berat offset di-nolkan.\n");
  delay(500);

  // Inisialisasi variabel laju perubahan
  lastWeight = abs(scale.get_units(1));
  lastWeightTime = millis();
}

void loop() {
  // Cek ada data di Serial (perintah TARE)
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 'T' || cmd == 't') {
      Serial.println("\nPerintah TARE diterima, meng-nol-kan berat...");
      scale.tare();
      Serial.println("Berat dinolkan!");
      // Reset variabel laju perubahan
      lastWeight = 0.0;
      lastWeightTime = millis();
    }
    // Kirim instruksi ulang jika input tidak dikenal
    else {
      Serial.println("Perintah tidak dikenal. Kirim 'T' untuk tare/zero.");
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
      rate = abs((weight - lastWeight) / (dt / 400.0)); // gram per detik
    }

//    Serial.print("Laju: ");
    Serial.print(rate, 2); // dua desimal
    Serial.print(",");
//    Serial.print(" g/detik");

//    Serial.print(" | Berat: ");
    Serial.println(weight, 2); // dua desimal
//    Serial.println(" gram");

    // Simpan nilai untuk iterasi berikutnya
    lastWeight = weight;
    lastWeightTime = now;
  }
}

// --------- Tips Kalibrasi Faktor -----------
// 1. Ubah nilai calibration_factor di atas (misal ±20 per step), upload ulang.
// 2. Letakkan benda referensi di loadcell, cek nilai berat pada Serial.
// 3. Sesuaikan calibration_factor hingga angka yang terbaca sesuai dengan berat sebenarnya.
// 4. Anda juga bisa atur via perintah Serial jika perlu.
//
