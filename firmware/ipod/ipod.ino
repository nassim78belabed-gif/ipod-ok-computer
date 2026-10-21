// iPod OK Computer Edition - firmware principal (ESP32-S3-DevKitC-1 N16R8)
// Interface type iPod : menus Artistes / Albums / Morceaux, ecran Lecture en cours, volume
// + page web (web.ino) : envoi de MP3, telecommande, bibliotheque
// Librairies : Adafruit ILI9341 + GFX, TJpg_Decoder (pochettes), ESP32-audioI2S-master 4.x
//
// Boutons :
//   dans les listes     : PREV/NEXT = monter/descendre (maintenir = defilement), PLAY = valider,
//                         MENU = retour (maintenir = menu principal), PLAY maintenu = pause
//   en Lecture en cours : PREV/NEXT = morceau precedent/suivant (maintenir = recul/avance rapide),
//                         PLAY = pause, MENU = retour
//   partout             : VOL+ / VOL- = volume
// Moniteur serie : memes touches que les boutons (m, p, <, >, +, -), P = PLAY maintenu,
//                  f = saut a 10 s de la fin (test), w = couper/rallumer le WiFi, t = baisser la puissance WiFi

#include <SPI.h>
#include <WiFi.h>
#include <SD.h>
#include <algorithm>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <TJpg_Decoder.h>
#include "src/ESP32-audioI2S/Audio.h"   // copie patchee de la librairie (voir src/ESP32-audioI2S/LISEZMOI.md)
#include "ipod.h"

Audio audio;
Adafruit_ILI9341 tft(&SPI, TFT_DC, TFT_CS, TFT_RST);
SemaphoreHandle_t verrouAudio;   // l'objet audio est utilise par 2 taches : une seule a la fois

std::vector<Morceau> biblio;
std::vector<Ecran> pile;          // ecrans ouverts, le dernier est affiche
std::vector<String> lignes;       // lignes de la liste affichee
std::vector<int> lignesIdx;       // E_MORCEAUX : indice du morceau de chaque ligne
std::vector<String> lignesArtiste;// E_ALBUMS : artiste de chaque album

std::vector<int> fileLecture;     // morceaux a jouer (la liste d'ou on a lance la lecture)
int posFile = -1;                 // position dans fileLecture (-1 = rien)
bool enPause = false;

// rempli par la tache audio (callback), lu par la boucle principale
volatile bool finMorceau = false;
volatile bool pochetteNouvelle = false;
volatile uint32_t pochettePos = 0, pochetteTaille = 0;

void log(const String &txt) {
  Serial.println(txt);
#if ARDUINO_USB_CDC_ON_BOOT
  Serial0.println(txt);   // aussi sur le port "COM" (UART0)
#endif
}

void verrouiller() { xSemaphoreTake(verrouAudio, portMAX_DELAY); }
void deverrouiller() { xSemaphoreGive(verrouAudio); }

// ------------------------------------------------------------------ Audio
// La tache audio tourne en continu pour que le son ne coupe pas pendant qu'on dessine l'ecran
void tacheAudio(void *) {
  for (;;) {
    verrouiller();
    audio.loop();
    deverrouiller();
    vTaskDelay(1);
  }
}

// messages de la librairie (appele depuis la tache audio)
void infoAudio(Audio::msg_t m) {
  if (m.e == Audio::evt_eof) finMorceau = true;   // pas de connecttoFS() ici : blocage
  else if (m.e == Audio::evt_image && m.vec1.size() >= 2) {
    pochettePos = m.vec1[0];
    pochetteTaille = m.vec1[1];
    pochetteNouvelle = true;
  }
}

Morceau &morceauCourant() { return biblio[fileLecture[posFile]]; }

void jouer(int pos) {
  if (fileLecture.empty()) return;
  posFile = (pos + fileLecture.size()) % fileLecture.size();
  Morceau &m = morceauCourant();
  log(">>> " + String(posFile + 1) + "/" + String(fileLecture.size()) + " : " + m.artiste + " - " + m.titre);
  easterMorceau(m);                   // theme et effets de l'album (easter.txt)
  pochettePos = pochetteTaille = 0;   // la pochette integree eventuelle arrive plus tard (evt_image)
  verrouiller();
  bool ok = audio.connecttoFS(SD, m.chemin.c_str());
  deverrouiller();
  if (!ok) { log("ECHEC ouverture " + m.chemin); easterErreur(m); }
  enPause = false;
  if (pile.back().type == E_LECTURE) dessinerLecture();
  else dessinerEntete();
}

