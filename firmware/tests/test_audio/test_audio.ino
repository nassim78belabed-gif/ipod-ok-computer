// Test audio : lecture des MP3 de /Music (SD) vers le DAC PCM5102A (CJMCU-5102) en I2S
// ESP32-S3-DevKitC-1 N16R8 - librairies "ESP32-audioI2S-master" (schreibfaux), "Adafruit ILI9341"
// Joue le premier MP3 trouve, puis passe au suivant a la fin de chaque morceau
// Ecran : titre, artiste, album, numero du morceau, format, temps et barre de progression
// Commandes : boutons PLAY (pause), PREV, NEXT, VOL+, VOL-
//             ou touches dans le moniteur serie : p = pause, < = precedent, > = suivant, + / - = volume,
//             f = saut a 10 s de la fin (test d'enchainement)

#include <SPI.h>
#include <SD.h>
#include <vector>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "Audio.h"

// --- Brochage (hardware/wiring.md) ---
#define SPI_SCK   8
#define SPI_MOSI  17
#define SPI_MISO  18
#define SD_CS     16
#define TFT_CS    11
#define TFT_DC    9
#define TFT_RST   10

#define I2S_BCK   7
#define I2S_DIN   6
#define I2S_LRCK  5

#define VOLUME    10   // 0 a 21 : on commence bas pour ne pas saturer

// Boutons (actif bas, INPUT_PULLUP)
#define BTN_MENU     42
#define BTN_PLAY     41
#define BTN_PREV     40
#define BTN_NEXT     39
#define BTN_VOLUP    38
#define BTN_VOLDOWN  1
#define ANTI_REBOND_MS 30

Audio audio;
Adafruit_ILI9341 tft(&SPI, TFT_DC, TFT_CS, TFT_RST);
std::vector<String> morceaux;
int courant = 0;

// infos du morceau en cours
String titre, artiste, album, format;
uint32_t echantillonnage = 0, debit = 0;
bool infosChangees = false;
volatile bool finMorceau = false;   // mis a true par la librairie a la fin d'un morceau

void log(const String &txt) {
  Serial.println(txt);
#if ARDUINO_USB_CDC_ON_BOOT
  Serial0.println(txt);   // aussi sur le port "COM" (UART0)
#endif
}

// la police de l'ecran n'a pas d'accents : on remplace les lettres accentuees (UTF-8) par la lettre simple
String sansAccents(const String &s) {
  static const char *de[] = {"é","è","ê","ë","à","â","ä","î","ï","ô","ö","ù","û","ü","ç","É","È","À","Ç","’"};
  static const char *vers[] = {"e","e","e","e","a","a","a","i","i","o","o","u","u","u","c","E","E","A","C","'"};
  String r = s;
  for (int i = 0; i < 20; i++) r.replace(de[i], vers[i]);
  String propre;
  for (unsigned i = 0; i < r.length(); i++) if ((uint8_t)r[i] < 0x80) propre += r[i];   // autres caracteres ignores
  return propre;
}

// coupe un texte trop long pour la largeur de l'ecran
String couper(const String &s, int taille) {
  int maxCar = 228 / (6 * taille);
  return (int)s.length() <= maxCar ? s : s.substring(0, maxCar - 2) + "..";
}

// cherche les .mp3 dans un dossier et ses sous-dossiers
void chercherMp3(const String &dossier) {
  File dir = SD.open(dossier);
  if (!dir) return;
  File f = dir.openNextFile();
  while (f) {
    String chemin = dossier + "/" + f.name();
    if (f.isDirectory()) chercherMp3(chemin);
    else {
      String nom = f.name(); nom.toLowerCase();
      if (nom.endsWith(".mp3")) morceaux.push_back(chemin);
    }
    f = dir.openNextFile();
  }
}

// --- Ecran ---
void dessinerInfos() {
  tft.fillScreen(ILI9341_BLACK);
  tft.fillRect(0, 0, 240, 24, ILI9341_NAVY);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(6, 5);
  tft.print("En lecture " + String(courant + 1) + "/" + String(morceaux.size()));

  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(6, 50);  tft.print(couper(sansAccents(titre), 2));
  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(6, 80);  tft.print(couper(sansAccents(artiste), 2));
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_LIGHTGREY);
  tft.setCursor(6, 108); tft.print(couper(sansAccents(album), 1));

  tft.setTextColor(ILI9341_DARKGREY);
  tft.setCursor(6, 140);
  tft.print(format);
  if (echantillonnage) tft.print("  " + String(echantillonnage / 1000.0, 1) + " kHz");
  if (debit) tft.print("  " + String(debit / 1000) + " kb/s");

  tft.setTextColor(ILI9341_YELLOW);
  tft.setCursor(6, 300);
  tft.print("Volume " + String(audio.getVolume()) + "/21");
}

