#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <Wire.h>

// === PIN TANIMLARI ===
#define RST_PIN         9
#define SS_PIN          10
#define SERVO_KAPI_PIN  6
#define SERVO_GARAJ_PIN 5
#define PIR_KORIDOR     2
#define PIR_GARAJ       3
#define TEMP_PIN        A0
#define FAN_PIN         4


const int ledPins[8] = {22, 23, 24, 25, 26, 27, 28, 29};

// === NESNELER ===
LiquidCrystal_I2C lcd(0x27,16,2);
MFRC522 rfid(SS_PIN, RST_PIN);
Servo servoKapi;
Servo servoGaraj;

// === DURUM DEĞİŞKENLERİ ===
bool sistemAktif = false;
bool ledlerYandi = false;
bool fanCalisiyor = false;
bool fanBloke = false;
float sicaklikC = 0;
unsigned long ledAcikZamani = 0;
unsigned long sonSicaklikGuncelleme = 0;
unsigned long fanBlokeBaslangic = 0;
const unsigned long sicaklikGuncellemeAraligi = 1000;
const unsigned long fanBeklemeSuresi = 30000;
unsigned long rfidBaslangicZamani = 0;
bool fanDemoBitti = false;

// === SETUP ===
void setup() {

  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();
  Wire.begin(0x42);
  Wire.onReceive(veriGonderI2C);
  lcd.init();  
  lcd.backlight();
  lcd.begin(16, 2);
  lcd.print("Sistem Basliyor");
  delay(1500);
  lcd.clear();
  lcd.print("RFID Kart Bekleniyor");

  pinMode(PIR_KORIDOR, INPUT);
  pinMode(PIR_GARAJ, INPUT);
  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, LOW);

  for (int i = 0; i < 8; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }

  servoKapi.attach(SERVO_KAPI_PIN);
  servoGaraj.attach(SERVO_GARAJ_PIN);
  servoKapi.write(0);
  servoGaraj.write(0);
}

// === LOOP ===
void loop() {
  kartKontrol();
  if (sistemAktif) {
    hareketKontrolKoridor();
    hareketKontrolGaraj();
    sicaklikOkuVeGoster();
  }

  if (ledlerYandi && millis() - ledAcikZamani > 10000) {
    for (int i = 0; i < 8; i++) digitalWrite(ledPins[i], LOW);
    ledlerYandi = false;
    lcd.setCursor(0, 0);
    lcd.print("Koridor Isiklari ");
    lcd.setCursor(0, 1);
    lcd.print("Kapandi          ");
    delay(1000);
    lcd.clear();
  }
}

// === I2C VERİ ALMA ===
void veriGonderI2C() {
  String veri = "";
  while (Wire.available()) {
    char c = Wire.read();
    veri += c;
  }

  Serial.print("Gelen Komut: ");
  Serial.println(veri);

  if (veri == "K") servoKapi.write(90);
  else if (veri == "k") servoKapi.write(0);
  else if (veri == "G") servoGaraj.write(90);
  else if (veri == "g") servoGaraj.write(0);
else if (veri == "I" && !ledlerYandi) {
  for (int a = 0; a < 4; a++) {
    // 2'li grubu yak
    digitalWrite(ledPins[a * 2], HIGH);
    digitalWrite(ledPins[a * 2 + 1], HIGH);
    delay(80000);
  }

  ledlerYandi = true;
  ledAcikZamani = millis();
}

  else if (veri == "i") {
    for (int i = 0; i < 8; i++)
    digitalWrite(ledPins[i], LOW);
    ledlerYandi = false;
    }
  else if (veri == "F") {
    digitalWrite(FAN_PIN, HIGH);
    fanCalisiyor = true;
  }
  else if (veri == "f") {
    digitalWrite(FAN_PIN, LOW);
    fanCalisiyor = false;
  }
  else if (veri.startsWith("M:")) {
    String mesaj = veri.substring(2);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(mesaj.substring(0, 16));
    if (mesaj.length() > 16) {
      lcd.setCursor(0, 1);
      lcd.print(mesaj.substring(16, 32));
    }
  }
}