void basculerPause() {
  if (posFile < 0) return;
  verrouiller();
  audio.pauseResume();
  deverrouiller();
  enPause = !enPause;
  log(enPause ? "Pause" : "Lecture");
  dessinerEntete();
}

void precedent() {
  verrouiller();
  uint32_t t = audio.getAudioCurrentTime();
  deverrouiller();
  jouer(t > 3 ? posFile : posFile - 1);   // comme un iPod : apres 3 s, revient au debut du morceau
}

void avanceRapide(int sec) {
  if (posFile < 0 || enPause) return;
  verrouiller();
  audio.setTimeOffset(sec);
  deverrouiller();
  majProgression();
}

void volume(int delta) {
  verrouiller();
  audio.setVolume(constrain(audio.getVolume() + delta, 0, VOLUME_MAX));
  deverrouiller();
  log("Volume " + String(audio.getVolume()) + "/" + String(VOLUME_MAX));
  if (pile.back().type == E_LECTURE) majVolume();
}

// ------------------------------------------------------------------ Navigation
void construireLignes() {
  Ecran &e = pile.back();
  lignes.clear(); lignesIdx.clear(); lignesArtiste.clear();
  switch (e.type) {
    case E_MENU:
      lignes = {"Artistes", "Albums", "Morceaux", "Lecture en cours", "Transfert MP3", "A propos"};
      break;
    case E_ARTISTES:
      lignes = listeArtistes();
      break;
    case E_ALBUMS:
      lignes = listeAlbums(e.artiste, &lignesArtiste);
      break;
    case E_MORCEAUX:
      lignesIdx = listeMorceaux(e.artiste, e.album);
      for (int i : lignesIdx) lignes.push_back(biblio[i].titre);
      break;
    case E_TRANSFERT:
      if (!webActif) { lignes = {"Demarrage du WiFi..."}; break; }
      lignes = {"1. WiFi du telephone :", "  " + reseauWeb, "2. Page :", "  http://" + adresseWeb,
                "", "Recus : " + String(nbRecus) + " MP3", "", "MENU = quitter"};
      break;
    case E_LECTURE: case E_APROPOS:
      break;
  }
}

void ouvrir(TypeEcran type, const String &titre, const String &artiste, const String &album) {
  Ecran e;
  e.type = type; e.titre = titre; e.artiste = artiste; e.album = album;
  pile.push_back(e);
  construireLignes();
  dessinerEcran();
}

void retour(bool jusquAuMenu) {
  if (pile.size() <= 1) return;
  if (pile.back().type == E_TRANSFERT) sortirTransfert();
  if (jusquAuMenu) pile.resize(1);
  else pile.pop_back();
  construireLignes();
  dessinerEcran();
}

void deplacer(int delta) {
  Ecran &e = pile.back();
  int n = lignes.size();
  if (n == 0) return;
  int avant = e.sel, hautAvant = e.haut;
  e.sel = constrain(e.sel + delta, 0, n - 1);
  if (e.sel == avant) return;
  if (e.sel < e.haut) e.haut = e.sel;
  if (e.sel >= e.haut + LIGNES_VISIBLES) e.haut = e.sel - LIGNES_VISIBLES + 1;
  if (e.haut != hautAvant) dessinerListe();   // la liste a defile : tout redessiner
  else { dessinerLigne(avant); dessinerLigne(e.sel); }
}

void valider() {
  Ecran &e = pile.back();
  if (lignes.empty()) return;
  String choix = lignes[e.sel];
  switch (e.type) {
    case E_MENU:
      if (e.sel == 0) ouvrir(E_ARTISTES, "Artistes", "", "");
      else if (e.sel == 1) ouvrir(E_ALBUMS, "Albums", "", "");
      else if (e.sel == 2) ouvrir(E_MORCEAUX, "Morceaux", "", "");
      else if (e.sel == 3) { if (posFile >= 0) ouvrir(E_LECTURE, "Lecture en cours", "", ""); }
      else if (e.sel == 4) entrerTransfert();
      else ouvrir(E_APROPOS, "A propos", "", "");
      break;
    case E_ARTISTES:
      ouvrir(E_ALBUMS, choix, choix, "");
      break;
    case E_ALBUMS:
      ouvrir(E_MORCEAUX, choix, lignesArtiste[e.sel], choix);
      break;
    case E_MORCEAUX: {
      int sel = e.sel;   // e ne sera plus valide une fois la pile agrandie
      fileLecture = lignesIdx;
      Ecran l;
      l.type = E_LECTURE; l.titre = "Lecture en cours";
      pile.push_back(l);
      jouer(sel);        // dessine l'ecran Lecture en cours
      break;
    }
    case E_LECTURE: case E_TRANSFERT: case E_APROPOS:
      break;
  }
}

