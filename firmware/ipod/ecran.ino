// Ecran ILI9341 240x320 : style iPod Classic (fond blanc, barre grise, selection bleue)


void initEcran() {
  tft.begin(40000000);
  tft.setRotation(0);   // portrait 240x320
  tft.fillScreen(C_BLANC);
  tft.setTextWrap(false);
}

// l'ecran perd ses reglages (blanc) quand le WiFi fait chuter le 3,3 V : on le reinitialise et on redessine
void reinitEcran() {
  initEcran();
  dessinerEcran();
}

// la police de l'ecran n'a pas d'accents : lettres accentuees (UTF-8) -> lettre simple
String sansAccents(const String &s) {
  static const char *de[] = {"é","è","ê","ë","à","â","ä","î","ï","ô","ö","ù","û","ü","ç","ñ",
                             "É","È","Ê","À","Â","Î","Ô","Ç","’","‘","“","”","–","—","…"};
  static const char *vers[] = {"e","e","e","e","a","a","a","i","i","o","o","u","u","u","c","n",
                               "E","E","E","A","A","I","O","C","'","'","\"","\"","-","-","..."};
  String r = s;
  for (unsigned i = 0; i < sizeof(de) / sizeof(de[0]); i++) r.replace(de[i], vers[i]);
  String propre;
  for (unsigned i = 0; i < r.length(); i++) if ((uint8_t)r[i] < 0x80) propre += r[i];   // le reste est ignore
  return propre;
}

// texte sans accents, coupe a maxCar caracteres
String ajuster(const String &s, int maxCar) {
  String r = sansAccents(s);
  return (int)r.length() <= maxCar ? r : r.substring(0, maxCar - 2) + "..";
}

void texteCentre(const String &s, int y, uint8_t taille, uint16_t couleur, uint16_t fond) {
  String r = ajuster(s, 236 / (6 * taille));
  tft.setTextSize(taille);
  tft.setTextColor(couleur, fond);
  tft.setCursor((240 - r.length() * 6 * taille) / 2, y);
  tft.print(r);
}

// ------------------------------------------------------------------ Barre du haut
void dessinerEntete() {
  if (pile.back().type == E_LECTURE && eStyle == "mac") { macBarreMenu(); return; }
  if (pile.back().type == E_LECTURE && eStyle == "tracker") { trackerEntete(); return; }
  for (int y = 0; y < HAUT_ENTETE; y++) {   // degrade gris clair -> gris
    uint8_t g = 240 - y * 2;
    tft.drawFastHLine(0, y, 240, tft.color565(g, g, g + 5 > 255 ? 255 : g + 5));
  }
  tft.drawFastHLine(0, HAUT_ENTETE - 1, 240, 0x8410);
  String titre = ajuster(pile.back().titre, 15);
  tft.setTextSize(2);
  tft.setTextColor(C_NOIR);
  tft.setCursor((240 - titre.length() * 12) / 2, 4);
  tft.print(titre);

  // icone lecture / pause a gauche
  if (posFile >= 0) {
    if (enPause) {
      tft.fillRect(8, 7, 3, 10, C_BLEU);
      tft.fillRect(14, 7, 3, 10, C_BLEU);
    } else {
      tft.fillTriangle(8, 6, 8, 17, 17, 11, C_BLEU);
    }
  }
}

// ------------------------------------------------------------------ Listes
bool ligneAvecFleche() {
  TypeEcran t = pile.back().type;
  return t == E_ARTISTES || t == E_ALBUMS || (t == E_MENU);   // E_TRANSFERT : pas de fleche
}

