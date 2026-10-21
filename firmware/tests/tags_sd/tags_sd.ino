// Lecture des tags ID3 (v2.2/2.3/2.4 + v1) et du format MPEG des MP3 de /Music - ESP32-S3-DevKitC-1
// Sortie sur Serial (USB CDC) ET Serial0 (UART0 / port COM), texte en UTF-8

#include <SPI.h>
#include <SD.h>
#include <vector>

// --- Brochage (hardware/wiring.md) ---
#define SPI_SCK   8
#define SPI_MOSI  17
#define SPI_MISO  18
#define SD_CS     16
#define TFT_CS    11   // ecran non utilise, mais CS maintenu haut pour liberer le bus

struct Tags {
  String version, titre, artiste, album, piste, annee, genre, pochette;
};

std::vector<String> albums;  // "Artiste / Album" sans doublon
uint32_t nbMp3 = 0, nbSansTag = 0;
uint8_t buf[4096];

void ecrire(const String &txt) {
  Serial.println(txt);
  Serial0.println(txt);
}

String taille(uint64_t o) {
  if (o >= 1024 * 1024) return String(o / 1048576.0, 1) + " Mo";
  if (o >= 1024) return String(o / 1024.0, 1) + " Ko";
  return String((uint32_t)o) + " o";
}

uint32_t synchsafe(const uint8_t *p) { return (p[0] << 21) | (p[1] << 14) | (p[2] << 7) | p[3]; }
uint32_t be32(const uint8_t *p) { return ((uint32_t)p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]; }

// --- Conversion des encodages ID3 vers UTF-8 ---
void ajouterCodepoint(String &s, uint32_t cp) {
  if (cp < 0x80) s += (char)cp;
  else if (cp < 0x800) { s += (char)(0xC0 | (cp >> 6)); s += (char)(0x80 | (cp & 0x3F)); }
  else if (cp < 0x10000) {
    s += (char)(0xE0 | (cp >> 12)); s += (char)(0x80 | ((cp >> 6) & 0x3F)); s += (char)(0x80 | (cp & 0x3F));
  } else {
    s += (char)(0xF0 | (cp >> 18)); s += (char)(0x80 | ((cp >> 12) & 0x3F));
    s += (char)(0x80 | ((cp >> 6) & 0x3F)); s += (char)(0x80 | (cp & 0x3F));
  }
}

String latin1(const uint8_t *p, size_t n) {
  String s;
  for (size_t i = 0; i < n && p[i]; i++) ajouterCodepoint(s, p[i]);
  return s;
}

String utf16(const uint8_t *p, size_t n, bool bigEndian) {
  size_t i = 0;
  if (n >= 2 && p[0] == 0xFF && p[1] == 0xFE) { bigEndian = false; i = 2; }
  else if (n >= 2 && p[0] == 0xFE && p[1] == 0xFF) { bigEndian = true; i = 2; }
  String s;
  while (i + 1 < n) {
    uint16_t u = bigEndian ? (p[i] << 8) | p[i + 1] : p[i] | (p[i + 1] << 8);
    i += 2;
    if (u == 0) break;
    uint32_t cp = u;
    if (u >= 0xD800 && u < 0xDC00 && i + 1 < n) {
      uint16_t u2 = bigEndian ? (p[i] << 8) | p[i + 1] : p[i] | (p[i + 1] << 8);
      i += 2;
      cp = 0x10000 + ((u - 0xD800) << 10) + (u2 - 0xDC00);
    }
    ajouterCodepoint(s, cp);
  }
  return s;
}

String texteId3(const uint8_t *p, size_t n) {
  if (n < 2) return "";
  switch (p[0]) {
    case 0: return latin1(p + 1, n - 1);
    case 1: return utf16(p + 1, n - 1, false);
    case 2: return utf16(p + 1, n - 1, true);
    default: {  // 3 = UTF-8
      String s;
      for (size_t i = 1; i < n && p[i]; i++) s += (char)p[i];
      return s;
    }
  }
}

String rogner(const uint8_t *p, size_t n) {  // champ ID3v1 (latin1, complete par espaces/zeros)
  String s = latin1(p, n);
  s.trim();
  return s;
}

