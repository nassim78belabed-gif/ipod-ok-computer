// Easter eggs par album : fichier easter.txt dans le dossier de l'album sur la SD
//
// Format (une instruction par ligne, # = commentaire, accents ignores a l'affichage) :
//   fond=RRGGBB  texte=RRGGBB  gris=RRGGBB  accent=RRGGBB   couleurs de l'ecran Lecture en cours (hexa)
//   banniere=TEXTE                                          petit titre au-dessus de la pochette
//   piste=N effet message...                                effet quand la piste N est jouee (numero de piste du tag)
//        effets : texte (fixe), defile (texte qui defile), glitch (fausse erreur qui clignote avec le titre)
//   code=SEQUENCE ligne1/ligne2/ligne3                      code secret aux boutons en Lecture en cours
//        SEQUENCE : 6 a 12 appuis sur + et - (VOL+ / VOL-), le volume est remis comme avant
//        -> ecran secret plein ecran 4 s (les / separent les lignes ; # devant la 1re ligne = tres gros chiffres)
//   barre=N eau                                             piste N : la barre de progression se remplit comme de l'eau (vagues)
//   style=N tracker                                         piste N : ecran "Spidey Tracker" (carte du monde animee, voir plus bas)
//   style=N mac                                             piste N : ecran Lecture en cours en style Mac 1997, titre tape lettre par lettre
//   fondanime=courants                                      fond anime : lignes d'ecoulement dans les marges de la pochette
//   rayure=N debut-fin                                      piste N : effet "disque raye" entre debut et fin (secondes, decimales permises) ;
//        plusieurs lignes rayure=N pour plusieurs passages
//   passage=N NOM                                           animation plein ecran quand la piste N commence juste apres la piste N-1 (NOM : notok)
//   badge=N TEXTE                                           petit autocollant a droite de la pochette pour la piste N
//   erreur=TEXTE                                            ecran affiche quand un fichier du dossier est introuvable
//   code=SEQ @zx                                            faux ecran de chargement ZX Spectrum
//   lancement=NOM                                           animation plein ecran quand l'album commence a jouer
//        NOMs connus : xero (Linkin Park : le premier nom du groupe etait Xero)

#include "tracker.h"

#define EASTER_BANDE_Y 270
#define EASTER_BANDE_H 16
#define EASTER_CODE_MS 4000

struct EasterPiste { int n; String effet, texte; };
struct EasterCode { String seq, texte; };

uint16_t eFond = C_BLANC, eTexte = C_NOIR, eGris = C_GRIS_TEXTE, eAccent = C_BARRE;
String eBanniere;
struct FenetreRayure { uint32_t a, b; };   // passage qui saute, en millisecondes dans le morceau
static std::vector<FenetreRayure> eFenetres;
static bool eRayActif = false;
static uint32_t eRayDernier = 0, eRayFrame = 0, eRefMs = 0, eRefS = 0xFFFFFFFF, eGuerisonT = 0;
static int eGuerison = -1;   // -1 = rien ; 0..7 = bande de pochette en cours de restauration
static std::vector<EasterPiste> eRayures;   // rayure=N debut-fin (effet = "debut-fin", secondes, decimales permises)
String eFondAnime;   // fond anime de l'album ("courants" ou vide)
static GFXcanvas16 *eToileFond = nullptr;
static uint32_t eFondDernier = 0;
String eStyle;   // style de l'ecran Lecture du morceau en cours ("mac" ou vide)
int eTape = 0;
static uint32_t eTapeDebut = 0;
static bool eCurseur = false;
String eBarre;   // effet de la barre de progression du morceau en cours ("eau" ou vide)
uint32_t eEcranFin = 0;
static String eLancement;
static String eAnimAttente, eAnimNom;   // animation de lancement (xero, notok) a jouer des que l'ecran Lecture est affiche
static std::vector<EasterPiste> ePassages, eBadges;
static String eBadge, eErreur;
static int ePrecPiste = -1;
static String ePrecDossier;
static int eMode = 0, eEtape = 0;   // ecran plein ecran en cours : 0 = code secret, 1 = animation de lancement, 2 = faux chargement ZX Spectrum

static std::vector<EasterPiste> ePistes;
static std::vector<EasterCode> eCodes;
static std::vector<EasterPiste> eStyles;   // style=N nom
static std::vector<EasterPiste> eBarres;   // barre=N effet (texte = nom de l'effet)
static GFXcanvas16 *eToileBarre = nullptr;
static uint32_t eBarreDernier = 0;
static String eDossier;
static bool eCharge = false;

// effet du morceau en cours
static String eEffet, eMessage;
static bool eStatiqueFait = false, eGlitchActif = false;
static uint32_t eScroll = 0, eDernier = 0, eGlitchFin = 0, eGlitchProchain = 0;
static GFXcanvas16 *eToile = nullptr;

// code secret
struct Touche { char c; int vol; };
static Touche eHist[12];
static int eNb = 0;
static int eCodeTrouve = -1, eCodeVolume = 0;   // code reconnu, a lancer dans loop() (apres le changement de volume)
static uint32_t eEcranDebut = 0;
static int eEcranY = 0;
static bool eEcranTexteFait = false;
static String eEcranTexte;

uint16_t couleurHex(const String &s) {
  uint32_t v = strtoul(s.c_str(), nullptr, 16);
  return tft.color565((v >> 16) & 255, (v >> 8) & 255, v & 255);
}

