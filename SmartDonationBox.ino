#define S2 2
#define S3 4
#define sensorOut 15

#define TRIG_PIN 12  // Sesuaikan dengan pin yang digunakan
#define ECHO_PIN 13

// Tambahkan pin untuk sensor getar SW-420
#define VIBRATION_SENSOR_PIN 25 

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Wire.h>
#include <MFRC522.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// RFID
#define RST_PIN         27
#define SS_PIN          5
#define SELENOID_PIN    26
MFRC522 rfid(SS_PIN, RST_PIN);


// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// WiFi dan Bot
#define WIFI_SSID "mez"
#define WIFI_PASSWORD "20202020"
#define BOT_TOKEN "8070595869:AAFYe1t-qSm9YKYXzGi84u2igzDZ2rO7ljc"
#define CHAT_ID "1220552570"
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

// Variabel Uang
int Red = 0;
int Green = 0;
int Blue = 0;
int Uang = 0;
bool statusUang = 0;
bool msg = 0;
int nominalUangTerakhir = 0;

unsigned long lastDetectionTime = 0;
unsigned long lastReadTime = 0;
unsigned long readInterval = 200;
bool solenoidActive = false;
unsigned long solenoidStartTime = 0;
unsigned long lastVibrationTime = 0;
const long vibrationCooldown = 5000; 


void setup() {
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(sensorOut, INPUT);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // inisial State Solenoid (Tertutup) 
  pinMode(SELENOID_PIN, OUTPUT);
  digitalWrite(SELENOID_PIN, HIGH);

  // Inisialisasi pin sensor getar sebagai INPUT_PULLUP
  pinMode(VIBRATION_SENSOR_PIN, INPUT_PULLUP); // SW-420 biasanya HIGH saat diam, LOW saat getar

  Serial.begin(115200);

  SPI.begin();           // Untuk RFID
  rfid.PCD_Init();
  Serial.println("RFID siap digunakan");

  Serial.print("Menghubungkan ke WiFi: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWiFi terhubung. IP: ");
  Serial.println(WiFi.localIP());

  configTime(0, 0, "pool.ntp.org");
  time_t now = time(nullptr);
  while (now < 24 * 3600) {
    Serial.print(".");
    delay(100);
    now = time(nullptr);
  }

  bot.sendMessage(CHAT_ID, "Bot Infaq Siap Digunakan ✅", "");

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("OLED gagal dimulai"));
    while (true);
  }

  display.display();
  delay(2000);
  display.clearDisplay();
  display.display();
}

void loop() {
  unsigned long currentMillis = millis();

  // RFID
  SPI.begin();           // Untuk RFID
  rfid.PCD_Init();


  // Judul OLED
  // display.clearDisplay();
  // display.setTextSize(2);
  // display.setTextColor(SSD1306_WHITE);
  // display.setCursor(0, 0);
  
  // display.println(F("Infaq Yuk!"));
  // display.display();
  // display.startscrollleft(0x00, 0x0F);

  

// Cek isi kotak amal
  long duration;
  float distance;

  // Kirim sinyal ultrasonic
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH);

  // Hitung jarak (dalam cm)
  distance = duration * 0.034 / 2;


// Hitung jarak (dalam cm)
  distance = duration * 0.034 / 2;

  // Tampilkan hasil ke Serial Monitor
  // Serial.print("Jarak: ");
  // Serial.print(distance);
  // Serial.println(" cm");

  // delay(1000);

// long distance = readUltrasonicDistance();

//  if (distance <= 3) {
//     display.println("Kotak amal sudah penuh");
//     display.display();
//     display.startscrollleft(0x00, 0x0F); // Scroll ke kiri seluruh baris
//     Serial.println("Kotak amal sudah penuh");
//   } else if (distance <= 6) {
//     display.println("Kotak amal hampir penuh");
//     display.display();
//     display.startscrollleft(0x00, 0x0F); // Scroll ke kiri seluruh baris
//     Serial.println("Kotak amal hampir penuh");
//   }