// --- ID3v2 : renvoie l'offset de fin du tag (0 si absent) ---
uint32_t lireId3v2(File &f, Tags &t) {
  uint8_t h[10];
  f.seek(0);
  if (f.read(h, 10) != 10 || memcmp(h, "ID3", 3) != 0) return 0;
  uint8_t ver = h[3], flags = h[5];
  uint32_t fin = 10 + synchsafe(h + 6);
  t.version = "ID3v2." + String(ver);
  if (flags & 0x80) t.version += " (unsync)";

  uint32_t pos = 10;
  if (flags & 0x40) {  // en-tete etendu
    uint8_t e[4];
    f.read(e, 4);
    pos += ver == 4 ? synchsafe(e) : be32(e) + 4;
  }
  const int idLen = ver == 2 ? 3 : 4, hdrLen = ver == 2 ? 6 : 10;

  while (pos + hdrLen <= fin) {
    uint8_t fh[10];
    f.seek(pos);
    f.read(fh, hdrLen);
    if (fh[0] == 0) break;  // padding
    char id[5] = {0};
    memcpy(id, fh, idLen);
    uint32_t fs = ver == 2 ? (fh[3] << 16) | (fh[4] << 8) | fh[5]
                : ver == 4 ? synchsafe(fh + 4) : be32(fh + 4);
    uint32_t data = pos + hdrLen;
    if (fs == 0 || data + fs > fin) break;

    String *cible = nullptr;
    if (!strcmp(id, "TIT2") || !strcmp(id, "TT2")) cible = &t.titre;
    else if (!strcmp(id, "TPE1") || !strcmp(id, "TP1")) cible = &t.artiste;
    else if (!strcmp(id, "TALB") || !strcmp(id, "TAL")) cible = &t.album;
    else if (!strcmp(id, "TRCK") || !strcmp(id, "TRK")) cible = &t.piste;
    else if (!strcmp(id, "TYER") || !strcmp(id, "TDRC") || !strcmp(id, "TYE")) cible = &t.annee;
    else if (!strcmp(id, "TCON") || !strcmp(id, "TCO")) cible = &t.genre;

    if (cible) {
      size_t n = min(fs, (uint32_t)256);
      f.read(buf, n);
      *cible = texteId3(buf, n);
    } else if (!strcmp(id, "APIC") || !strcmp(id, "PIC")) {
      size_t n = min(fs, (uint32_t)64);
      f.read(buf, n);
      String mime;
      if (ver == 2) mime = String((char)buf[1]) + (char)buf[2] + (char)buf[3];
      else for (size_t i = 1; i < n && buf[i]; i++) mime += (char)buf[i];
      t.pochette = mime + ", " + taille(fs);
    }
    pos = data + fs;
  }
  return fin + ((flags & 0x10) ? 10 : 0);
}

// --- ID3v1 en fin de fichier : complete les champs vides ---
bool lireId3v1(File &f, Tags &t) {
  if (f.size() < 128) return false;
  f.seek(f.size() - 128);
  uint8_t v1[128];
  if (f.read(v1, 128) != 128 || memcmp(v1, "TAG", 3) != 0) return false;
  if (t.titre.isEmpty())   t.titre = rogner(v1 + 3, 30);
  if (t.artiste.isEmpty()) t.artiste = rogner(v1 + 33, 30);
  if (t.album.isEmpty())   t.album = rogner(v1 + 63, 30);
  if (t.annee.isEmpty())   t.annee = rogner(v1 + 93, 4);
  if (t.piste.isEmpty() && v1[125] == 0 && v1[126]) t.piste = String(v1[126]);
  return true;
}