void chargerEaster(const String &dossier) {
  if (eCharge && dossier == eDossier) return;
  eCharge = true; eDossier = dossier;
  eFond = C_BLANC; eTexte = C_NOIR; eGris = C_GRIS_TEXTE; eAccent = C_BARRE;
  eBanniere = ""; eLancement = ""; eFondAnime = ""; eErreur = "";
  ePassages.clear(); eBadges.clear();
  ePistes.clear(); eCodes.clear(); eBarres.clear(); eStyles.clear(); eRayures.clear();
  File f = SD.open(dossier + "/easter.txt");
  if (!f) return;
  while (f.available()) {
    String l = f.readStringUntil('\n'); l.trim();
    int eq = l.indexOf('=');
    if (!l.length() || l[0] == '#' || eq < 0) continue;
    String k = l.substring(0, eq), v = l.substring(eq + 1);
    k.trim(); v.trim(); k.toLowerCase();
    if (k == "fond") eFond = couleurHex(v);
    else if (k == "texte") eTexte = couleurHex(v);
    else if (k == "gris") eGris = couleurHex(v);
    else if (k == "accent") eAccent = couleurHex(v);
    else if (k == "banniere") eBanniere = sansAccents(v);
    else if (k == "erreur") eErreur = sansAccents(v);
    else if (k == "passage" || k == "badge") {
      int a = v.indexOf(' ');
      if (a > 0) { String nom = v.substring(a + 1); nom.trim(); (k == "badge" ? eBadges : ePassages).push_back({(int)v.substring(0, a).toInt(), k == "badge" ? sansAccents(nom) : nom, ""}); }
    } else if (k == "rayure") {
      int a = v.indexOf(' ');
      if (a > 0) { String f = v.substring(a + 1); f.trim(); eRayures.push_back({(int)v.substring(0, a).toInt(), f, ""}); }
    } else if (k == "fondanime") { v.toLowerCase(); eFondAnime = v; }
    else if (k == "style") {
      int a = v.indexOf(' ');
      if (a > 0) { String nom = v.substring(a + 1); nom.trim(); eStyles.push_back({(int)v.substring(0, a).toInt(), nom, ""}); }
    } else if (k == "barre") {
      int a = v.indexOf(' ');
      if (a > 0) { String nom = v.substring(a + 1); nom.trim(); eBarres.push_back({(int)v.substring(0, a).toInt(), nom, ""}); }
    } else if (k == "lancement") { v.toLowerCase(); eLancement = v; }
    else if (k == "piste") {
      int a = v.indexOf(' '), b = v.indexOf(' ', a + 1);
      if (a > 0 && b > a) ePistes.push_back({(int)v.substring(0, a).toInt(), v.substring(a + 1, b), sansAccents(v.substring(b + 1))});
    } else if (k == "code") {
      int a = v.indexOf(' ');
      if (a >= 6 && a <= 12) eCodes.push_back({v.substring(0, a), sansAccents(v.substring(a + 1))});
    }
  }
  f.close();
  log("Easter : " + dossier + " (" + String(ePistes.size()) + " pistes, " + String(eCodes.size()) + " codes)");
}

// appele par jouer() avant de dessiner l'ecran Lecture en cours
void easterMorceau(const Morceau &m) {
  String dossier = m.chemin.substring(0, m.chemin.lastIndexOf('/'));
  bool nouvelAlbum = !(eCharge && dossier == eDossier);
  chargerEaster(dossier);
  if (nouvelAlbum && eLancement.length()) eAnimAttente = eLancement;   // l'album commence a jouer
  bool suite = !nouvelAlbum && ePrecDossier == dossier && ePrecPiste == (int)m.piste - 1;   // la piste suit la precedente de l'album
  for (auto &p : ePassages) if (p.n == m.piste && suite) eAnimAttente = p.effet;
  ePrecDossier = dossier; ePrecPiste = m.piste;
  eBadge = "";
  for (auto &b : eBadges) if (b.n == m.piste) eBadge = b.effet;
  eEffet = eMessage = eBarre = eStyle = "";
  for (auto &s : eStyles) if (s.n == m.piste) eStyle = s.effet;
  eFenetres.clear(); eRayActif = false; eGuerison = -1; eRefS = 0xFFFFFFFF;
  for (auto &r : eRayures) if (r.n == m.piste) {
    int tiret = r.effet.indexOf('-');
    eFenetres.push_back({(uint32_t)(r.effet.substring(0, tiret).toFloat() * 1000), (uint32_t)(r.effet.substring(tiret + 1).toFloat() * 1000)});
  }
  if (!eFenetres.empty() && !pochetteCache) pochetteCache = (uint16_t *)ps_malloc(POCHETTE_T * POCHETTE_T * 2);   // copie de la pochette pour la restaurer
  for (auto &b : eBarres) if (b.n == m.piste) eBarre = b.effet;
  for (auto &p : ePistes) if (p.n == m.piste) { eEffet = p.effet; eMessage = p.texte; }
  if (eEffet.length()) log("Easter : effet " + eEffet + " (piste " + String(m.piste) + ")");
  eGlitchActif = false;
  eGlitchProchain = millis() + 2500;
}

// appele en fin de dessinerLecture() : la bande est vide, a redessiner
void easterLecture() {
  eStatiqueFait = false;
  eScroll = 0;
  eTape = 0; eTapeDebut = millis(); eCurseur = false;
  if (eBadge.length() && eStyle != "mac" && eStyle != "tracker") {   // autocollant a droite de la pochette
    int w = eBadge.length() * 6 + 10, x = 236 - w, y = 44;
    uint16_t fond = eBadge == "INEDIT" ? eAccent : eGris;
    tft.fillRoundRect(x + 2, y + 2, w, 16, 3, eGris);      // ombre
    tft.fillRoundRect(x, y, w, 16, 3, fond);
    tft.setTextSize(1);
    tft.setTextColor(eFond, fond);
    tft.setCursor(x + 5, y + 4);
    tft.print(eBadge);
  }
}

void titreDecale(int dx, uint16_t couleur) {
  String t = ajuster(morceauCourant().titre, 19);
  tft.fillRect(0, 186, 240, 18, eFond);
  tft.setTextSize(2);
  tft.setTextColor(couleur, eFond);
  tft.setCursor((240 - t.length() * 12) / 2 + dx, 186);
  tft.print(t);
}

void envoyerBande() {
  tft.drawRGBBitmap(0, EASTER_BANDE_Y, eToile->getBuffer(), 240, EASTER_BANDE_H);
}