void dessinerProgression() {
  uint32_t t = audio.getAudioCurrentTime(), d = audio.getAudioFileDuration();
  tft.drawRect(6, 200, 228, 10, ILI9341_WHITE);
  int plein = d ? (int)(226L * t / d) : 0;
  tft.fillRect(7, 201, plein, 8, ILI9341_GREEN);
  tft.fillRect(7 + plein, 201, 226 - plein, 8, ILI9341_BLACK);

  char txt[24];
  snprintf(txt, sizeof(txt), "%u:%02u / %u:%02u", t / 60, t % 60, d / 60, d % 60);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);   // fond noir : efface l'ancien texte
  tft.setCursor(6, 220);
  tft.print(txt);
  tft.print("  ");
}

void jouer(int i) {
  courant = i % morceaux.size();
  log("\n>>> Lecture " + String(courant + 1) + "/" + String(morceaux.size()) + " : " + morceaux[courant]);
  // valeurs par defaut si le MP3 n'a pas de tags : nom du fichier et du dossier
  String c = morceaux[courant];
  titre = c.substring(c.lastIndexOf('/') + 1);
  titre.replace(".mp3", ""); titre.replace(".MP3", "");
  String dossier = c.substring(0, c.lastIndexOf('/'));
  artiste = dossier.substring(dossier.lastIndexOf('/') + 1);
  album = ""; format = ""; echantillonnage = 0; debit = 0;
  infosChangees = true;
  if (!audio.connecttoFS(SD, morceaux[courant].c_str())) log("ECHEC ouverture du fichier");
}

// recupere la valeur apres "Nom: " dans un message de la librairie
bool valeur(const String &msg, const char *cle, String &sortie) {
  if (!msg.startsWith(cle)) return false;
  sortie = msg.substring(strlen(cle));
  sortie.trim();
  return true;
}

// messages de la librairie (format, debit, tags, fin de morceau...)
void infoAudio(Audio::msg_t m) {
  if (m.e == Audio::evt_log) return;   // trop bavard
  String msg = m.msg ? m.msg : "";
  log(String("[") + Audio::eventStr[m.e] + "] " + msg);
  String v;
  if (m.e == Audio::evt_id3data) {
    if (valeur(msg, "Title:", v)) titre = v;
    else if (valeur(msg, "Artist:", v)) artiste = v;
    else if (valeur(msg, "Album:", v)) album = v;
    else return;
    infosChangees = true;
  } else if (m.e == Audio::evt_info) {
    if (valeur(msg, "SampleRate (Hz):", v)) { echantillonnage = v.toInt(); infosChangees = true; }
    else if (msg.startsWith("MPEG")) { format = msg; infosChangees = true; }
  } else if (m.e == Audio::evt_bitrate) {
    debit = msg.toInt(); infosChangees = true;
  } else if (m.e == Audio::evt_eof) {
    finMorceau = true;   // pas de jouer() ici : la librairie n'a pas fini de fermer le fichier (blocage)
  }
}

void setup() {
  Serial.begin(115200);
#if ARDUINO_USB_CDC_ON_BOOT
  Serial0.begin(115200);
#endif
  delay(1500);
  log("\n=== Test audio ===");
  // cause du dernier redemarrage (3 = reset logiciel, 4 = plantage, 9 = chute de tension "brownout")
  static const char *causes[] = {"inconnue", "mise sous tension", "broche externe", "logiciel", "plantage (panic)",
                                 "watchdog int", "watchdog tache", "watchdog", "sortie de veille", "BROWNOUT (chute de tension)", "SDIO"};
  esp_reset_reason_t r = esp_reset_reason();
  log(String("Cause du demarrage : ") + (r <= 10 ? causes[r] : "?") + " (" + String((int)r) + ")");

  pinMode(TFT_CS, OUTPUT); digitalWrite(TFT_CS, HIGH);
  pinMode(SD_CS, OUTPUT);  digitalWrite(SD_CS, HIGH);
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  initBoutons();

  tft.begin(40000000);
  tft.setRotation(0);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setCursor(6, 6);

  if (!SD.begin(SD_CS, SPI, 20000000)) {
    log("SD : ECHEC montage");
    tft.setTextColor(ILI9341_RED); tft.print("SD : ECHEC");
    return;
  }
  log("SD OK");

  chercherMp3("/Music");
  log(String(morceaux.size()) + " MP3 trouve(s) dans /Music");
  if (morceaux.empty()) { tft.setTextColor(ILI9341_RED); tft.print("Aucun MP3"); return; }

  Audio::audio_info_callback = infoAudio;
  audio.setPinout(I2S_BCK, I2S_LRCK, I2S_DIN);
  audio.setVolume(VOLUME);
  log("I2S : BCK=" + String(I2S_BCK) + " LRCK=" + String(I2S_LRCK) + " DIN=" + String(I2S_DIN) +
      "  volume " + String(VOLUME) + "/21");
  jouer(0);
}

