// Liste recursive du contenu de la SD (sans ecran) - ESP32-S3-DevKitC-1
// Sortie sur Serial (USB CDC) ET Serial0 (UART0 / port COM)

#include <SPI.h>
#include <SD.h>

// --- Brochage (hardware/wiring.md) ---
#define SPI_SCK   8
#define SPI_MOSI  17
#define SPI_MISO  18
#define SD_CS     16
#define TFT_CS    11   // ecran non utilise, mais CS maintenu haut pour liberer le bus

uint32_t nbFichiers = 0, nbDossiers = 0, nbMp3 = 0;
uint64_t octetsTotal = 0;

void ecrire(const String &txt) {
  Serial.println(txt);
  Serial0.println(txt);
}

String taille(uint64_t o) {
  if (o >= 1024 * 1024) return String(o / 1048576.0, 1) + " Mo";
  if (o >= 1024) return String(o / 1024.0, 1) + " Ko";
  return String((uint32_t)o) + " o";
}

void lister(const String &chemin, int niveau) {
  File dir = SD.open(chemin);
  if (!dir) { ecrire("ECHEC ouverture " + chemin); return; }
  String indent = "";
  for (int i = 0; i < niveau; i++) indent += "  ";

  File f = dir.openNextFile();
  while (f) {
    String nom = f.name();
    if (f.isDirectory()) {
      nbDossiers++;
      ecrire(indent + "[D] " + nom);
      String sous = chemin == "/" ? "/" + nom : chemin + "/" + nom;
      f.close();
      lister(sous, niveau + 1);
    } else {
      nbFichiers++;
      octetsTotal += f.size();
      String bas = nom; bas.toLowerCase();
      if (bas.endsWith(".mp3")) nbMp3++;
      ecrire(indent + "    " + nom + "  (" + taille(f.size()) + ")");
      f.close();
    }
    f = dir.openNextFile();
  }
  dir.close();
}

void setup() {
  Serial.begin(115200);
  Serial0.begin(115200);
  delay(2000);
  ecrire("\n=== Contenu de la SD ===");

  pinMode(TFT_CS, OUTPUT); digitalWrite(TFT_CS, HIGH);
  pinMode(SD_CS, OUTPUT);  digitalWrite(SD_CS, HIGH);
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

  if (!SD.begin(SD_CS, SPI, 20000000)) {
    ecrire("SD : ECHEC montage");
    return;
  }

  lister("/", 0);
  ecrire("---");
  ecrire(String(nbDossiers) + " dossier(s), " + String(nbFichiers) + " fichier(s) dont "
         + String(nbMp3) + " MP3, " + taille(octetsTotal) + " au total");
  ecrire("=== Fin ===");
}

void loop() {}