// --- Premiere trame MPEG apres le tag ---
String formatMpeg(File &f, uint32_t debut) {
  static const uint16_t br1[] = {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320};
  static const uint16_t br2[] = {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160};
  static const uint32_t sr1[] = {44100, 48000, 32000};

  f.seek(debut);
  size_t n = f.read(buf, sizeof(buf));
  for (size_t i = 0; i + 4 < n; i++) {
    if (buf[i] != 0xFF || (buf[i + 1] & 0xE0) != 0xE0) continue;
    uint8_t v = (buf[i + 1] >> 3) & 3, layer = (buf[i + 1] >> 1) & 3;
    uint8_t bri = buf[i + 2] >> 4, sri = (buf[i + 2] >> 2) & 3, mode = buf[i + 3] >> 6;
    if (v == 1 || layer != 1 || bri == 0 || bri == 15 || sri == 3) continue;  // layer III seulement

    uint16_t kbps = v == 3 ? br1[bri] : br2[bri];
    uint32_t hz = sr1[sri] >> (v == 3 ? 0 : v == 2 ? 1 : 2);
    String s = String(v == 3 ? "MPEG1" : v == 2 ? "MPEG2" : "MPEG2.5") + " L3, " + String(hz) + " Hz, "
               + (mode == 3 ? "mono" : "stereo");

    // En-tete Xing/Info/VBRI dans la premiere trame
    String type = "CBR " + String(kbps) + " kbps";
    for (size_t j = i + 4; j + 4 < min(n, i + 200); j++) {
      if (!memcmp(buf + j, "Xing", 4) || !memcmp(buf + j, "VBRI", 4)) { type = "VBR"; break; }
      if (!memcmp(buf + j, "Info", 4)) break;
    }
    if (type != "VBR") {
      uint32_t sec = (uint64_t)(f.size() - debut) * 8 / (kbps * 1000UL);
      type += ", ~" + String(sec / 60) + ":" + (sec % 60 < 10 ? "0" : "") + String(sec % 60);
    }
    return s + ", " + type;
  }
  return "aucune trame MPEG trouvee";
}

void analyser(const String &chemin) {
  File f = SD.open(chemin);
  if (!f) { ecrire("ECHEC ouverture " + chemin); return; }
  nbMp3++;
  Tags t;
  uint32_t debut = lireId3v2(f, t);
  bool v1 = lireId3v1(f, t);
  if (t.version.isEmpty()) t.version = v1 ? "ID3v1 seul" : "AUCUN TAG";
  if (!debut && !v1) nbSansTag++;

  ecrire(chemin + "  (" + taille(f.size()) + ")");
  ecrire("  Tag      : " + t.version);
  ecrire("  Titre    : " + t.titre);
  ecrire("  Artiste  : " + t.artiste);
  ecrire("  Album    : " + t.album);
  ecrire("  Piste    : " + t.piste + "   Annee : " + t.annee + "   Genre : " + t.genre);
  ecrire("  Pochette : " + (t.pochette.isEmpty() ? String("aucune") : t.pochette));
  ecrire("  Audio    : " + formatMpeg(f, debut));
  f.close();

  String cle = (t.artiste.isEmpty() ? String("?") : t.artiste) + " / " + (t.album.isEmpty() ? String("?") : t.album);
  bool connu = false;
  for (auto &a : albums) if (a == cle) connu = true;
  if (!connu) albums.push_back(cle);
}

void parcourir(const String &chemin) {
  File dir = SD.open(chemin);
  if (!dir) { ecrire("ECHEC ouverture " + chemin); return; }
  std::vector<String> dossiers, mp3;
  File f = dir.openNextFile();
  while (f) {
    String nom = f.name();
    String complet = chemin + "/" + nom;
    String bas = nom; bas.toLowerCase();
    if (f.isDirectory()) dossiers.push_back(complet);
    else if (bas.endsWith(".mp3")) mp3.push_back(complet);
    f.close();
    f = dir.openNextFile();
  }
  dir.close();
  for (auto &m : mp3) analyser(m);
  for (auto &d : dossiers) parcourir(d);
}

void setup() {
  Serial.begin(115200);
  Serial0.begin(115200);
  delay(2000);
  ecrire("\n=== Tags ID3 de /Music ===");

  pinMode(TFT_CS, OUTPUT); digitalWrite(TFT_CS, HIGH);
  pinMode(SD_CS, OUTPUT);  digitalWrite(SD_CS, HIGH);
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

  if (!SD.begin(SD_CS, SPI, 20000000)) {
    ecrire("SD : ECHEC montage");
    return;
  }

  uint32_t t0 = millis();
  parcourir("/Music");
  ecrire("---");
  ecrire(String(nbMp3) + " MP3, " + String(nbSansTag) + " sans tag, lus en " + String(millis() - t0) + " ms");
  ecrire("Albums (artiste / album) :");
  for (auto &a : albums) ecrire("  " + a);
  ecrire("=== Fin ===");
}

void loop() {}