void dessinerLigne(int i) {
  Ecran &e = pile.back();
  int rang = i - e.haut;
  if (rang < 0 || rang >= LIGNES_VISIBLES || i >= (int)lignes.size()) return;
  bool defile = lignes.size() > LIGNES_VISIBLES;
  int largeur = defile ? 234 : 240;
  int y = HAUT_ENTETE + rang * HAUT_LIGNE;
  bool sel = (i == e.sel);
  uint16_t fond = sel ? C_BLEU : C_BLANC, texte = sel ? C_BLANC : C_NOIR;

  tft.fillRect(0, y, largeur, HAUT_LIGNE, fond);
  tft.setTextSize(2);
  tft.setTextColor(texte);
  tft.setCursor(8, y + 5);
  tft.print(ajuster(lignes[i], (largeur - 30) / 12));
  if (ligneAvecFleche()) {   // chevron ">" : ouvre un sous-menu
    int x = largeur - 14, yc = y + HAUT_LIGNE / 2;
    tft.drawLine(x, yc - 5, x + 5, yc, texte);
    tft.drawLine(x, yc + 5, x + 5, yc, texte);
    tft.drawLine(x + 1, yc - 5, x + 6, yc, texte);
    tft.drawLine(x + 1, yc + 5, x + 6, yc, texte);
  }
}

void dessinerAscenseur() {
  if (lignes.size() <= LIGNES_VISIBLES) return;
  Ecran &e = pile.back();
  int zone = LIGNES_VISIBLES * HAUT_LIGNE;
  int h = max(12, (int)(zone * LIGNES_VISIBLES / lignes.size()));
  int y = HAUT_ENTETE + (zone - h) * e.haut / (lignes.size() - LIGNES_VISIBLES);
  tft.fillRect(234, HAUT_ENTETE, 6, zone, C_GRIS_CLAIR);
  tft.fillRect(235, y, 4, h, C_GRIS_TEXTE);
}

void dessinerListe() {
  Ecran &e = pile.back();
  for (int i = e.haut; i < e.haut + LIGNES_VISIBLES; i++) {
    if (i < (int)lignes.size()) dessinerLigne(i);
    else tft.fillRect(0, HAUT_ENTETE + (i - e.haut) * HAUT_LIGNE, 240, HAUT_LIGNE, C_BLANC);
  }
  tft.fillRect(0, HAUT_ENTETE + LIGNES_VISIBLES * HAUT_LIGNE, 240, 320, C_BLANC);
  dessinerAscenseur();
  if (lignes.empty()) texteCentre("(vide)", 150, 2, C_GRIS_TEXTE, C_BLANC);
}

void dessinerEcran() {
  if (pile.back().type == E_LECTURE) { dessinerLecture(); return; }
  if (pile.back().type == E_APROPOS) { dessinerAPropos(); return; }
  dessinerEntete();
  dessinerListe();
}

// ------------------------------------------------------------------ Logo et ecran "A propos"
// logo : un "ok" en minuscules entoure d'un cercle (dessine par le code)
void logoOk(int cx, int cy, int r, uint16_t couleur, uint16_t fond, uint8_t taille = 5) {
  for (int i = 0; i < 4; i++) tft.drawCircle(cx, cy, r - i, couleur);
  tft.setTextSize(taille);
  tft.setTextColor(couleur, fond);
  tft.setCursor(cx - 6 * taille, cy - 4 * taille);   // 2 lettres de 6*taille px de large sur 8*taille de haut
  tft.print("ok");
}

void dessinerAPropos() {
  tft.fillRect(0, 0, 240, 320, C_BLANC);
  dessinerEntete();
  logoOk(120, 68, 36, C_BLEU, C_BLANC, 4);
  texteCentre("iPod OK Computer Edition", 112, 1, C_NOIR, C_BLANC);
  texteCentre("NUMERO DE SERIE", 132, 1, C_GRIS_TEXTE, C_BLANC);
  texteCentre(NUMERO_SERIE, 143, 2, C_NOIR, C_BLANC);
  texteCentre("MODELE", 168, 1, C_GRIS_TEXTE, C_BLANC);
  texteCentre(MODELE, 179, 2, C_NOIR, C_BLANC);
  texteCentre("CREE PAR", 204, 1, C_GRIS_TEXTE, C_BLANC);
  texteCentre(AUTEUR, 215, 2, C_NOIR, C_BLANC);
  texteCentre("DEBUT DU PROJET", 240, 1, C_GRIS_TEXTE, C_BLANC);
  texteCentre(DEBUT_PROJET, 251, 2, C_NOIR, C_BLANC);
  texteCentre(String(biblio.size()) + " morceaux - SD " + String(SD.cardSize() / 1000000000.0, 1) + " Go", 284, 1, C_GRIS_TEXTE, C_BLANC);
  texteCentre("ESP32-S3 - 16 Mo flash - 8 Mo PSRAM", 298, 1, C_GRIS_TEXTE, C_BLANC);
}