// === SICAKLIK OKUMA ===
void sicaklikOkuVeGoster() {
  if (!sistemAktif || fanDemoBitti) return;

  unsigned long gecenSure = millis() - rfidBaslangicZamani;
  float gosterilecekSicaklik = 28.0;

  if (gecenSure < 5000) {
    // Bekleme süresi
    lcd.setCursor(0, 0);
    lcd.print("Bekleniyor...    ");
    digitalWrite(FAN_PIN, LOW);
    fanCalisiyor = false;
    gosterilecekSicaklik = 28.0;  // Başlangıçta yüksek sıcaklık
  } 
  else if (gecenSure < 10000) {
    // Fan aktif süresi
    if (!fanCalisiyor) {
      digitalWrite(FAN_PIN, HIGH);
      fanCalisiyor = true;
      lcd.setCursor(0, 0);
      lcd.print("Fan Acildi       ");
    }
    gosterilecekSicaklik = 27.0; // Fan çalışırken biraz düşsün
  } 
  else {
    // Fan kapanır ve demo biter
    digitalWrite(FAN_PIN, LOW);
    fanCalisiyor = false;
    fanDemoBitti = true;
    lcd.setCursor(0, 0);
    lcd.print("Fan Kapandi      ");
    lcd.setCursor(0, 1);
    lcd.print("Sadece Blynk     ");
    gosterilecekSicaklik = 26.0; // Son durumda en düşük sıcaklık
  }

  // LCD sıcaklık göstergesi (ikinci satır)
  lcd.setCursor(0, 1);
  lcd.print("Sicaklik: ");
  lcd.print(gosterilecekSicaklik, 1);
  lcd.print((char)223);
  lcd.print("C   ");
}




// === RFID KART KONTROL ===
void kartKontrol() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

  sistemAktif = !sistemAktif;

  if (sistemAktif) {
    rfidBaslangicZamani = millis();
    fanDemoBitti = false;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Kart Okundu");
    lcd.setCursor(0, 1);
    lcd.print("Sistem Aktif");
    delay(1500);
    servoKapi.write(90);
    lcd.setCursor(0, 0);
    lcd.print("Kapi Aciliyor   ");
    delay(3000);
    servoKapi.write(0);
    lcd.setCursor(0, 0);
    lcd.print("Kapi Kapandi    ");
    delay(1000);
    lcd.clear();
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Kart Okundu");
    lcd.setCursor(0, 1);
    lcd.print("Sistem Kilitlendi");
    servoKapi.write(0);
    servoGaraj.write(0);
    digitalWrite(FAN_PIN, LOW);
    fanCalisiyor = false;
    fanBloke = false;
    for (int i = 0; i < 8; i++) digitalWrite(ledPins[i], LOW);
    ledlerYandi = false;
    delay(2000);
    lcd.clear();
    lcd.print("RFID Bekleniyor");
  }

  rfid.PICC_HaltA();
}

// === KORIDOR HAREKET ALGILAMA ===
void hareketKontrolKoridor() {
  if (digitalRead(PIR_KORIDOR) == HIGH && !ledlerYandi) {
    lcd.setCursor(0, 0);
    lcd.print("Koridor Hareket ");
    lcd.setCursor(0, 1);
    lcd.print("Isiklar Yanacak ");
    delay(500);

    for (int i = 0; i < 4; i++) {
      digitalWrite(ledPins[i * 2], HIGH);
      digitalWrite(ledPins[i * 2 + 1], HIGH);
      delay(1000);
    }

    ledlerYandi = true;
    ledAcikZamani = millis();
  }
}

// === GARAJ HAREKET ALGILAMA ===
void hareketKontrolGaraj() {
  if (digitalRead(PIR_GARAJ) == HIGH) {
    lcd.setCursor(0, 0);
    lcd.print("Garajda Hareket ");
    lcd.setCursor(0, 1);
    lcd.print("Kapak Aciliyor  ");
    servoGaraj.write(90);
    delay(3000);
    servoGaraj.write(0);
    lcd.setCursor(0, 0);
    lcd.print("Garaj Kapandi   ");
    delay(1000);
    lcd.clear();
  }
}
