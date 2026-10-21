// Declarations communes a tous les onglets du sketch ipod
#pragma once
#include <Arduino.h>
#include <vector>

// --- Brochage (hardware/wiring.md, PCB v_final1) ---
#define SPI_SCK   8
#define SPI_MOSI  17
#define SPI_MISO  18   // SD uniquement (SDO ecran NON cable)
#define SD_CS     16
#define TFT_CS    11
#define TFT_DC    9
#define TFT_RST   10

#define I2S_BCK   7
#define I2S_DIN   6
#define I2S_LRCK  5

#define BTN_MENU     42
#define BTN_PLAY     41
#define BTN_PREV     40
#define BTN_NEXT     39
#define BTN_VOLUP    38
#define BTN_VOLDOWN  1

#define VOLUME_DEPART 16   // volume au demarrage
#define VOLUME_MAX    19   // sur 21 : au-dessus, la sortie du DAC sature (gresillements)

// Web : 0 = version finale (WiFi seulement dans le menu "Transfert MP3", page = envoi de MP3)
//       1 = tests (WiFi allume en permanence, page = telecommande + bibliotheque + envoi)
// Sur breadboard, le WiFi fait des bips dans le son et peut blanchir l'ecran (pics de courant sur le 3,3 V)
#define TELECOMMANDE_WEB 0

// Listes : barre du haut 24 px + 11 lignes de 26 px
#define HAUT_ENTETE     24
#define HAUT_LIGNE      26
#define LIGNES_VISIBLES 11

// Un morceau de la bibliotheque (lu dans les tags ID3 au demarrage)
struct Morceau {
  String chemin, titre, artiste, album;
  uint16_t piste;
  uint32_t apic;   // position de la pochette integree (cadre APIC), 0 = aucune
};

// Les ecrans de l'interface, empiles comme sur un iPod (MENU = retour)
enum TypeEcran { E_MENU, E_ARTISTES, E_ALBUMS, E_MORCEAUX, E_LECTURE, E_TRANSFERT, E_APROPOS };

struct Ecran {
  TypeEcran type;
  String artiste, album;   // filtres (vides = tout)
  String titre;            // texte de la barre du haut
  int sel = 0;             // ligne selectionnee
  int haut = 0;            // premiere ligne visible
};

// Evenements des boutons
enum Geste { G_APPUI, G_REPETE, G_COURT, G_LONG };

// Fonctions dont Arduino ne genere pas le prototype automatiquement (type de retour complexe)
std::vector<String> listeArtistes();
std::vector<String> listeAlbums(const String &artiste, std::vector<String> *artistesAlbums);
std::vector<int> listeMorceaux(const String &artiste, const String &album);
Morceau &morceauCourant();
String cheminPochetteSD(const Morceau &m);
extern String adresseWeb, reseauWeb;   // definis dans web.ino
extern bool webActif;
extern int nbRecus;

// zone de la pochette sur l'ecran Lecture en cours
#define POCHETTE_X   50
#define POCHETTE_Y   38
#define POCHETTE_T   140

// Ecran "A propos" : numero de serie imprime en rouge au dos de la pochette du CD OKNOTOK 1997 2017
#define NUMERO_SERIE "18576397"
#define MODELE        "OKNOTOK 1997-2017"
#define AUTEUR        "Nassim Belabed"
#define DEBUT_PROJET  "16 mars 2026"   // premier commit du depot

// Couleurs de l'interface (style iPod Classic)
#define C_BLANC      ILI9341_WHITE
#define C_NOIR       ILI9341_BLACK
#define C_BLEU       0x1C9F   // selection (RGB 30,145,250)
#define C_GRIS_TEXTE 0x6B4D   // gris fonce
#define C_GRIS_CLAIR 0xCE59
#define C_BARRE      0x1C9F

// Easter eggs par album (easter.ino) : theme de l'ecran Lecture en cours, change selon l'album
extern uint16_t eFond, eTexte, eGris, eAccent;
extern String eBanniere;
extern uint32_t eEcranFin;   // != 0 : un ecran secret est affiche
extern uint16_t *pochetteCache;   // copie de la pochette affichee (PSRAM), pour la restaurer sans la redecoder
extern bool pochetteEnCache;
extern String eStyle;        // "mac" : l'ecran Lecture en cours prend le style Mac 1997 (easter.ino / ecran.ino)
extern int eTape;            // lettres du titre deja tapees (style mac)
extern String eBarre;        // "eau" : la barre de progression est animee par easter.ino
extern uint32_t dernierTemps, derniereDuree;   // progression affichee (ecran.ino), en secondes