// ------------------------------------------------------------------ Style Mac 1997 (dessine par le code)
// barre de menus blanche, bureau bleu-gris, fenetre gris platine a rayures avec case de fermeture, texte noir
#define MAC_BUREAU 0x5393   // RGB 86,112,152
#define MAC_GRIS   0xC618   // gris platine
#define MAC_OMBRE  0x8410
#define MAC_RAYURE 0xAD55
#define MAC_BLEU   0x3AF4   // remplissage de la barre de progression

void macBarreMenu() {
  tft.fillRect(0, 0, 240, 17, C_BLANC);
  tft.drawFastHLine(0, 17, 240, C_NOIR);
  tft.setTextSize(1);
  tft.setTextColor(C_NOIR, C_BLANC);
  tft.setCursor(8, 5);
  tft.print("Fichier  Edition  Presentation");
  if (posFile >= 0) {   // lecture / pause a droite
    if (enPause) { tft.fillRect(222, 4, 3, 9, C_NOIR); tft.fillRect(228, 4, 3, 9, C_NOIR); }
    else tft.fillTriangle(222, 4, 222, 13, 230, 8, C_NOIR);
  }
}

// titre tape dans la zone de texte : n lettres, avec ou sans curseur (bloc noir)
void macTitre(int n, bool curseur) {
  String t = ajuster(morceauCourant().titre, 16);
  tft.fillRect(24, 72, 192, 22, C_BLANC);
  tft.setTextSize(2);
  tft.setTextColor(C_NOIR, C_BLANC);
  tft.setCursor(28, 76);
  tft.print(t.substring(0, n));
  if (curseur) tft.fillRect(28 + n * 12, 76, 8, 14, C_NOIR);
}

void macProgression(uint32_t t, uint32_t d) {
  const int x = 24, y = 196, l = 192, h = 12;
  int plein = d ? min(l - 2, (int)((l - 2) * t / d)) : 0;
  tft.drawRect(x, y, l, h, C_NOIR);
  tft.fillRect(x + 1, y + 1, l - 2, h - 2, C_BLANC);
  tft.fillRect(x + 1, y + 1, plein, h - 2, MAC_BLEU);
  for (int i = 0; i < plein; i += 6) tft.drawFastVLine(x + 1 + i, y + 1, h - 2, MAC_BUREAU);   // rayures
  tft.fillRect(x, y + 16, l, 10, MAC_GRIS);
  tft.setTextSize(1);
  tft.setTextColor(C_NOIR, MAC_GRIS);
  tft.setCursor(x, y + 17);
  tft.print(minSec(t));
  String reste = "-" + minSec(d > t ? d - t : 0);
  tft.setCursor(x + l - reste.length() * 6, y + 17);
  tft.print(reste);
}

void macVolume() {
  verrouiller();
  int v = audio.getVolume();
  deverrouiller();
  tft.fillRect(24, 236, 192, 10, MAC_GRIS);
  tft.setTextSize(1);
  tft.setTextColor(C_NOIR, MAC_GRIS);
  tft.setCursor(24, 237);
  tft.print("Volume : " + String(v) + " / " + String(VOLUME_MAX));
}