//   display.stopscroll(); // Hentikan scroll setelah delay


  // Baca sensor warna tiap 500ms
  if (currentMillis - lastReadTime >= readInterval) {
    Red = getRed();
    Green = getGreen();
    Blue = getBlue();
    lastReadTime = currentMillis;

    Serial.print("Red: "); Serial.print(Red);
    Serial.print("  Green: "); Serial.print(Green);
    Serial.print("  Blue: "); Serial.println(Blue);
  }

  // Deteksi uang
  if (statusUang == 0) {
    int nominal = 0;

    if (Red > 95 && Red < 105 && Green > 97 && Green < 112 && Blue > 100 && Blue < 112) {
      nominal = 1000;
    } else if (Red > 80 && Red < 90 && Green > 68 && Green < 77 && Blue > 52 && Blue < 62) {
      nominal = 2000;
    } else if (Red > 70 && Red < 80 && Green > 75 && Green < 100 && Blue > 80 && Blue < 100) {
      nominal = 5000;
    } else if (Red > 114 && Red < 124 && Green > 107 && Green < 117 && Blue > 85 && Blue < 100) {
      nominal = 10000;
    } else if (Red > 62 && Red < 72 && Green > 47 && Green < 57 && Blue > 47 && Blue < 57) {
      nominal = 20000;
    } else if (Red > 124 && Red < 134 && Green > 95 && Green < 105 && Blue > 73 && Blue < 83) {
      nominal = 50000;
    } else if (Red > 35 && Red < 50 && Green > 52 && Green < 62 && Blue > 39 && Blue < 49) {
      nominal = 100000;
    } 

    if (nominal > 0) {
      Uang += nominal;
      nominalUangTerakhir = nominal;
      statusUang = 1;
      msg = 0;
      lastDetectionTime = currentMillis;
    }
  }

  // Kirim pesan Telegram
  if (statusUang == 1 && msg == 0) {
    // display.clearDisplay();
    // display.setTextSize(2);
    // display.setCursor(5, 0);
    // display.println(F("Jazakllahu"));
    // display.setCursor(10, 15);
    // display.println(F("Khairan!"));
    // display.display();

    bot.sendMessage(CHAT_ID, "✅ Uang Terdeteksi! Sejumlah Rp " + String(nominalUangTerakhir) + "\nTotal Infaq: Rp " + String(Uang), "");
    msg = 1;
  }

  if (digitalRead(VIBRATION_SENSOR_PIN) == LOW) {
    if (currentMillis - lastVibrationTime >= vibrationCooldown) {
      Serial.println("Getaran terdeteksi!"); // Cetak ke Serial Monitor untuk debugging
      bot.sendMessage(CHAT_ID, "⚠ Terdeteksi KOtak Infaq Dimaling", ""); // Kirim notifikasi ke Telegram
      lastVibrationTime = currentMillis; // Update waktu terakhir getaran terdeteksi
    }
  }

  // Reset status setelah 5 detik
  if (statusUang == 1 && currentMillis - lastDetectionTime >= 5000) {
    statusUang = 0;
    msg = 0;
    Serial.println("Siap deteksi uang berikutnya.");
  }

    // RFID - Solenoid
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String uidStr = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      uidStr += String(rfid.uid.uidByte[i] < 0x10 ? "0" : ""); // Tambahkan leading zero 
      uidStr += String(rfid.uid.uidByte[i], HEX);
      if (i < rfid.uid.size - 1) uidStr += " ";
    }
    uidStr.toUpperCase();

    Serial.print("UID terbaca: ");
    Serial.println(uidStr);

    // Daftar UID yang valid
    String validUID1 = "04 26 35 4A FC 6E 80";
    String validUID2 = "A3 A9 C0 01";

    if (uidStr == validUID1 || uidStr == validUID2) {
      Serial.println("UID Valid - Membuka solenoid");
      digitalWrite(SELENOID_PIN, LOW); // Buka solenoid
      delay(5000);                      // Tunggu 5 detik
      digitalWrite(SELENOID_PIN, HIGH); // Tutup solenoid
      Serial.println("Solenoid DITUTUP");
    } else {
      Serial.println("UID tidak dikenali.");
    }

    // Hentikan komunikasi dengan kartu
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }
  tampilkanOLED(distance, nominalUangTerakhir, !digitalRead(SELENOID_PIN));
}

// Fungsi sensor warna
int getRed() {
  digitalWrite(S2, LOW);
  digitalWrite(S3, LOW);
  return pulseIn(sensorOut, LOW, 250);
}

int getGreen() {
  digitalWrite(S2, HIGH);
  digitalWrite(S3, HIGH);
  return pulseIn(sensorOut, LOW, 250);
}

int getBlue() {
  digitalWrite(S2, LOW);
  digitalWrite(S3, HIGH);
  return pulseIn(sensorOut, LOW, 250);
}

// Ultrasonic
long readUltrasonicDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  long distance = duration * 0.034 / 2; // Konversi ke cm
  return distance;
}

void tampilkanOLED(float distance, int nominal, bool solenoidStatus) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Baris 1: Judul
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Infaq Yuk!");

  // Baris 2: Ucapan jika nominal terdeteksi
  display.setCursor(0, 10);
  if (nominal > 0) {
    display.println("Terima kasih telah");
    display.setCursor(0, 20);
    display.println("berinfaq!");
  } else {
    display.println("Masukkan uang Anda");
  }

  // Baris 3: Nominal terakhir
  display.setCursor(0, 30);
  display.print("Nominal: Rp ");
  display.println(nominal);

  // Baris 4: Status kotak amal
  display.setCursor(0, 40);
  if (distance <= 3) {
    display.println("Kotak: PENUH");
  } else if (distance <= 6) {
    display.println("Kotak: Hampir penuh");
  } else {
    display.println("Kotak: Masih muat");
  }

  // Baris 5: Status kotak terbuka/tidak
  display.setCursor(0, 50);
  display.print("Kotak ");
  display.println(solenoidStatus ? "TERBUKA" : "TERTUTUP");

  display.display();
}
