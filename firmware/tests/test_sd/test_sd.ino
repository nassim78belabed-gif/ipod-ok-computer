// Test lecteur SD seul (sans ecran) - ESP32-S3-DevKitC-1
// Montage, taille, liste de la racine, ecriture puis relecture de /test.txt
// Sortie sur Serial (USB CDC) ET Serial0 (UART0 / port COM), pour lire sur l'un ou l'autre port

#include <SPI.h>
#include <SD.h>

// --- Brochage (hardware/wiring.md) ---
#define SPI_SCK   8
#define SPI_MOSI  17
#define SPI_MISO  18
#define SD_CS     16
#define TFT_CS    11   // ecran non utilise, mais CS maintenu haut pour liberer le bus

void ecrire(const String &txt) {
  Serial.println(txt);
  Serial0.println(txt);
}

void setup() {
  Serial.begin(115200);
  Serial0.begin(115200);
  delay(2000);
  ecrire("\n=== Test SD ===");

  pinMode(TFT_CS, OUTPUT); digitalWrite(TFT_CS, HIGH);
  pinMode(SD_CS, OUTPUT);  digitalWrite(SD_CS, HIGH);

  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

  // --- Montage ---
  if (!SD.begin(SD_CS, SPI, 20000000)) {
    ecrire("SD : ECHEC montage (carte absente, pas FAT32, ou cablage)");
    return;
  }
  const char *types[] = {"AUCUNE", "MMC", "SDSC", "SDHC/SDXC"};
  uint8_t type = SD.cardType();
  ecrire("SD : montage OK, type " + String(type < 4 ? types[type] : "INCONNU"));

  // --- Taille ---
  ecrire("Taille carte : " + String((uint32_t)(SD.cardSize() / (1024 * 1024))) + " Mo");
  ecrire("Volume       : " + String((uint32_t)(SD.totalBytes() / (1024 * 1024))) + " Mo, utilise "
         + String((uint32_t)(SD.usedBytes() / (1024 * 1024))) + " Mo");

  // --- Liste de la racine ---
  ecrire("Racine :");
  File root = SD.open("/");
  File f = root.openNextFile();
  int n = 0;
  while (f) {
    String nom = String(f.isDirectory() ? "  [D] " : "      ") + f.name();
    if (!f.isDirectory()) nom += "  " + String((uint32_t)f.size()) + " o";
    ecrire(nom);
    n++;
    f = root.openNextFile();
  }
  root.close();
  ecrire(String(n) + " element(s)");

  // --- Ecriture puis relecture ---
  String contenu = "iPod OK Computer - test SD, millis=" + String(millis());
  File w = SD.open("/test.txt", FILE_WRITE);
  if (!w) {
    ecrire("Ecriture : ECHEC ouverture /test.txt");
    return;
  }
  w.print(contenu);
  w.close();
  ecrire("Ecriture : " + contenu);

  File r = SD.open("/test.txt", FILE_READ);
  if (!r) {
    ecrire("Relecture : ECHEC ouverture /test.txt");
    return;
  }
  String lu = r.readString();
  r.close();
  ecrire("Relecture : " + lu);
  ecrire(lu == contenu ? "Ecriture/relecture : OK" : "Ecriture/relecture : DIFFERENT");
}

void loop() {}