void majBande() {
  uint32_t t = millis();
  if (!eToile) eToile = new GFXcanvas16(240, EASTER_BANDE_H);
  if (!eToile->getBuffer()) return;
  String msg = ajuster(eMessage, 140);   // un message qui defile peut etre long
  String effet = eEffet;
  if (effet == "texte" && msg.length() > 20) effet = "defile";   // trop long pour l'ecran : il defile

  if (effet == "texte") {
    if (eStatiqueFait) return;
    eStatiqueFait = true;
    eToile->fillScreen(eFond);
    eToile->setTextSize(2);
    eToile->setTextColor(eAccent);
    eToile->setCursor(max(0, (240 - (int)msg.length() * 12) / 2), 1);
    eToile->print(ajuster(eMessage, 20));
    envoyerBande();
  } else if (effet == "defile") {
    int largeur = msg.length() * 12;
    eToile->fillScreen(eFond);
    eToile->setTextSize(2);
    eToile->setTextColor(eAccent);
    eToile->setCursor(240 - (int)(eScroll % (largeur + 240)), 1);
    eToile->print(msg);
    eScroll += 4;
    envoyerBande();
  } else if (effet == "glitch") {
    if (!eGlitchActif) {
      if (t < eGlitchProchain) return;
      eGlitchActif = true; eGlitchFin = t + 700;
    }
    if (t >= eGlitchFin) {   // fin de la secousse : tout remis en place
      eGlitchActif = false;
      eGlitchProchain = t + 3500 + random(4000);
      eToile->fillScreen(eFond);
      envoyerBande();
      titreDecale(0, eTexte);
      return;
    }
    static const uint16_t couleurs[] = {ILI9341_RED, ILI9341_WHITE, ILI9341_CYAN, ILI9341_YELLOW};
    uint16_t c = couleurs[random(4)];
    eToile->fillScreen(random(3) ? eFond : ILI9341_RED);
    eToile->setTextSize(2);
    eToile->setTextColor(c);
    eToile->setCursor(max(0, (240 - (int)ajuster(eMessage, 20).length() * 12) / 2) + random(-8, 9), 1);
    eToile->print(ajuster(eMessage, 20));
    for (int i = 0; i < 5; i++) eToile->drawFastHLine(random(240), random(EASTER_BANDE_H), random(10, 80), couleurs[random(4)]);
    envoyerBande();
    titreDecale(random(-6, 7), random(2) ? ILI9341_RED : eTexte);
  }
}

// ------------------------------------------------------------------ Code secret
void lancerEcranSecret(const String &texte) {
  eEcranDebut = millis();
  eEcranFin = eEcranDebut + EASTER_CODE_MS;
  eEcranY = 0;
  eEcranTexteFait = false;
  eEcranTexte = texte;
  eMode = 0;
}

// ------------------------------------------------------------------ Animation de lancement : XERO
// "Xero" etait le premier nom du groupe (avant Linkin Park) : le titre From Zero y fait allusion
#define LANCEMENT_MS 3800

void motXero(const char *mot) {
  tft.setTextSize(7);
  tft.setTextColor(eAccent, eFond);
  tft.setCursor(36, 100);   // 4 lettres de 42 px : 168 px de large
  tft.print(mot);
}

void lancerLancement(const String &nom) {
  eMode = 1; eEtape = 0; eAnimNom = nom;
  eEcranDebut = millis();
  eEcranFin = eEcranDebut + (nom == "notok" ? 4300 : LANCEMENT_MS);
  log("Easter : animation " + nom);
}

void animerXero() {
  uint32_t t = millis() - eEcranDebut;
  if (eEtape == 0) { tft.fillScreen(eFond); eEtape = 1; }
  else if (eEtape == 1 && t >= 400) { motXero("XERO"); eEtape = 2; }
  else if (eEtape == 2 && t >= 1500) { motXero("ZERO"); eEtape = 3; }       // le X devient Z : FROM ZERO
  else if (eEtape == 3 && t >= 1620) { motXero("XERO"); eEtape = 4; }
  else if (eEtape == 4 && t >= 1740) { motXero("ZERO"); eEtape = 5; }
  else if (eEtape == 5 && t >= 2200) {
    texteCentre("PREMIER NOM DU GROUPE", 190, 1, eTexte, eFond);
    texteCentre("AVANT LINKIN PARK", 206, 1, eGris, eFond);
    eEtape = 6;
  }
}

void animerEcranSecret() {
  uint32_t t = millis() - eEcranDebut;
  if (t < 800) {   // des bandes tombent du haut de l'ecran
    int y = t * 320 / 800;
    for (int y0 = eEcranY / 16 * 16; y0 < y; y0 += 16)
      tft.fillRect(0, y0, 240, 16, (y0 / 16) % 2 ? eAccent : eFond);
    eEcranY = y;
  } else if (!eEcranTexteFait) {
    eEcranTexteFait = true;
    tft.fillScreen(eFond);
    String t = eEcranTexte;
    int y = 0;
    bool grosFait = false;
    if (t.startsWith("#")) {   // 1re ligne en tres gros chiffres (#42 -> 42)
      int fin = t.indexOf('/');
      if (fin < 0) fin = t.length();
      String gros = t.substring(1, fin);
      tft.setTextSize(10);
      tft.setTextColor(eAccent, eFond);
      tft.setCursor((240 - gros.length() * 60) / 2, 30);
      tft.print(gros);
      t = fin < (int)t.length() ? t.substring(fin + 1) : "";
      y = 130; grosFait = true;
    }
    int n = 1;
    for (unsigned i = 0; i < t.length(); i++) if (t[i] == '/') n++;
    if (!y) y = 160 - n * 14;
    int debut = 0;
    for (int k = 0; k < n; k++) {
      int fin = t.indexOf('/', debut);
      if (fin < 0) fin = t.length();
      texteCentre(t.substring(debut, fin), y, 2, (k == 0 && !grosFait) ? eAccent : eTexte, eFond);
      y += 28; debut = fin + 1;
    }
  }
}

// ------------------------------------------------------------------ Passage CD 1 -> CD 2 : "OK" se barre et devient "NOTOK"
// (les deux disques s'appellent OK, pistes 1-12, et NOTOK, pistes 13-23 : titres des 2 disques sur MusicBrainz, ordre au dos de la pochette)
static int eBarreX = 0;

void motNotok(int n) {
  tft.setTextSize(6);
  tft.setTextColor(ILI9341_RED, eFond);
  tft.setCursor(30, 110);
  tft.print(String("NOT").substring(0, n));
}

