// Test ecran ILI9341 2.4" + lecteur SD du module - ESP32-S3-DevKitC-1
// Cablage sur breadboard / fils dupont (memes broches que le futur PCB)
// Librairies a installer : "Adafruit ILI9341" (+ "Adafruit GFX" proposee automatiquement), "TJpg_Decoder"

#include <SPI.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <TJpg_Decoder.h>

// --- Brochage (identique au PCB v_final1) ---
#define SPI_SCK   8
#define SPI_MOSI  17
#define SPI_MISO  18   // uniquement vers SD_MISO (SDO de l'ecran NON branche)
#define SD_CS     16
#define TFT_CS    11
#define TFT_DC    9
#define TFT_RST   10

Adafruit_ILI9341 tft(&SPI, TFT_DC, TFT_CS, TFT_RST);

int ligne = 0;
void ecrire(const String &txt, uint16_t couleur = ILI9341_WHITE) {
  Serial.println(txt);
  tft.setTextColor(couleur);
  tft.setCursor(4, 4 + ligne * 12);
  tft.print(txt);
  ligne++;
  if (ligne > 25) { ligne = 0; delay(2000); tft.fillScreen(ILI9341_BLACK); }
}

// lit un entier little-endian dans un en-tete BMP
uint32_t lire32(const uint8_t *b) { return b[0] | (b[1] << 8) | (b[2] << 16) | ((uint32_t)b[3] << 24); }

// affiche un BMP 24 ou 16 bits non compresse, agrandi ou reduit pour remplir 240x320, centre
bool afficherBmp(const String &chemin) {
  File f = SD.open(chemin);
  if (!f) return false;
  uint8_t e[54];
  if (f.read(e, 54) != 54 || e[0] != 'B' || e[1] != 'M') { Serial.println("BMP : en-tete invalide"); return false; }
  uint32_t debut = lire32(e + 10);
  int32_t w = (int32_t)lire32(e + 18), h = (int32_t)lire32(e + 22);
  uint16_t bpp = e[28] | (e[29] << 8);
  uint32_t compression = lire32(e + 30);
  bool basEnHaut = h > 0;   // BMP classique : lignes stockees du bas vers le haut
  if (h < 0) h = -h;
  Serial.printf("BMP : %dx%d, %u bits, compression %u\n", w, h, bpp, compression);
  if (!((bpp == 24 && compression == 0) || (bpp == 16 && (compression == 0 || compression == 3)))) {
    Serial.println("BMP : format non gere (il faut 24 bits ou 16 bits non compresse)");
    return false;
  }

  // mise a l'echelle : la plus grande taille qui tient dans 240x320 (garde les proportions)
  int wa = 240, ha = (int32_t)h * 240 / w;
  if (ha > 320) { ha = 320; wa = (int32_t)w * 320 / h; }
  int x0 = (240 - wa) / 2, y0 = (320 - ha) / 2;
  uint32_t tailleLigne = ((w * bpp + 31) / 32) * 4;
  uint8_t *src = (uint8_t *)malloc(tailleLigne);
  uint16_t *dst = (uint16_t *)malloc(wa * 2);
  if (!src || !dst) { free(src); free(dst); return false; }
  Serial.printf("Affichage %dx%d\n", wa, ha);

  tft.fillScreen(ILI9341_BLACK);
  int syPrec = -1;
  for (int oy = 0; oy < ha; oy++) {
    int sy = (int32_t)oy * h / ha;    // ligne source correspondante
    if (sy != syPrec) {               // relire seulement si la ligne source change
      int ligneFichier = basEnHaut ? (h - 1 - sy) : sy;
      f.seek(debut + (uint32_t)ligneFichier * tailleLigne);
      f.read(src, tailleLigne);
      syPrec = sy;
    }
    for (int ox = 0; ox < wa; ox++) {
      int sx = (int32_t)ox * w / wa;
      if (bpp == 24) {
        uint8_t *p = src + sx * 3;          // ordre B, G, R
        dst[ox] = ((p[2] & 0xF8) << 8) | ((p[1] & 0xFC) << 3) | (p[0] >> 3);
      } else {
        uint16_t v = src[sx * 2] | (src[sx * 2 + 1] << 8);
        dst[ox] = (compression == 3) ? v    // deja en RGB565
                : ((v & 0x7C00) << 1) | ((v & 0x03E0) << 1) | (v & 0x001F);  // RGB555 -> 565
      }
    }
    tft.drawRGBBitmap(x0, y0 + oy, dst, wa, 1);
  }
  free(src); free(dst);
  f.close();
  return true;
}