void dessinerLectureMac() {
  Morceau &m = morceauCourant();
  tft.fillRect(0, 18, 240, 302, MAC_BUREAU);
  macBarreMenu();
  tft.fillRect(15, 44, 216, 232, C_NOIR);                       // ombre de la fenetre
  tft.fillRect(12, 40, 216, 232, MAC_GRIS);
  tft.drawRect(12, 40, 216, 232, C_NOIR);
  for (int y = 42; y < 58; y++) tft.drawFastHLine(14, y, 212, (y % 2) ? C_BLANC : MAC_RAYURE);   // barre de titre rayee
  tft.drawFastHLine(13, 58, 214, C_NOIR);
  tft.fillRect(18, 44, 11, 11, MAC_GRIS); tft.drawRect(18, 44, 11, 11, C_NOIR);                  // case de fermeture
  const char *titre = "Lecture en cours";
  int w = strlen(titre) * 6 + 8;
  tft.fillRect((240 - w) / 2, 43, w, 14, MAC_GRIS);
  tft.setTextSize(1);
  tft.setTextColor(C_NOIR, MAC_GRIS);
  tft.setCursor((240 - w) / 2 + 4, 46);
  tft.print(titre);
  // zone de texte en creux
  tft.fillRect(20, 66, 200, 112, C_BLANC);
  tft.drawFastHLine(20, 66, 200, MAC_OMBRE); tft.drawFastVLine(20, 66, 112, MAC_OMBRE);
  tft.drawFastHLine(20, 177, 200, C_NOIR); tft.drawFastVLine(219, 66, 112, C_NOIR);
  tft.setTextColor(C_NOIR, C_BLANC);
  tft.setCursor(28, 104); tft.print(ajuster(m.artiste, 31));
  tft.setCursor(28, 118); tft.print(ajuster(m.album, 31));
  tft.setCursor(28, 132); tft.print("Piste " + String(posFile + 1) + " sur " + String(fileLecture.size()));
  tft.setTextColor(MAC_OMBRE, C_BLANC);
  tft.setCursor(28, 154); tft.print("Voix : Fred (SimpleText)");
  macTitre(0, false);
  dernierTemps = derniereDuree = 0xFFFFFFFF;
  majProgression();
  majVolume();
  easterLecture();   // le titre se tape
}

// ------------------------------------------------------------------ Lecture en cours
uint32_t dernierTemps = 0xFFFFFFFF, derniereDuree = 0xFFFFFFFF;

void dessinerLecture() {
  if (posFile >= 0 && eStyle == "mac") { dessinerLectureMac(); return; }
  if (posFile >= 0 && eStyle == "tracker") { dessinerLectureTracker(); return; }
  tft.fillRect(0, HAUT_ENTETE, 240, 320 - HAUT_ENTETE, eFond);
  dessinerEntete();
  if (posFile < 0) { texteCentre("Rien en lecture", 150, 2, eGris, eFond); return; }
  Morceau &m = morceauCourant();
  texteCentre((eBanniere.length() ? eBanniere + "  -  " : String("")) + String(posFile + 1) + " sur " + String(fileLecture.size()), 27, 1,
              eBanniere.length() ? eAccent : eGris, eFond);
  dessinerPochette();
  texteCentre(m.titre, 186, 2, eTexte, eFond);
  texteCentre(m.artiste, 208, 1, eTexte, eFond);
  texteCentre(m.album, 222, 1, eGris, eFond);
  dernierTemps = derniereDuree = 0xFFFFFFFF;   // force le dessin de la barre
  majProgression();
  majVolume();
  easterLecture();   // bande animee de l'album
}

String minSec(uint32_t s) {
  char txt[12];
  snprintf(txt, sizeof(txt), "%u:%02u", s / 60, s % 60);
  return txt;
}

void majProgression() {
  if (pile.back().type != E_LECTURE || posFile < 0) return;
  verrouiller();
  uint32_t t = audio.getAudioCurrentTime(), d = audio.getAudioFileDuration();
  deverrouiller();
  if (t == dernierTemps && d == derniereDuree) return;
  dernierTemps = t; derniereDuree = d;
  if (eStyle == "mac") { macProgression(t, d); return; }
  if (eStyle == "tracker") { trackerProgression(t, d); return; }

  const int x = 16, y = 244, l = 208, h = 9;
  int plein = d ? min(l - 2, (int)((l - 2) * t / d)) : 0;
  if (eBarre != "eau") {   // "eau" : barre dessinee par easter.ino (majBarreEau)
    tft.drawRect(x, y, l, h, eGris);
    tft.fillRect(x + 1, y + 1, plein, h - 2, eAccent);
    tft.fillRect(x + 1 + plein, y + 1, l - 2 - plein, h - 2, eFond);
  }

  tft.setTextSize(1);
  tft.setTextColor(eTexte, eFond);
  tft.setCursor(x, y + 14);
  tft.print(minSec(t) + "  ");
  String reste = "-" + minSec(d > t ? d - t : 0);
  tft.fillRect(170, y + 14, 70, 8, eFond);
  tft.setCursor(x + l - reste.length() * 6, y + 14);
  tft.print(reste);
}