// --- Commandes ---
void commande(char c) {
  if (morceaux.empty()) return;
  switch (c) {
    case 'p':
      audio.pauseResume();
      log(audio.isRunning() ? "Lecture" : "Pause");
      break;
    case '>': jouer(courant + 1); break;
    case 'f':   // test : saute a 10 s de la fin pour verifier l'enchainement
      audio.setAudioPlayTime(audio.getAudioFileDuration() > 10 ? audio.getAudioFileDuration() - 10 : 0);
      log("Saut a 10 s de la fin");
      break;
    case '<':
      // comme sur un iPod : apres 3 s, "precedent" revient au debut du morceau
      if (audio.getAudioCurrentTime() > 3) jouer(courant);
      else jouer(courant + morceaux.size() - 1);
      break;
    case '+': case '-': {
      int v = audio.getVolume() + (c == '+' ? 1 : -1);
      audio.setVolume(constrain(v, 0, 21));
      log("Volume " + String(audio.getVolume()) + "/21");
      tft.fillRect(0, 296, 240, 16, ILI9341_BLACK);
      tft.setTextSize(1);
      tft.setTextColor(ILI9341_YELLOW);
      tft.setCursor(6, 300);
      tft.print("Volume " + String(audio.getVolume()) + "/21");
      break;
    }
  }
}

struct Bouton {
  uint8_t broche;
  char action;          // meme code que les touches du moniteur serie
  bool etat, lecture;
  uint32_t changement;
};
Bouton boutons[] = {
  {BTN_PLAY, 'p'}, {BTN_PREV, '<'}, {BTN_NEXT, '>'}, {BTN_VOLUP, '+'}, {BTN_VOLDOWN, '-'},
};

void initBoutons() {
  pinMode(BTN_MENU, INPUT_PULLUP);   // pas utilise ici
  for (auto &b : boutons) {
    pinMode(b.broche, INPUT_PULLUP);
    b.etat = b.lecture = (digitalRead(b.broche) == LOW);
    b.changement = millis();
  }
}

void lireBoutons() {
  uint32_t maintenant = millis();
  for (auto &b : boutons) {
    bool lu = (digitalRead(b.broche) == LOW);
    if (lu != b.lecture) { b.lecture = lu; b.changement = maintenant; }
    else if (lu != b.etat && maintenant - b.changement >= ANTI_REBOND_MS) {
      b.etat = lu;
      if (b.etat) commande(b.action);   // action a l'appui
    }
  }
}

uint32_t dernierLog = 0, derniereBarre = 0, derniereInfo = 0;

void loop() {
  audio.loop();
  if (finMorceau) { finMorceau = false; jouer(courant + 1); }
  lireBoutons();
  while (Serial.available()) commande(Serial.read());
  uint32_t maintenant = millis();
  // redessine les infos quand elles changent (au plus toutes les 300 ms, les tags arrivent groupes)
  if (infosChangees && maintenant - derniereInfo > 300) {
    infosChangees = false;
    derniereInfo = maintenant;
    dessinerInfos();
    dessinerProgression();
  }
  if (audio.isRunning() && maintenant - derniereBarre > 1000) {
    derniereBarre = maintenant;
    dessinerProgression();
  }
  if (audio.isRunning() && maintenant - dernierLog > 5000) {
    dernierLog = maintenant;
    log("  " + String(audio.getAudioCurrentTime()) + " s / " + String(audio.getAudioFileDuration()) + " s");
  }
  vTaskDelay(1);
}