void animerNotok() {
  uint32_t t = millis() - eEcranDebut;
  if (eEtape == 0) { tft.fillScreen(eFond); eBarreX = 56; eEtape = 1; }
  else if (eEtape == 1 && t >= 300) {            // "OK" en grand
    tft.setTextSize(8); tft.setTextColor(eAccent, eFond); tft.setCursor(72, 100); tft.print("OK");
    eEtape = 2;
  } else if (eEtape == 2 && t >= 1000) {         // trait rouge qui le barre, de gauche a droite
    int x = min(184, 56 + (int)((t - 1000) * 128 / 600));
    if (x > eBarreX) { tft.fillRect(eBarreX, 126, x - eBarreX, 8, ILI9341_RED); eBarreX = x; }
    if (x >= 184) eEtape = 3;
  } else if (eEtape == 3 && t >= 1900) {         // "OK" se decale pour laisser la place a "NOT"
    tft.fillRect(0, 90, 240, 90, eFond);
    tft.setTextSize(6); tft.setTextColor(eAccent, eFond); tft.setCursor(138, 110); tft.print("OK");
    eEtape = 4;
  } else if (eEtape == 4 && t >= 2100) { motNotok(1); eEtape = 5; }
  else if (eEtape == 5 && t >= 2300) { motNotok(2); eEtape = 6; }
  else if (eEtape == 6 && t >= 2500) { motNotok(3); eEtape = 7; }
  else if (eEtape == 7 && t >= 3000) {
    texteCentre("CD 1 : OK  (PISTES 1-12)", 200, 1, eGris, eFond);
    texteCentre("CD 2 : NOTOK  (PISTES 13-23)", 214, 1, eTexte, eFond);
    eEtape = 8;
  }
}

void animerLancement() {
  if (eAnimNom == "notok") animerNotok(); else animerXero();
}

// ------------------------------------------------------------------ Faux chargement ZX Spectrum
// La cassette du coffret OKNOTOK se termine par "OK Computer Program" : chargee dans un ZX Spectrum, cette piste lance un programme
// qui liste les membres du groupe, la date du 19 decembre 1996 et affiche un message secret (sources : presse et fiches du coffret)
#define ZX_MS 10500
#define ZX_NB 9
static const char *ZX_LIGNES[ZX_NB] = {"19 DECEMBRE 1996", "", "CONGRATULATIONS", "....YOU'VE FOUND", "THE SECRET", "MESSAGE", "SYD LIVES HMMMM.", "WE SHOULD GET", "OUT MORE"};
#define ZX_PX 20
#define ZX_PY 48
#define ZX_PL 200
#define ZX_PH 224

void lancerZX() {
  eMode = 2; eEtape = 0;
  eEcranDebut = millis();
  eEcranFin = eEcranDebut + ZX_MS;
  log("Easter : faux chargement ZX Spectrum");
}

// bandes de la bordure : rouge/cyan (en-tete), puis jaune/bleu (donnees), puis bleu fixe
void bordureZX(uint32_t t) {
  uint16_t c1, c2;
  if (t < 1800) { c1 = ILI9341_RED; c2 = ILI9341_CYAN; }
  else if (t < 7500) { c1 = ILI9341_YELLOW; c2 = 0x001F; }
  else { c1 = c2 = 0x0010; }
  int y = 0, k = random(2);
  while (y < 320) {
    int h = (t < 1800) ? 16 : random(3, 11);
    if (y + h > 320) h = 320 - y;
    uint16_t c = (k++ % 2) ? c1 : c2;
    if (y + h <= ZX_PY || y >= ZX_PY + ZX_PH) tft.fillRect(0, y, 240, h, c);
    else {
      tft.fillRect(0, y, ZX_PX, h, c);
      tft.fillRect(ZX_PX + ZX_PL, y, 240 - ZX_PX - ZX_PL, h, c);
    }
    y += h;
  }
}

void animerZX() {
  static int dessinees = 0;
  static uint32_t derniereBordure = 0;
  uint32_t t = millis() - eEcranDebut;
  if (eEtape == 0) {
    tft.fillScreen(C_BLANC);
    tft.setTextSize(1);
    tft.setTextColor(C_NOIR, C_BLANC);
    tft.setCursor(ZX_PX + 4, ZX_PY + 6);
    tft.print("Program: OK Computer Program");
    eEtape = 1; dessinees = 0; derniereBordure = 0;
  }
  if ((t < 7500 && t - derniereBordure >= 50) || (t >= 7500 && eEtape == 1)) {   // la bordure bouge pendant le "chargement"
    bordureZX(t);
    derniereBordure = t;
    if (t >= 7500) eEtape = 2;
  }
  int ligne = t < 2500 ? 0 : min(ZX_NB, (int)((t - 2500) / 600) + 1);   // le texte apparait ligne par ligne
  while (dessinees < ligne) {
    tft.setTextSize(2);
    tft.setTextColor(C_NOIR, C_BLANC);
    tft.setCursor(ZX_PX + 4, ZX_PY + 28 + dessinees * 20);
    tft.print(ZX_LIGNES[dessinees]);
    dessinees++;
  }
}

// ------------------------------------------------------------------ Erreur "fichier introuvable"
void easterErreur(const Morceau &m) {
  if (!eErreur.length()) return;
  log("Easter : erreur " + eErreur);
  lancerEcranSecret(eErreur + "/FICHIER INTROUVABLE/" + ajuster(m.titre, 20));
}

// ------------------------------------------------------------------ Spidey Tracker (Loser, Tame Impala)
// Ecran inspire du site officiel spideytracker.com (Sony Pictures, juin 2026), reprise de l'appli du film Spider-Man: Brand New Day :
// carte du monde avec des "sightings" de Spider-Man, confirmes (vert) ou rumeurs (rouge), et un "Activity Log". Loser est dans le film.
// Les images officielles (logo, personnage, pastilles) sont des fichiers BMP sur la SD, dans /spidey/ : elles ne sont PAS dans le depot
// (usage perso). Sans ces fichiers, l'ecran utilise un dessin simplifie fait par le code (araignee, masque, pastilles).
#define TK_Y0 54
#define TK_FOND  0x0842   // fond sombre (8,8,16), aussi la couleur "transparente" des pastilles
#define TK_POINT 0x42CF   // points de la carte (70,90,125) : assez clairs pour se voir sur l'ecran reel
#define TK_VERT  0x064B   // confirmed sighting
#define TK_ROUGE 0xE105   // rumored sighting
#define TK_GRIS  0x8C73
#define TK_BOUCLE_MS 18000
#define TK_LOG_Y 218
#define TK_TETE_X 204
#define TK_TETE_Y 216
#define TK_TETE_L 60      // hauteur d'une image du personnage
#define TK_TETE_N 76      // images de l'animation