void majVolume() {
  if (eStyle == "mac") { macVolume(); return; }
  if (eStyle == "tracker") { trackerVolume(); return; }
  const int y = 292, x = 40, l = 184;
  // petit haut-parleur
  tft.fillRect(16, y, 5, 8, eGris);
  tft.fillTriangle(20, y + 4, 26, y - 2, 26, y + 10, eGris);
  verrouiller();
  int v = audio.getVolume();
  deverrouiller();
  int plein = (l - 2) * v / VOLUME_MAX;   // barre pleine = volume max autorise
  tft.drawRect(x, y, l, 8, eGris);
  tft.fillRect(x + 1, y + 1, plein, 6, eGris);
  tft.fillRect(x + 1 + plein, y + 1, l - 2 - plein, 6, eFond);
}

// ------------------------------------------------------------------ Pochette
// Ordre de recherche : image integree au MP3 (tag APIC), /covers/<album>.jpg|.bmp,
// cover.jpg / folder.jpg / cover.bmp dans le dossier du morceau, /covers/cover.bmp, sinon une note
// le JPEG est d'abord decode dans une image en PSRAM, puis redimensionne pour remplir le cadre
uint16_t *pochetteCache = nullptr;   // alloue par easter.ino quand un effet doit pouvoir restaurer la pochette
bool pochetteEnCache = false;
uint16_t *jpgImage = nullptr;
int jpgL = 0, jpgH = 0;

bool blocPochette(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
  for (int j = 0; j < h && y + j < jpgH; j++)
    for (int i = 0; i < w && x + i < jpgL; i++) jpgImage[(y + j) * jpgL + x + i] = bitmap[j * w + i];
  return true;
}

// redimensionnement bilineaire (RGB565) de jpgImage vers un carre de POCHETTE_T pixels
void dessinerImageRedim() {
  uint16_t ligne[POCHETTE_T];
  for (int oy = 0; oy < POCHETTE_T; oy++) {
    int32_t fy = jpgH > 1 ? (int32_t)oy * (jpgH - 1) * 256 / (POCHETTE_T - 1) : 0;
    int y0 = fy >> 8, y1 = min(y0 + 1, jpgH - 1), dy = fy & 255;
    for (int ox = 0; ox < POCHETTE_T; ox++) {
      int32_t fx = jpgL > 1 ? (int32_t)ox * (jpgL - 1) * 256 / (POCHETTE_T - 1) : 0;
      int x0 = fx >> 8, x1 = min(x0 + 1, jpgL - 1), dx = fx & 255;
      uint16_t p[4] = {jpgImage[y0 * jpgL + x0], jpgImage[y0 * jpgL + x1],
                       jpgImage[y1 * jpgL + x0], jpgImage[y1 * jpgL + x1]};
      int poids[4] = {(256 - dx) * (256 - dy), dx * (256 - dy), (256 - dx) * dy, dx * dy};
      uint32_t r = 0, g = 0, b = 0;
      for (int k = 0; k < 4; k++) {
        r += (p[k] >> 11) * poids[k];
        g += ((p[k] >> 5) & 0x3F) * poids[k];
        b += (p[k] & 0x1F) * poids[k];
      }
      ligne[ox] = ((r >> 16) << 11) | ((g >> 16) << 5) | (b >> 16);
    }
    if (pochetteCache) memcpy(pochetteCache + oy * POCHETTE_T, ligne, POCHETTE_T * 2);
    tft.drawRGBBitmap(POCHETTE_X, POCHETTE_Y + oy, ligne, POCHETTE_T, 1);
  }
  pochetteEnCache = pochetteCache != nullptr;
}

void pochetteVide() {
  tft.fillRect(POCHETTE_X, POCHETTE_Y, POCHETTE_T, POCHETTE_T, C_GRIS_CLAIR);
  // note de musique
  int cx = POCHETTE_X + POCHETTE_T / 2, cy = POCHETTE_Y + POCHETTE_T / 2;
  tft.fillCircle(cx - 12, cy + 22, 10, C_BLANC);
  tft.fillRect(cx - 4, cy - 30, 5, 52, C_BLANC);
  tft.fillTriangle(cx - 4, cy - 30, cx + 22, cy - 18, cx - 4, cy - 12, C_BLANC);
}

