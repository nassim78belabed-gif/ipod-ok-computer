/*
  iPod "OK Computer Edition" - TEST SD + ECRAN (bus SPI partage)
  Carte : ESP32-S3-DevKitC-1 N16R8, ILI9341 + SD du module, PCB de test V7.
  Aucune librairie a installer (SPI.h, SD.h fournis avec le core ESP32).

  Reglages Arduino IDE (carte "ESP32S3 Dev Module") :
    USB CDC On Boot : Enabled
    Flash Size      : 16MB (128Mb)
    PSRAM           : OPI PSRAM        <-- N16R8 = PSRAM octale
    Partition       : Default 4MB with spiffs (ou 16MB Flash)
  Moniteur serie : 115200
*/
#include <SPI.h>
#include <SD.h>
#include "pins.h"

SPIClass spi(FSPI);          // un seul bus pour l'ecran ET la SD (comme sur le PCB)
uint32_t okCount = 0, koCount = 0;

// ---------------------------------------------------------------- ecran ILI9341 (sans librairie)
#define TFT_FREQ 20000000
static inline void tftBegin() { spi.beginTransaction(SPISettings(TFT_FREQ, MSBFIRST, SPI_MODE0)); digitalWrite(TFT_CS, LOW); }
static inline void tftEnd()   { digitalWrite(TFT_CS, HIGH); spi.endTransaction(); }
void tftCmd(uint8_t c)  { digitalWrite(TFT_DC, LOW);  tftBegin(); spi.transfer(c); tftEnd(); }
void tftDat(uint8_t d)  { digitalWrite(TFT_DC, HIGH); tftBegin(); spi.transfer(d); tftEnd(); }
void tftCmdD(uint8_t c, std::initializer_list<uint8_t> d) { tftCmd(c); for (auto v : d) tftDat(v); }

void tftInit() {
  digitalWrite(TFT_RST, LOW); delay(20); digitalWrite(TFT_RST, HIGH); delay(150);
  tftCmd(0x01); delay(150);                 // reset logiciel
  tftCmd(0x11); delay(120);                 // sortie de veille
  tftCmdD(0x3A, {0x55});                    // 16 bits/pixel
  tftCmdD(0x36, {0x48});                    // orientation
  tftCmd(0x29); delay(20);                  // affichage ON
}
void tftFill(uint16_t color) {
  tftCmdD(0x2A, {0, 0, 0, 239});            // colonnes 0..239
  tftCmdD(0x2B, {0, 0, 1, 63});             // lignes 0..319
  tftCmd(0x2C);
  digitalWrite(TFT_DC, HIGH);
  static uint8_t line[480];
  for (int i = 0; i < 240; i++) { line[2 * i] = color >> 8; line[2 * i + 1] = color & 0xFF; }
  tftBegin();
  for (int y = 0; y < 320; y++) spi.writeBytes(line, 480);
  tftEnd();
}

// ---------------------------------------------------------------- sonde SD "brute" (diagnostic)
uint8_t sdCmd(uint8_t cmd, uint32_t arg, uint8_t crc, uint8_t *extra = nullptr) {
  digitalWrite(SD_CS, LOW);
  spi.transfer(0xFF);
  spi.transfer(0x40 | cmd);
  spi.transfer(arg >> 24); spi.transfer(arg >> 16); spi.transfer(arg >> 8); spi.transfer(arg); spi.transfer(crc);
  uint8_t r = 0xFF;
  for (int i = 0; i < 16 && r == 0xFF; i++) r = spi.transfer(0xFF);
  if (extra) for (int i = 0; i < 4; i++) extra[i] = spi.transfer(0xFF);
  digitalWrite(SD_CS, HIGH); spi.transfer(0xFF);
  return r;
}
void sdRawProbe() {
  Serial.println("\n--- SONDE BRUTE SD (400 kHz) ---");
  pinMode(SPI_MISO, INPUT);
  Serial.printf("MISO au repos (CS SD et TFT a HIGH) : %s  (attendu: HIGH grace a la resistance de rappel)\n",
                digitalRead(SPI_MISO) ? "HIGH" : "LOW  <-- SUSPECT: court-circuit / SDO ecran branche ?");
  spi.beginTransaction(SPISettings(400000, MSBFIRST, SPI_MODE0));
  digitalWrite(SD_CS, HIGH); digitalWrite(TFT_CS, HIGH);
  for (int i = 0; i < 10; i++) spi.transfer(0xFF);                  // 80 impulsions d'horloge
  uint8_t r0 = sdCmd(0, 0, 0x95);
  uint8_t e[4] = {0};
  uint8_t r8 = sdCmd(8, 0x1AA, 0x87, e);
  spi.endTransaction();
  Serial.printf("CMD0 -> 0x%02X  (attendu 0x01)\n", r0);
  Serial.printf("CMD8 -> 0x%02X  echo %02X %02X %02X %02X  (attendu 0x01 puis 00 00 01 AA)\n", r8, e[0], e[1], e[2], e[3]);
  if (r0 == 0xFF) Serial.println("=> 0xFF partout : la carte ne repond pas. Verifier 3V3, GND, CS, MISO, carte enfoncee, SD_CS/MISO non inverses.");
  else if (r0 == 0x01) Serial.println("=> La carte repond : le cablage SD est bon.");
}