// ------------------------------------------------------------------ Commandes (boutons et moniteur serie)
void action(char c, Geste g) {
  bool lecture = pile.back().type == E_LECTURE;
  if (lecture && easterEcranToucheur()) return;   // ecran secret : l'appui sert a le fermer
  if (lecture && g == G_APPUI) easterTouche(c);   // code secret aux boutons
  switch (c) {
    case 'm':
      if (g == G_COURT) retour(false);
      else if (g == G_LONG) retour(true);
      break;
    case 'p':
      if (g == G_LONG) basculerPause();
      else if (g == G_COURT) { if (lecture) basculerPause(); else valider(); }
      break;
    case '<': case '>': {
      int sens = c == '>' ? 1 : -1;
      if (!lecture) { if (g == G_APPUI || g == G_REPETE) deplacer(sens); }
      else if (g == G_COURT) { if (sens > 0) jouer(posFile + 1); else precedent(); }
      else if (g == G_REPETE) avanceRapide(5 * sens);
      break;
    }
    case '+': case '-':
      if (g == G_APPUI || g == G_REPETE) volume(c == '+' ? 1 : -1);
      break;
  }
}

struct Bouton {
  uint8_t broche;
  char code;
  bool etat, lecture, longFait;
  uint32_t changement, debut, dernierRepete;
};
Bouton boutons[] = {
  {BTN_MENU, 'm'}, {BTN_PLAY, 'p'}, {BTN_PREV, '<'}, {BTN_NEXT, '>'}, {BTN_VOLUP, '+'}, {BTN_VOLDOWN, '-'},
};
#define ANTI_REBOND_MS 30
#define APPUI_LONG_MS  500
#define REPETITION_MS  150

void initBoutons() {
  for (auto &b : boutons) {
    pinMode(b.broche, INPUT_PULLUP);
    b.etat = b.lecture = (digitalRead(b.broche) == LOW);
    b.changement = millis();
    b.longFait = true;   // un bouton deja appuye au demarrage ne declenche rien
  }
}

void lireBoutons() {
  uint32_t t = millis();
  for (auto &b : boutons) {
    bool lu = (digitalRead(b.broche) == LOW);
    if (lu != b.lecture) { b.lecture = lu; b.changement = t; }
    else if (lu != b.etat && t - b.changement >= ANTI_REBOND_MS) {
      b.etat = lu;
      if (b.etat) {                         // appui
        b.debut = t; b.longFait = false;
        action(b.code, G_APPUI);
      } else if (!b.longFait) {             // relache avant l'appui long
        action(b.code, G_COURT);
      }
    }
    if (b.etat && !b.longFait && t - b.debut >= APPUI_LONG_MS) {
      b.longFait = true; b.dernierRepete = t;
      action(b.code, G_LONG);
      action(b.code, G_REPETE);
    } else if (b.etat && b.longFait && t - b.dernierRepete >= REPETITION_MS && t - b.debut >= APPUI_LONG_MS) {
      b.dernierRepete = t;
      action(b.code, G_REPETE);
    }
  }
}

// liste tous les fichiers de la SD (commande 'l')
void listerSD(const String &dossier) {
  File dir = SD.open(dossier);
  if (!dir) return;
  File f = dir.openNextFile();
  while (f) {
    String chemin = (dossier == "/" ? "" : dossier) + "/" + f.name();
    bool estDossier = f.isDirectory();
    uint32_t taille = f.size();
    f.close();
    if (estDossier) { log("D " + chemin); listerSD(chemin); }
    else log("F " + chemin + " " + String(taille / 1024) + " Ko");
    f = dir.openNextFile();
  }
}

