#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Konfigurasi pin photodioda
#define PHOTO_PIN A0

// Konfigurasi pin Relay
#define RELAY_PIN 4

// --- Konfigurasi Pin RGB LED ---
#define LED_R_PIN 7
#define LED_G_PIN 6
#define LED_B_PIN 5

// --- Konfigurasi Pin Sensor TCS3200 ---
#define S0_PIN 8
#define S1_PIN 9
#define S2_PIN 10
#define S3_PIN 11
#define OUT_PIN 12

// Waktu jeda pembacaan TCS3200 (dalam milidetik)
#define FILTER_DELAY 100
#define SAMPLE_DELAY 50

// Jumlah pembacaan TCS3200 untuk menghitung rata-rata
#define SAMPLE_COUNT 10

// --- Ambang Batas Kelayakan Minyak ---
// PENTING: Nilai ini harus Anda kalibrasi sendiri!
// Ukur nilai 'Merah Mentah' pada minyak baru, lalu pada minyak bekas.
// Tentukan nilai ambang batas di antara keduanya.
// Nilai yang lebih TINGGI menandakan minyak lebih GELAP/MERAH.
#define AMBANG_BATAS_MERAH  2878
#define AMBANG_BATAS_HIJAU  3188
#define AMBANG_BATAS_BIRU   2384
#define AMBANG_BATAS_CLEAR  1
#define AMBANG_BATAS_PHOTO  706

// --- Pengaturan LCD I2C ---
// Atur alamat I2C ke 0x27 untuk LCD 16x2. Mungkin berbeda (misal 0x3F).
// Gunakan I2C Scanner jika alamat ini tidak berfungsi.
LiquidCrystal_I2C lcd(0x27, 16, 2);

enum Freq : uint8_t {
  F0 = 0b00,
  F2 = 0b01,
  F20 = 0b10,
  F100 = 0b11
};

enum Color : uint8_t {
  Red = 0b00,
  Blue = 0b01,
  Clear = 0b10,
  Green = 0b11
};

void setFreq(Freq freq) {
  digitalWrite(S0_PIN, (freq >> 1) & 0x1);
  digitalWrite(S1_PIN, (freq) & 0x1);
  delay(FILTER_DELAY);
}

void setFilter(Color color) {
  digitalWrite(S2_PIN, (color >> 1) & 0x1);
  digitalWrite(S3_PIN, (color) & 0x1);
  delay(FILTER_DELAY);
}

// --- Fungsi untuk Membaca Warna dari TCS3200 ---
// uint16_t readColor(Color color) {
//   setFilter(color);
//   uint16_t val = pulseIn(OUT_PIN, LOW); // Mengukur durasi pulsa LOW
//   delay(SAMPLE_DELAY);
//   return val;
// }

uint16_t getColorAvg(Color color) {
  uint32_t sum = 0;
  setFilter(color);
  for (uint8_t i = 0; i < SAMPLE_COUNT; i++) {
    sum += pulseIn(OUT_PIN, LOW);
    delay(SAMPLE_DELAY);
  }
  return sum / SAMPLE_COUNT;
}

// --- Fungsi untuk Mengatur Warna LED RGB ---
void setLedColor(uint8_t red, uint8_t green, uint8_t blue) {
  digitalWrite(LED_R_PIN, red);
  digitalWrite(LED_G_PIN, green);
  digitalWrite(LED_B_PIN, blue);
}

void nyalakanTCS() {
  digitalWrite(RELAY_PIN, HIGH);
  setFreq(F2);
  // TODO: nyalakan juga OE
}

void matikanTCS() {
  digitalWrite(RELAY_PIN, LOW);
  setFreq(F0);
  setFilter(Red);
  // TODO: matikan juga OE
}

void setup() {
  // // Matikan lampu pada pin 13
  // pinMode(13, OUTPUT);
  // digitalWrite(13, LOW);

  // // Inisialisasi pin relay
  // pinMode(RELAY_PIN, OUTPUT);
  // digitalWrite(RELAY_PIN, HIGH);

  // Inisialisasi pin TCS3200
  pinMode(S0_PIN, OUTPUT), pinMode(S1_PIN, OUTPUT);
  pinMode(S2_PIN, OUTPUT), pinMode(S3_PIN, OUTPUT);
  pinMode(OUT_PIN, INPUT);

  // Inisialisasi pin LED RGB
  pinMode(LED_R_PIN, OUTPUT);
  pinMode(LED_G_PIN, OUTPUT);
  pinMode(LED_B_PIN, OUTPUT);

  Serial.begin(9600);

  // Inisialisasi LCD
  lcd.init();
  lcd.backlight();
  lcd.display();

  lcd.setCursor(0, 0), lcd.print("Alat Pemantau");
  lcd.setCursor(0, 1), lcd.print("Minyak Goreng");

  Serial.println("Alat Pemantau Minyak Goreng Siap!");

  delay(2000);
  lcd.clear();
  nyalakanTCS();
}

void loop() {
  Serial.println("============================");

  /* -------- Membaca Sensor TCS3200 -------------- */
  //nyalakanTCS();

  // Membaca nilai rata-rata dari setiap filter warna
  uint16_t red_avg = getColorAvg(Red);
  uint16_t green_avg = getColorAvg(Green);
  uint16_t blue_avg = getColorAvg(Blue);

  Serial.print("Merah: "), Serial.println(red_avg);
  Serial.print("Hijau: "), Serial.println(green_avg);
  Serial.print("Biru: "), Serial.println(blue_avg);

  /* -------- Membaca Sensor Photodiode ------------- */
  // matikanTCS();

  // Membaca nilai dari sensor photodioda (MH-series)
  uint32_t total = 0;
  for (uint8_t i = 0; i < SAMPLE_COUNT; i++) {
    total += analogRead(PHOTO_PIN);
    delay(2);
  }
  uint16_t photo_avg = total / SAMPLE_COUNT;
  Serial.print("Photodioda: "); Serial.println(photo_avg);

  // --- Logika Penentuan Kelayakan Minyak ---
  Serial.println("----------------------------");
  lcd.setCursor(0, 0);
  lcd.print("Status:         ");

  // if (red_avg > AMBANG_BATAS_MERAH) {
  // if (red_avg > AMBANG_BATAS_MERAH && green_avg > AMBANG_BATAS_HIJAU) {
  if ( // jika
    red_avg >= AMBANG_BATAS_MERAH
      && // dan 
    green_avg >= AMBANG_BATAS_HIJAU
      && // dan
    blue_avg >= AMBANG_BATAS_BIRU
      && // dan
    photo_avg >= AMBANG_BATAS_PHOTO
  ) {
    // Jika nilai merah mentah MELEBIHI ambang batas, minyak dianggap tidak layak
    lcd.setCursor(0, 1), lcd.print("TIDAK LAYAK     ");
    Serial.println("Status: TIDAK LAYAK");
    setLedColor(1, 0, 0);
  } else {
    // Jika tidak, minyak dianggap layak pakai
    lcd.setCursor(0, 1), lcd.print("      LAYAK     ");
    Serial.println("Status: LAYAK");
    setLedColor(0, 1, 0);
  }
}