// cherche le debut du JPEG dans le cadre APIC (apres encodage, type mime, description)
int32_t debutJpeg(File &f, uint32_t pos) {
  uint8_t b[512];
  f.seek(pos);
  int n = f.read(b, sizeof(b));
  for (int i = 0; i + 2 < n; i++)
    if (b[i] == 0xFF && b[i + 1] == 0xD8 && b[i + 2] == 0xFF) return pos + i;
  return -1;
}

// JPEG (baseline) situe a l'octet "debut" du fichier : decode a la plus petite taille >= cadre (1/2/4/8),
// puis redimensionne pour remplir le cadre (image carree ; sinon etiree)
bool pochetteJpeg(const String &chemin, uint32_t debut) {
  File f = SD.open(chemin);
  if (!f) return false;
  uint16_t w = 0, h = 0;
  f.seek(debut);
  TJpgDec.getFsJpgSize(&w, &h, f);   // ferme le fichier
  if (w == 0 || h == 0) { log("Pochette " + chemin + " : JPEG illisible"); return false; }
  uint8_t echelle = 1;
  while (echelle < 8 && w / (echelle * 2) >= POCHETTE_T && h / (echelle * 2) >= POCHETTE_T) echelle *= 2;
  jpgL = (w + echelle - 1) / echelle; jpgH = (h + echelle - 1) / echelle;
  jpgImage = (uint16_t *)ps_malloc((size_t)jpgL * jpgH * 2);
  if (!jpgImage) { log("Pochette : memoire insuffisante"); return false; }
  memset(jpgImage, 0xFF, (size_t)jpgL * jpgH * 2);
  TJpgDec.setJpgScale(echelle);
  TJpgDec.setCallback(blocPochette);
  f = SD.open(chemin);
  f.seek(debut);
  JRESULT r = TJpgDec.drawFsJpg(0, 0, f);   // ferme le fichier
  if (r == JDR_OK) { dessinerImageRedim(); log("Pochette " + String(w) + "x" + String(h) + " decodee en " + String(jpgL) + "x" + String(jpgH)); }
  else log("Pochette " + chemin + " : echec decodage (JPEG progressif ?)");
  free(jpgImage); jpgImage = nullptr;
  return r == JDR_OK;
}

uint32_t lire32(const uint8_t *b) { return b[0] | (b[1] << 8) | (b[2] << 16) | ((uint32_t)b[3] << 24); }