static int eTkEtat[TK_NB + 1];      // -1 pas encore apparu, 0 eteint, 1 allume
static int eTkCar[TK_NB + 1];       // caracteres deja ecrits dans le journal
static uint32_t eTkDebut = 0, eTkDernier = 0;
static int eTkNbConf = 0, eTkNbRum = 0, eTkFrame = -1;
static bool eTkAssets = false;      // images officielles trouvees sur la SD
static uint16_t eTkPinRam[2][256];  // pastilles 16x16 (0 = verte, 1 = rouge)

int tkX(int i) { return i < TK_NB ? TK_PINS[i].x : TK_IPOD_X; }
int tkY(int i) { return i < TK_NB ? TK_PINS[i].y : TK_IPOD_Y; }
uint32_t tkDebut(int i) { return i < TK_NB ? 1000 + i * 2500 : 11500; }   // moment ou le repere apparait

String tkTexte(int i) {
  if (i == TK_NB) return "POSITION ACTUELLE : TON IPOD";
  return String(TK_PINS[i].confirme ? "CONFIRMED SIGHTING - " : "RUMORED SIGHTING - ") + TK_PINS[i].nom;
}

// BMP 24 bits sans compression, ecrit de haut en bas : dessine les lignes [l0, l0+n) en (x,y), ou les copie dans ram (RGB565)
bool tkBmp(const char *chemin, int x, int y, int l0, int n, uint16_t *ram) {
  File f = SD.open(chemin);
  if (!f) return false;
  uint8_t e[54];
  if (f.read(e, 54) != 54 || e[0] != 'B' || e[1] != 'M') { f.close(); return false; }
  uint32_t debut = lire32(e + 10);
  int w = (int32_t)lire32(e + 18);
  int32_t h = (int32_t)lire32(e + 22);
  if (h > 0 || (e[28] | (e[29] << 8)) != 24 || lire32(e + 30) != 0 || w <= 0) { f.close(); return false; }   // il faut le sens haut -> bas
  uint32_t rs = ((w * 3 + 3) & ~3);
  uint8_t *src = (uint8_t *)malloc(rs * n);
  uint16_t *dst = (uint16_t *)malloc(w * 2);
  if (!src || !dst) { free(src); free(dst); f.close(); return false; }
  f.seek(debut + (uint32_t)l0 * rs);
  f.read(src, rs * n);
  f.close();
  for (int r = 0; r < n; r++) {
    uint8_t *ligne = src + r * rs;
    for (int i = 0; i < w; i++) { uint8_t *p = ligne + i * 3; dst[i] = ((p[2] & 0xF8) << 8) | ((p[1] & 0xFC) << 3) | (p[0] >> 3); }
    if (ram) memcpy(ram + r * w, dst, w * 2); else tft.drawRGBBitmap(x, y + r, dst, w, 1);
  }
  free(src); free(dst);
  return true;
}

// dessine une image en memoire (RGB565) en sautant les pixels de la couleur du fond (transparence)
void tkRam(const uint16_t *img, int x, int y, int w, int h) {
  for (int r = 0; r < h; r++) {
    int i = 0;
    while (i < w) {
      if (img[r * w + i] == TK_FOND) { i++; continue; }
      int j = i;
      while (j < w && img[r * w + j] != TK_FOND) j++;
      tft.drawRGBBitmap(x + i, y + r, (uint16_t *)img + r * w + i, j - i, 1);
      i = j;
    }
  }
}

void tkChargerImages() {
  eTkAssets = SD.exists("/spidey/logo.bmp") && SD.exists("/spidey/head.bmp") && SD.exists("/spidey/pin_v.bmp") && SD.exists("/spidey/pin_r.bmp");
  if (eTkAssets) eTkAssets = tkBmp("/spidey/pin_v.bmp", 0, 0, 0, 16, eTkPinRam[0]) && tkBmp("/spidey/pin_r.bmp", 0, 0, 0, 16, eTkPinRam[1]);
  log(String("Tracker : images ") + (eTkAssets ? "officielles (SD)" : "dessinees par le code"));
}

// redessine les points de terre dans un rectangle de la carte (efface un repere)
void tkCarteZone(int x0, int y0, int x1, int y1) {
  x0 = max(0, x0); y0 = max(0, y0); x1 = min(239, x1); y1 = min(119, y1);
  tft.fillRect(x0, TK_Y0 + y0, x1 - x0 + 1, y1 - y0 + 1, TK_FOND);
  for (int r = y0 / 4; r <= y1 / 4 && r < TK_ROWS; r++)
    for (int c = x0 / 4; c <= x1 / 4 && c < TK_COLS; c++)
      if (TK_CARTE[r][c] == '#') tft.fillRect(c * 4, TK_Y0 + r * 4, 3, 3, TK_POINT);
}

void tkPin(int i, bool allume) {
  int x = tkX(i), y = TK_Y0 + tkY(i);
  if (!allume) { tkCarteZone(tkX(i) - 9, tkY(i) - 18, tkX(i) + 9, tkY(i) + 8); return; }
  if (i == TK_NB) {   // l'iPod : un losange blanc entoure d'un anneau
    tft.fillTriangle(x, y - 5, x - 4, y, x + 4, y, C_BLANC);
    tft.fillTriangle(x, y + 5, x - 4, y, x + 4, y, C_BLANC);
    tft.drawCircle(x, y, 7, C_BLANC);
  } else if (eTkAssets) {   // pastille officielle 16x16, pointe en bas au centre
    tkRam(eTkPinRam[TK_PINS[i].confirme ? 0 : 1], x - 8, y - 16, 16, 16);
  } else {
    uint16_t c = TK_PINS[i].confirme ? TK_VERT : TK_ROUGE;
    tft.fillCircle(x, y, 3, c);
    tft.drawCircle(x, y, 6, c);
  }
}