// ---------------------------------------------------------------- test SD avec la librairie
bool sdTest(uint32_t hz) {
  SD.end();
  if (!SD.begin(SD_CS, spi, hz)) { Serial.printf("SD.begin @%lu Hz : ECHEC\n", (unsigned long)hz); return false; }
  uint8_t t = SD.cardType();
  Serial.printf("SD.begin @%lu Hz : OK  type=%s  taille=%llu Mo\n", (unsigned long)hz,
                t == CARD_MMC ? "MMC" : t == CARD_SD ? "SDSC" : t == CARD_SDHC ? "SDHC/SDXC" : "?", SD.cardSize() / (1024ULL * 1024ULL));
  File f = SD.open("/test_ipod.txt", FILE_WRITE);
  if (!f) { Serial.println("  ecriture: impossible d'ouvrir /test_ipod.txt"); return false; }
  f.printf("iPod OK Computer - test %lu\n", (unsigned long)millis()); f.close();
  f = SD.open("/test_ipod.txt");
  if (!f) { Serial.println("  lecture: echec"); return false; }
  String s = f.readStringUntil('\n'); f.close();
  Serial.printf("  relu: \"%s\"\n", s.c_str());
  return true;
}
void listRoot() {
  File root = SD.open("/");
  if (!root) return;
  Serial.println("  Contenu de la racine :");
  for (File e = root.openNextFile(); e; e = root.openNextFile())
    Serial.printf("   %s %-32s %10u o\n", e.isDirectory() ? "[D]" : "   ", e.name(), (unsigned)e.size());
}
void speedTest() {
  File f = SD.open("/speed.bin", FILE_WRITE);
  if (!f) return;
  static uint8_t buf[4096]; memset(buf, 0xA5, sizeof buf);
  uint32_t t0 = millis();
  for (int i = 0; i < 256; i++) f.write(buf, sizeof buf);       // 1 Mo
  f.close();
  uint32_t dt = millis() - t0;
  Serial.printf("  ecriture 1 Mo : %lu ms (%.0f ko/s)\n", (unsigned long)dt, 1024.0 * 1000.0 / dt);
  f = SD.open("/speed.bin"); t0 = millis();
  while (f.read(buf, sizeof buf) > 0) {}
  dt = millis() - t0; f.close();
  Serial.printf("  lecture  1 Mo : %lu ms (%.0f ko/s)\n", (unsigned long)dt, 1024.0 * 1000.0 / dt);
  SD.remove("/speed.bin");
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("\n=== iPod OK Computer : test SD + ecran (bus partage) ===");
  // 1) TOUS les chip select a HIGH avant de toucher au bus
  pinMode(SD_CS, OUTPUT);  digitalWrite(SD_CS, HIGH);
  pinMode(TFT_CS, OUTPUT); digitalWrite(TFT_CS, HIGH);
  pinMode(TFT_DC, OUTPUT); pinMode(TFT_RST, OUTPUT); digitalWrite(TFT_RST, HIGH);
  spi.begin(SPI_SCK, SPI_MISO, SPI_MOSI, -1);

  sdRawProbe();
  Serial.println("\n--- TEST SD (librairie) ---");
  bool ok = false;
  const uint32_t freqs[] = {400000, 4000000, 10000000, 20000000};
  for (uint32_t f : freqs) { if (sdTest(f)) { ok = true; } else if (ok) break; }
  if (ok) { listRoot(); speedTest(); }

  Serial.println("\n--- TEST ECRAN ---");
  tftInit();
  tftFill(0xF800);                                        // rouge
  Serial.println("Ecran : rouge affiche (si rien : verifier VCC/LED, CS, RST, DC).");
}

void loop() {
  // Preuve du partage de bus : on alterne ecran et SD en boucle.
  static const uint16_t colors[] = {0xF800, 0x07E0, 0x001F, 0xFFFF};
  static int i = 0;
  tftFill(colors[i++ & 3]);
  File f = SD.open("/test_ipod.txt");
  bool ok = f && f.available();
  if (f) f.close();
  ok ? okCount++ : koCount++;
  Serial.printf("cycle %d : ecran OK, lecture SD %s  (ok=%lu ko=%lu)\n", i, ok ? "OK" : "ECHEC",
                (unsigned long)okCount, (unsigned long)koCount);
  delay(1500);
}