// BMP 24 bits ou 16 bits non compresse, agrandi ou reduit pour remplir le cadre
bool pochetteBmp(const String &chemin) {
  File f = SD.open(chemin);
  if (!f) return false;
  uint8_t e[54];
  if (f.read(e, 54) != 54 || e[0] != 'B' || e[1] != 'M') { f.close(); return false; }
  uint32_t debut = lire32(e + 10);
  int32_t w = (int32_t)lire32(e + 18), h = (int32_t)lire32(e + 22);
  uint16_t bpp = e[28] | (e[29] << 8);
  uint32_t compression = lire32(e + 30);
  bool basEnHaut = h > 0;   // BMP classique : lignes stockees du bas vers le haut
  if (h < 0) h = -h;
  if (w <= 0 || !((bpp == 24 && compression == 0) || (bpp == 16 && (compression == 0 || compression == 3)))) {
    log("Pochette " + chemin + " : BMP non gere (" + String(bpp) + " bits)");
    f.close(); return false;
  }
  int wa = POCHETTE_T, ha = (int32_t)h * POCHETTE_T / w;
  if (ha > POCHETTE_T) { ha = POCHETTE_T; wa = (int32_t)w * POCHETTE_T / h; }
  int x0 = POCHETTE_X + (POCHETTE_T - wa) / 2, y0 = POCHETTE_Y + (POCHETTE_T - ha) / 2;
  uint32_t tailleLigne = ((w * bpp + 31) / 32) * 4;
  uint8_t *src = (uint8_t *)malloc(tailleLigne);
  uint16_t *dst = (uint16_t *)malloc(wa * 2);
  if (!src || !dst) { free(src); free(dst); f.close(); return false; }

  tft.fillRect(POCHETTE_X, POCHETTE_Y, POCHETTE_T, POCHETTE_T, C_BLANC);
  int syPrec = -1;
  for (int oy = 0; oy < ha; oy++) {
    int sy = (int32_t)oy * h / ha;
    if (sy != syPrec) {   // relire seulement si la ligne source change
      f.seek(debut + (uint32_t)(basEnHaut ? h - 1 - sy : sy) * tailleLigne);
      f.read(src, tailleLigne);
      syPrec = sy;
    }
    for (int ox = 0; ox < wa; ox++) {
      int sx = (int32_t)ox * w / wa;
      if (bpp == 24) {
        uint8_t *p = src + sx * 3;   // ordre B, G, R
        dst[ox] = ((p[2] & 0xF8) << 8) | ((p[1] & 0xFC) << 3) | (p[0] >> 3);
      } else {
        uint16_t v = src[sx * 2] | (src[sx * 2 + 1] << 8);
        dst[ox] = compression == 3 ? v : ((v & 0x7C00) << 1) | ((v & 0x03E0) << 1) | (v & 0x001F);
      }
    }
    tft.drawRGBBitmap(x0, y0 + oy, dst, wa, 1);
  }
  free(src); free(dst);
  f.close();
  return true;
}

// image separee sur la SD : .jpg ou .bmp selon l'extension
bool pochetteFichier(const String &chemin) {
  String ext = chemin; ext.toLowerCase();
  bool ok = ext.endsWith(".bmp") ? pochetteBmp(chemin) : pochetteJpeg(chemin, 0);
  if (ok) log("Pochette : " + chemin);
  return ok;
}

void dessinerPochette() {
  if (pile.back().type != E_LECTURE || posFile < 0) return;
  pochetteEnCache = false;
  Morceau &m = morceauCourant();

  uint32_t pos = m.apic ? m.apic : pochettePos;   // connue des la lecture des tags, sinon signalee par la librairie
  if (pos) {   // image integree au MP3
    File f = SD.open(m.chemin);
    int32_t debut = f ? debutJpeg(f, pos) : -1;
    if (f) f.close();
    if (debut >= 0 && pochetteJpeg(m.chemin, debut)) { log("Pochette : integree au MP3"); return; }
    if (debut < 0) log("Pochette integree : pas un JPEG (PNG ?)");
  }

  String c = cheminPochetteSD(m);
  if (c.length() && pochetteFichier(c)) return;
  log("Pochette : aucune");
  pochetteVide();
}

// premiere image de pochette trouvee sur la SD pour ce morceau ("" si aucune)
String cheminPochetteSD(const Morceau &m) {
  String dossier = m.chemin.substring(0, m.chemin.lastIndexOf('/'));
  String candidats[] = {
    "/covers/" + m.album + ".jpg", "/covers/" + m.album + ".bmp",
    dossier + "/cover.jpg", dossier + "/folder.jpg", dossier + "/cover.bmp",
    "/covers/cover.bmp",
  };
  for (auto &c : candidats) if (SD.exists(c)) return c;
  return "";
}


// ------------------------------------------------------------------ Demarrage
void afficherChargement(int n) {
  if (n == 0) {
    tft.fillScreen(C_BLANC);
    logoOk(120, 90, 44, C_BLEU, C_BLANC);
    texteCentre("iPod", 160, 3, C_NOIR, C_BLANC);
    texteCentre("OK Computer Edition", 196, 1, C_GRIS_TEXTE, C_BLANC);
  }
  texteCentre("Chargement... " + String(n), 250, 1, C_GRIS_TEXTE, C_BLANC);
}

void afficherWifi(const String &msg) {
  texteCentre(msg, 270, 1, C_GRIS_TEXTE, C_BLANC);
}

void afficherErreur(const String &msg) {
  tft.fillScreen(C_BLANC);
  texteCentre(msg, 150, 2, ILI9341_RED, C_BLANC);
}