void tkSpider(int cx, int cy, uint16_t c) {   // araignee simplifiee : corps, tete, 8 pattes (secours)
  tft.fillCircle(cx, cy + 1, 3, c);
  tft.fillCircle(cx, cy - 3, 2, c);
  static const int8_t pattes[4][2] = {{-7, -6}, {-9, -1}, {-8, 4}, {-5, 8}};
  for (auto &p : pattes) {
    tft.drawLine(cx, cy + 1, cx + p[0], cy + p[1], c);
    tft.drawLine(cx, cy + 1, cx - p[0], cy + p[1], c);
  }
}

void tkMasque(int cx, int cy) {   // masque simplifie : tete rouge, toile, deux yeux blancs inclines (secours)
  tft.fillCircle(cx, cy, 10, TK_ROUGE);
  for (int a = 0; a < 8; a++) {
    float r = a * 0.785398f;
    tft.drawLine(cx, cy, cx + (int)(10 * cosf(r)), cy + (int)(10 * sinf(r)), 0x8000);
  }
  tft.drawCircle(cx, cy, 5, 0x8000);
  tft.fillTriangle(cx - 9, cy - 5, cx - 1, cy - 1, cx - 8, cy + 3, C_BLANC);
  tft.fillTriangle(cx + 9, cy - 5, cx + 1, cy - 1, cx + 8, cy + 3, C_BLANC);
  tft.drawTriangle(cx - 9, cy - 5, cx - 1, cy - 1, cx - 8, cy + 3, C_NOIR);
  tft.drawTriangle(cx + 9, cy - 5, cx + 1, cy - 1, cx + 8, cy + 3, C_NOIR);
}

void trackerEntete() {
  tft.fillRect(0, 0, 240, 52, TK_FOND);
  if (eTkAssets) tkBmp("/spidey/logo.bmp", 4, 0, 0, 39, nullptr);   // logo officiel 232 x 39
  else {
    tkSpider(14, 10, TK_ROUGE);
    tft.setTextSize(2);
    tft.setTextColor(C_BLANC, TK_FOND);
    tft.setCursor(30, 3);
    tft.print("SPIDEY TRACKER");
  }
  tft.setTextSize(1);
  tft.setTextColor(TK_GRIS, TK_FOND);
  tft.setCursor(6, 43);
  tft.print("NOW PLAYING: " + ajuster(morceauCourant().titre, 14));
  if (enPause) { tft.fillRect(222, 42, 3, 8, C_BLANC); tft.fillRect(228, 42, 3, 8, C_BLANC); }
  else tft.fillTriangle(222, 42, 222, 50, 230, 46, C_BLANC);
}

void tkCompteurs() {
  tft.fillRect(0, 189, 240, 10, TK_FOND);
  tft.setTextSize(1);
  tft.setTextColor(TK_VERT, TK_FOND); tft.setCursor(6, 190); tft.print("CONFIRMED " + String(eTkNbConf));
  tft.setTextColor(TK_ROUGE, TK_FOND); tft.setCursor(96, 190); tft.print("RUMORED " + String(eTkNbRum));
}

// carte, legende, compteurs et journal vides : l'animation repart de zero
void trackerCorps() {
  tft.fillRect(0, TK_Y0, 240, 266, TK_FOND);
  tkCarteZone(0, 0, 239, 119);
  // legende
  tft.setTextSize(1); tft.setTextColor(C_BLANC, TK_FOND);
  tft.fillCircle(10, 181, 3, TK_VERT);  tft.setCursor(18, 178); tft.print("CONFIRMED");
  tft.fillCircle(92, 181, 3, TK_ROUGE); tft.setCursor(100, 178); tft.print("RUMORED");
  tft.fillTriangle(160, 176, 156, 181, 164, 181, C_BLANC); tft.fillTriangle(160, 186, 156, 181, 164, 181, C_BLANC); tft.setCursor(170, 178); tft.print("YOU");
  eTkNbConf = eTkNbRum = 0;
  tkCompteurs();
  tft.setTextColor(C_BLANC, TK_FOND); tft.setCursor(6, 203); tft.print("ACTIVITY LOG");
  tft.drawFastHLine(6, 213, 228, TK_GRIS);
  tft.setTextColor(TK_GRIS, TK_FOND); tft.setCursor(6, 290); tft.print("BRAND NEW DAY - 31 JUL 2026");
  for (int i = 0; i <= TK_NB; i++) { eTkEtat[i] = -1; eTkCar[i] = 0; }
  eTkFrame = -1;
  if (!eTkAssets) tkMasque(TK_TETE_X + 16, TK_TETE_Y + 30);   // personnage de secours (fixe)
  eTkDebut = millis();
}

void dessinerLectureTracker() {
  tkChargerImages();
  tft.fillRect(0, 0, 240, 320, TK_FOND);
  trackerEntete();
  trackerCorps();
  dernierTemps = derniereDuree = 0xFFFFFFFF;
  majProgression();
  majVolume();
}

void trackerProgression(uint32_t t, uint32_t d) {
  const int x = 6, y = 306, l = 150, h = 6;
  int plein = d ? min(l - 2, (int)((l - 2) * t / d)) : 0;
  tft.drawRect(x, y, l, h, TK_GRIS);
  tft.fillRect(x + 1, y + 1, plein, h - 2, TK_ROUGE);
  tft.fillRect(x + 1 + plein, y + 1, l - 2 - plein, h - 2, TK_FOND);
  tft.fillRect(162, 304, 40, 10, TK_FOND);
  tft.setTextSize(1);
  tft.setTextColor(C_BLANC, TK_FOND);
  tft.setCursor(162, 305);
  tft.print(minSec(t));
}

void trackerVolume() {
  verrouiller();
  int v = audio.getVolume();
  deverrouiller();
  tft.fillRect(196, 304, 44, 10, TK_FOND);
  tft.setTextSize(1);
  tft.setTextColor(TK_GRIS, TK_FOND);
  tft.setCursor(198, 305);
  tft.print("VOL " + String(v));
}

