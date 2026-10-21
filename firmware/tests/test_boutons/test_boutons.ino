// Test des 6 boutons avec anti-rebond - ESP32-S3-DevKitC-1
// Boutons vers GND, pull-up interne (actif bas)
// Affiche le bouton appuye / relache a l'ecran et dans le moniteur serie

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// --- Brochage (identique au PCB v_final1) ---
#define SPI_SCK   8
#define SPI_MOSI  17
#define SPI_MISO  18
#define SD_CS     16
#define TFT_CS    11
#define TFT_DC    9
#define TFT_RST   10

#define BTN_MENU     42
#define BTN_PLAY     41
#define BTN_PREV     40
#define BTN_NEXT     39
#define BTN_VOLUP    38
#define BTN_VOLDOWN  1

#define ANTI_REBOND_MS 30   // l'etat doit rester stable 30 ms pour etre pris en compte

Adafruit_ILI9341 tft(&SPI, TFT_DC, TFT_CS, TFT_RST);

struct Bouton {
  uint8_t broche;
  const char *nom;
  bool etat;            // etat valide (true = appuye)
  bool lecture;         // derniere lecture brute
  uint32_t changement;  // moment du dernier changement brut
  uint32_t compteur;    // nombre d'appuis valides
};

Bouton boutons[] = {
  {BTN_MENU,    "MENU"},
  {BTN_PLAY,    "PLAY"},
  {BTN_PREV,    "PREV"},
  {BTN_NEXT,    "NEXT"},
  {BTN_VOLUP,   "VOL+"},
  {BTN_VOLDOWN, "VOL-"},
};
const int NB = sizeof(boutons) / sizeof(boutons[0]);

void log(const String &txt) {
  Serial.println(txt);
#if ARDUINO_USB_CDC_ON_BOOT
  Serial0.println(txt);   // aussi sur le port "COM" (UART0)
#endif
}

// une ligne par bouton : nom, etat, nombre d'appuis
void dessinerBouton(int i) {
  Bouton &b = boutons[i];
  int y = 60 + i * 40;
  tft.fillRect(0, y, 240, 36, b.etat ? ILI9341_GREEN : ILI9341_BLACK);
  tft.drawRect(0, y, 240, 36, ILI9341_DARKGREY);
  tft.setTextSize(3);
  tft.setTextColor(b.etat ? ILI9341_BLACK : ILI9341_WHITE);
  tft.setCursor(10, y + 7);
  tft.print(b.nom);
  tft.setTextSize(2);
  tft.setCursor(140, y + 11);
  tft.print("x");
  tft.print(b.compteur);
}

void dessinerDernier(const char *nom) {
  tft.fillRect(0, 0, 240, 50, ILI9341_NAVY);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 6);
  tft.print("Dernier appui :");
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_YELLOW);
  tft.setCursor(10, 28);
  tft.print(nom);
}

void setup() {
  Serial.begin(115200);
#if ARDUINO_USB_CDC_ON_BOOT
  Serial0.begin(115200);
#endif
  delay(1500);
  log("\n=== Test boutons ===");

  pinMode(SD_CS, OUTPUT); digitalWrite(SD_CS, HIGH);   // SD inactive sur le bus
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  tft.begin(40000000);
  tft.setRotation(0);
  tft.fillScreen(ILI9341_BLACK);
  dessinerDernier("-");

  for (int i = 0; i < NB; i++) {
    pinMode(boutons[i].broche, INPUT_PULLUP);
    boutons[i].lecture = boutons[i].etat = (digitalRead(boutons[i].broche) == LOW);
    boutons[i].changement = millis();
    if (boutons[i].etat) log(String("ATTENTION : ") + boutons[i].nom + " deja appuye au demarrage (court-circuit ?)");
    dessinerBouton(i);
  }
  log("Appuie sur les boutons...");
}

void loop() {
  uint32_t maintenant = millis();
  for (int i = 0; i < NB; i++) {
    Bouton &b = boutons[i];
    bool lu = (digitalRead(b.broche) == LOW);
    if (lu != b.lecture) {            // ca bouge : on relance le chrono
      b.lecture = lu;
      b.changement = maintenant;
    } else if (lu != b.etat && maintenant - b.changement >= ANTI_REBOND_MS) {
      b.etat = lu;                    // stable assez longtemps : changement valide
      if (b.etat) {
        b.compteur++;
        log(String(b.nom) + " appuye (GPIO " + b.broche + ")  x" + b.compteur);
        dessinerDernier(b.nom);
      } else {
        log(String(b.nom) + " relache");
      }
      dessinerBouton(i);
    }
  }
}