void lireSerie() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == 'l') {
      log("--- Fichiers SD ---");
      listerSD("/");
      log("--- Fin liste ---");
    } else if (c == 'x') {   // x<chemin> : supprime un fichier (ou un dossier vide)
      String chemin = Serial.readStringUntil('\n'); chemin.trim();
      verrouiller();
      audio.stopSong();       // ne pas supprimer un fichier en cours de lecture
      deverrouiller();
      File f = SD.open(chemin);
      bool estDossier = f && f.isDirectory();
      if (f) f.close();
      bool ok = estDossier ? SD.rmdir(chemin) : SD.remove(chemin);
      log(String(ok ? "Supprime : " : "ECHEC suppression : ") + chemin);
    } else if (c == 'i') {   // i<chemin> : diagnostic des tags d'un MP3
      String chemin = Serial.readStringUntil('\n'); chemin.trim();
      diagnosticTags(chemin);
    } else if (c == 'r') {   // relit la bibliotheque (apres des suppressions)
      verrouiller();
      audio.stopSong();
      deverrouiller();
      chargerBibliotheque();
    } else if (c == 'R') {
      log("Redemarrage");
      delay(100);
      ESP.restart();
    } else if (c == 'E') {   // test : ecran d'erreur "fichier introuvable" de l'album en cours
      if (posFile >= 0) easterErreur(morceauCourant());
    } else if (c == 'g') {   // g<secondes> : saute a cet instant du morceau (test)
      uint32_t s = Serial.readStringUntil('\n').toInt();
      verrouiller();
      audio.setAudioPlayTime(s);
      deverrouiller();
      log("Saut a " + String(s) + " s");
    } else if (c == 'P') action('p', G_LONG);
    else if (c == 'f') {
      verrouiller();
      uint32_t d = audio.getAudioFileDuration();
      audio.setAudioPlayTime(d > 10 ? d - 10 : 0);
      deverrouiller();
      log("Saut a 10 s de la fin");
    } else if (c == 'w') {   // test : couper / rallumer le WiFi
      if (WiFi.getMode() == WIFI_OFF) initWeb();
      else { WiFi.mode(WIFI_OFF); webActif = false; log("WiFi coupe"); }
    } else if (c == 't') {   // test : puissance d'emission WiFi de plus en plus basse
      static const wifi_power_t niveaux[] = {WIFI_POWER_19_5dBm, WIFI_POWER_15dBm, WIFI_POWER_11dBm,
                                             WIFI_POWER_8_5dBm, WIFI_POWER_5dBm, WIFI_POWER_2dBm};
      static int n = 0;
      n = (n + 1) % 6;
      WiFi.setTxPower(niveaux[n]);
      log("Puissance WiFi : " + String(niveaux[n] / 4.0, 1) + " dBm");
    } else if (strchr("mp<>+-", c)) {
      action(c, G_APPUI);
      action(c, G_COURT);
    }
  }
}

// ------------------------------------------------------------------ Demarrage
void setup() {
  Serial.begin(115200);
#if ARDUINO_USB_CDC_ON_BOOT
  Serial0.begin(115200);
#endif
  delay(1000);
  log("\n=== iPod OK Computer ===");

  pinMode(TFT_CS, OUTPUT); digitalWrite(TFT_CS, HIGH);
  pinMode(SD_CS, OUTPUT);  digitalWrite(SD_CS, HIGH);
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  initBoutons();
  initEcran();

  if (!SD.begin(SD_CS, SPI, 20000000)) {
    log("SD : ECHEC montage");
    afficherErreur("Carte SD absente");
    while (true) delay(1000);
  }
  afficherChargement(0);
  chargerBibliotheque();
  if (biblio.empty()) { afficherErreur("Aucun MP3 dans /Music"); while (true) delay(1000); }

  verrouAudio = xSemaphoreCreateMutex();
  Audio::audio_info_callback = infoAudio;
  audio.setPinout(I2S_BCK, I2S_LRCK, I2S_DIN);
  audio.setVolume(VOLUME_DEPART);
  xTaskCreatePinnedToCore(tacheAudio, "audio", 8192, nullptr, 2, nullptr, 1);
#if TELECOMMANDE_WEB
  initWeb();   // tests : WiFi allume des le demarrage
#endif

  ouvrir(E_MENU, "iPod", "", "");
}

uint32_t derniereMaj = 0;

void loop() {
  lireBoutons();
  lireSerie();
  if (webActif) boucleWeb();
  if (finMorceau) { finMorceau = false; jouer(posFile + 1); }

  if (pile.back().type == E_LECTURE) {
    if (pochetteNouvelle) {   // deja affichee si la pochette etait connue par les tags
      pochetteNouvelle = false;
      if (!morceauCourant().apic) dessinerPochette();
    }
    majEaster();
    if (millis() - derniereMaj > 500) { derniereMaj = millis(); majProgression(); }
  }
  delay(2);
}