// animation en boucle de 18 s : un repere apparait toutes les 2,5 s (il clignote, puis reste), son entree du journal se tape,
// enfin "POSITION ACTUELLE : TON IPOD" ; l'ensemble repart de zero. Le personnage tourne la tete en continu (10 images par seconde).
void majTracker() {
  if (enPause) return;
  uint32_t t = millis();
  if (t - eTkDernier < 50) return;
  eTkDernier = t;
  uint32_t e = t - eTkDebut;
  if (e >= TK_BOUCLE_MS) { trackerCorps(); return; }
  if (eTkAssets) {
    int fr = (e / 100) % TK_TETE_N;
    if (fr != eTkFrame) { tkBmp("/spidey/head.bmp", TK_TETE_X, TK_TETE_Y, fr * TK_TETE_L, TK_TETE_L, nullptr); eTkFrame = fr; }
  }
  for (int i = 0; i <= TK_NB; i++) {
    if (e < tkDebut(i)) continue;
    uint32_t age = e - tkDebut(i);
    if (eTkEtat[i] == -1) {
      if (i < TK_NB) { if (TK_PINS[i].confirme) eTkNbConf++; else eTkNbRum++; tkCompteurs(); }
    }
    bool allume = age >= 1600 || (age / 200) % 2 == 0;
    if (eTkEtat[i] != (allume ? 1 : 0)) { tkPin(i, allume); eTkEtat[i] = allume ? 1 : 0; }
    String txt = tkTexte(i);
    int nb = min((int)txt.length(), (int)(age / 20));
    if (nb != eTkCar[i]) {
      tft.setTextSize(1);
      tft.setTextColor(i == TK_NB ? C_BLANC : (TK_PINS[i].confirme ? TK_VERT : TK_ROUGE), TK_FOND);
      tft.setCursor(6, TK_LOG_Y + i * 14);
      tft.print(txt.substring(0, nb));
      eTkCar[i] = nb;
    }
  }
}

// un appui sur un bouton, en Lecture en cours (avant que l'action ne soit faite : le volume est encore l'ancien)
void easterTouche(char c) {
  if (c != '+' && c != '-') { eNb = 0; return; }
  if (eNb == 12) { memmove(eHist, eHist + 1, 11 * sizeof(Touche)); eNb = 11; }
  verrouiller();
  eHist[eNb].vol = audio.getVolume();
  deverrouiller();
  eHist[eNb++].c = c;
  for (unsigned k = 0; k < eCodes.size(); k++) {
    int L = eCodes[k].seq.length();
    if (L > eNb) continue;
    bool ok = true;
    for (int i = 0; i < L && ok; i++) ok = eHist[eNb - L + i].c == eCodes[k].seq[i];
    if (ok) { eCodeTrouve = k; eCodeVolume = eHist[eNb - L].vol; eNb = 0; return; }
  }
}

// un appui pendant l'ecran secret : on le ferme (apres 1 s, pour ne pas le fermer en relachant le bouton du code)
bool easterEcranToucheur() {
  if (!eEcranFin) return false;
  if (millis() - eEcranDebut > 1000) eEcranFin = millis();
  return true;
}

// Barre de progression "eau" : la surface ondule, ecume claire dessus, fond plus sombre ; le bord avant s'amincit
static uint16_t couleurMelange(uint16_t c, int pour256, int blanc) {   // pour256 : 256 = inchange ; blanc : 0..255 de blanc ajoute
  int r = ((c >> 11) & 31) << 3, g = ((c >> 5) & 63) << 2, b = (c & 31) << 3;
  r = min(255, r * pour256 / 256 + blanc); g = min(255, g * pour256 / 256 + blanc); b = min(255, b * pour256 / 256 + blanc);
  return tft.color565(r, g, b);
}

void majBarreEau() {
  if (eBarre != "eau" || pile.back().type != E_LECTURE || posFile < 0 || eEcranFin) return;
  uint32_t t = millis();
  if (t - eBarreDernier < 50) return;
  eBarreDernier = t;
  const int x = 16, y = 244, l = 208, h = 9;
  if (!eToileBarre) eToileBarre = new GFXcanvas16(l, h);
  if (!eToileBarre->getBuffer()) return;
  uint32_t tps = dernierTemps == 0xFFFFFFFF ? 0 : dernierTemps, dur = derniereDuree == 0xFFFFFFFF ? 0 : derniereDuree;
  int plein = dur ? min(l - 2, (int)((l - 2) * tps / dur)) : 0;
  uint16_t eau = eAccent, ecume = couleurMelange(eAccent, 256, 150), fond = couleurMelange(eAccent, 140, 0);
  eToileBarre->fillScreen(eFond);
  eToileBarre->drawRect(0, 0, l, h, eGris);
  for (int cx = 0; cx < plein; cx++) {
    float s = sinf(cx * 0.22f + t * 0.008f) + 0.5f * sinf(cx * 0.5f - t * 0.011f);
    float haut = 2.0f + 1.2f * s;                      // niveau de la surface (ligne 1 = haut du cadre)
    int reste = plein - cx;
    if (reste < 10) haut += (10 - reste) * 0.5f;       // le bord avant s'amincit
    int top = constrain((int)haut, 1, 7);
    eToileBarre->drawFastVLine(1 + cx, top, 8 - top, eau);
    eToileBarre->drawFastVLine(1 + cx, 6, 2, fond);
    eToileBarre->drawPixel(1 + cx, top, ecume);
  }
  tft.drawRGBBitmap(x, y, eToileBarre->getBuffer(), l, h);
}

// Style Mac 1997 : le titre est tape lettre par lettre (130 ms par lettre), puis le curseur clignote
void majMac() {
  uint32_t t = millis();
  if (t - eDernier < 60) return;
  eDernier = t;
  int n = sansAccents(morceauCourant().titre).length();
  int cible = t - eTapeDebut < 600 ? 0 : min(n, (int)((t - eTapeDebut - 600) / 130) + 1);
  bool vis = cible < n || ((t / 500) % 2 == 0);
  if (cible != eTape || (cible == n && vis != eCurseur)) {
    eTape = cible; eCurseur = vis;
    macTitre(eTape, vis);
  }
}