// appelee par le decodeur JPEG pour chaque bloc de pixels decode
bool dessinerBloc(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
  if (y >= tft.height()) return false;
  tft.drawRGBBitmap(x, y, bitmap, w, h);
  return true;
}

bool afficherJpg(const String &chemin) {
  uint16_t w = 0, h = 0;
  TJpgDec.getFsJpgSize(&w, &h, chemin, SD);
  // reduction 1, 2, 4 ou 8 pour tenir dans 240x320
  uint8_t echelle = 1;
  while (echelle < 8 && (w / echelle > 240 || h / echelle > 320)) echelle *= 2;
  TJpgDec.setJpgScale(echelle);
  TJpgDec.setCallback(dessinerBloc);
  Serial.printf("JPG : %ux%u (reduction 1/%u)\n", w, h, echelle);
  tft.fillScreen(ILI9341_BLACK);
  JRESULT r = TJpgDec.drawFsJpg((240 - w / echelle) / 2, (320 - h / echelle) / 2, chemin, SD);
  if (r != JDR_OK) Serial.printf("JPG : echec decodage (code %d) - JPEG progressif ?\n", r);
  return r == JDR_OK;
}

// affiche la premiere image (.bmp ou .jpg) trouvee dans /covers
void afficherCover() {
  File dir = SD.open("/covers");
  if (!dir) { Serial.println("Pas de dossier /covers"); return; }
  String chemin = "";
  File f = dir.openNextFile();
  while (f) {
    String nom = f.name();
    Serial.println("covers/" + nom);
    nom.toLowerCase();
    if (chemin == "" && !f.isDirectory() &&
        (nom.endsWith(".bmp") || nom.endsWith(".jpg") || nom.endsWith(".jpeg")))
      chemin = String("/covers/") + f.name();
    f = dir.openNextFile();
  }
  if (chemin == "") { Serial.println("Aucune image dans /covers"); return; }

  Serial.println("Cover : " + chemin);
  String ext = chemin; ext.toLowerCase();
  bool ok = ext.endsWith(".bmp") ? afficherBmp(chemin) : afficherJpg(chemin);
  if (!ok) { ligne = 0; tft.fillScreen(ILI9341_BLACK); ecrire("Cover : echec affichage", ILI9341_RED); }
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("\n=== Test ecran + SD ===");

  // les 2 CS au repos (haut) avant tout
  pinMode(TFT_CS, OUTPUT); digitalWrite(TFT_CS, HIGH);
  pinMode(SD_CS, OUTPUT);  digitalWrite(SD_CS, HIGH);

  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

  // --- Ecran ---
  tft.begin(40000000);
  tft.setRotation(0);            // portrait 240x320
  tft.fillScreen(ILI9341_RED);   delay(300);
  tft.fillScreen(ILI9341_GREEN); delay(300);
  tft.fillScreen(ILI9341_BLUE);  delay(300);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextSize(1);
  ecrire("Ecran OK", ILI9341_GREEN);

  // --- SD ---
  if (!SD.begin(SD_CS, SPI, 20000000)) {
    ecrire("SD : ECHEC montage", ILI9341_RED);
    ecrire("Carte FAT32 ? bien inseree ?", ILI9341_RED);
    return;
  }
  uint64_t taille = SD.cardSize() / (1024 * 1024);
  ecrire("SD OK - " + String((uint32_t)taille) + " Mo", ILI9341_GREEN);

  // lister la racine
  File root = SD.open("/");
  File f = root.openNextFile();
  int n = 0;
  while (f) {
    String nom = String(f.isDirectory() ? "[D] " : "    ") + f.name();
    if (!f.isDirectory()) nom += "  " + String((uint32_t)f.size()) + " o";
    ecrire(nom);
    n++;
    f = root.openNextFile();
  }
  ecrire(String(n) + " element(s)", ILI9341_YELLOW);

  delay(3000);
  afficherCover();
}

void loop() {}