// Fond "courants" : des lignes ondulantes coulent vers le bas dans les deux marges de la pochette (50 px de large chacune)
void majFondCourants() {
  if (enPause) return;
  uint32_t t = millis();
  if (t - eFondDernier < 70) return;
  eFondDernier = t;
  const int W = 50, H = 156, Y0 = 26;   // entre l'entete et le titre du morceau
  if (!eToileFond) eToileFond = new GFXcanvas16(W, H);
  if (!eToileFond->getBuffer()) return;
  uint16_t cols[3] = {eAccent, couleurMelange(eAccent, 200, 50), eGris};
  for (int cote = 0; cote < 2; cote++) {
    eToileFond->fillScreen(eFond);
    for (int i = 0; i < 5; i++) {
      int base = 5 + i * 9;
      for (int y = 0; y < H; y++) {
        float x = base + 5.0f * sinf(y * 0.05f - t * 0.002f + i * 1.3f + cote * 2.0f) + 2.5f * sinf(y * 0.13f + t * 0.0011f + i);
        int xi = cote ? W - 2 - (int)x : (int)x;
        eToileFond->drawPixel(xi, y, cols[i % 3]);
        eToileFond->drawPixel(xi + 1, y, cols[i % 3]);
      }
    }
    tft.drawRGBBitmap(cote ? 240 - W : 0, Y0, eToileFond->getBuffer(), W, H);
  }
}

// "Disque raye" : pendant les passages qui sautent, des rayures s'accumulent sur la pochette, le titre tremble
// et la barre de progression recule par a-coups. A la fin d'un passage les rayures "guerissent" en 8 bandes
// (la pochette est restauree depuis sa copie en memoire, sans la redecoder).
// Le temps de lecture est connu a la seconde : on l'affine avec millis() entre deux changements de seconde.
void majRayure() {
  if (eFenetres.empty() || enPause) return;
  uint32_t t = millis();
  if (t - eRayDernier < 50) return;
  eRayDernier = t;
  verrouiller();
  uint32_t s = audio.getAudioCurrentTime();
  deverrouiller();
  if (s != eRefS) { eRefS = s; eRefMs = t; }
  int32_t est = (int32_t)(s * 1000 + (t - eRefMs)) - 100;   // -100 ms : le son entendu est un peu en retard sur le decodage
  bool dans = false;
  for (auto &f : eFenetres) if (est >= (int32_t)f.a && est < (int32_t)f.b) dans = true;

  if (dans) {
    eRayActif = true; eGuerison = -1;
    if (t - eRayFrame < 100) return;
    eRayFrame = t;
    for (int i = 0; i < 2; i++) {   // rayures fines sur la pochette
      int x = POCHETTE_X + random(POCHETTE_T), y = POCHETTE_Y + random(POCHETTE_T);
      int x2 = constrain(x + random(25, 80) * (random(2) ? 1 : -1), POCHETTE_X, POCHETTE_X + POCHETTE_T - 1);
      int y2 = constrain(y + random(-8, 9), POCHETTE_Y, POCHETTE_Y + POCHETTE_T - 1);
      tft.drawLine(x, y, x2, y2, random(3) ? C_BLANC : eAccent);
    }
    titreDecale(random(-5, 6), random(3) ? eTexte : eAccent);   // le titre tremble
    const int x = 16, y = 244, l = 208, h = 9;                    // la barre recule par a-coups
    uint32_t dur = derniereDuree == 0xFFFFFFFF ? 0 : derniereDuree;
    int plein = dur ? min(l - 2, (int)((l - 2) * s / dur)) : 0;
    tft.fillRect(x + 1, y + 1, l - 2, h - 2, eFond);
    tft.fillRect(x + 1, y + 1, max(0, plein - (int)random(0, 9)), h - 2, eAccent);
    return;
  }
  if (eRayActif) { eRayActif = false; eGuerison = 0; eGuerisonT = 0; }
  if (eGuerison >= 0 && t - eGuerisonT >= 40) {   // guerison : une bande de pochette par 40 ms
    eGuerisonT = t;
    if (pochetteEnCache && pochetteCache) {
      int y0 = eGuerison * 18, n = min(18, POCHETTE_T - y0);
      if (n > 0) tft.drawRGBBitmap(POCHETTE_X, POCHETTE_Y + y0, pochetteCache + y0 * POCHETTE_T, POCHETTE_T, n);
    } else if (eGuerison == 0) dessinerPochette();   // pas de copie : on la redessine d'un coup
    eGuerison++;
    if (eGuerison >= 8) {
      eGuerison = -1;
      titreDecale(0, eTexte);
      dernierTemps = derniereDuree = 0xFFFFFFFF;   // redessine la barre normale
      majProgression();
    }
  }
}

// appele a chaque tour de loop()
void majEaster() {
  if (pile.back().type != E_LECTURE || posFile < 0) return;
  uint32_t t = millis();
  if (eCodeTrouve >= 0) {
    int k = eCodeTrouve; eCodeTrouve = -1;
    verrouiller(); audio.setVolume(eCodeVolume); deverrouiller();
    log("Easter : code secret " + eCodes[k].seq);
    if (eCodes[k].texte.startsWith("@zx")) lancerZX(); else lancerEcranSecret(eCodes[k].texte);
    return;
  }
  if (eEcranFin) {
    if (t >= eEcranFin) { eEcranFin = 0; dessinerLecture(); }
    else if (eMode == 1) animerLancement();
    else if (eMode == 2) animerZX();
    else animerEcranSecret();
    return;
  }
  if (eAnimAttente.length()) {
    String nom = eAnimAttente; eAnimAttente = "";
    if (nom == "xero" || nom == "notok") { lancerLancement(nom); return; }
  }
  if (eStyle == "mac") { majMac(); return; }
  if (eStyle == "tracker") { majTracker(); return; }
  majRayure();
  if (eFondAnime == "courants") majFondCourants();
  majBarreEau();
  if (!eEffet.length() || t - eDernier < 50) return;
  eDernier = t;
  majBande();
}
