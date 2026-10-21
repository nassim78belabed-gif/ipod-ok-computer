// Bibliotheque : parcours de /Music, lecture des tags ID3 (v2.2/2.3/2.4 + v1), listes triees

static uint8_t tamponId3[260];

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

void lireId3v2(File &f, Morceau &m) {
  uint8_t h[10];
  f.seek(0);
  if (f.read(h, 10) != 10 || memcmp(h, "ID3", 3) != 0) return;
  uint8_t ver = h[3], flags = h[5];
  uint32_t fin = 10 + synchsafe(h + 6);
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

    String piste, *cible = nullptr;
    if (!strcmp(id, "TIT2") || !strcmp(id, "TT2")) cible = &m.titre;
    else if (!strcmp(id, "TPE1") || !strcmp(id, "TP1")) cible = &m.artiste;
    else if (!strcmp(id, "TALB") || !strcmp(id, "TAL")) cible = &m.album;
    else if (!strcmp(id, "TRCK") || !strcmp(id, "TRK")) cible = &piste;
    if (!strcmp(id, "APIC") || !strcmp(id, "PIC")) m.apic = data;   // pochette : affichee sans attendre la lecture
    if (cible) {
      size_t n = min(fs, (uint32_t)256);
      f.read(tamponId3, n);
      *cible = texteId3(tamponId3, n);
      if (cible == &piste) m.piste = piste.toInt();   // "3/12" -> 3
    }
    pos = data + fs;
  }
}

void lireId3v1(File &f, Morceau &m) {
  if (f.size() < 128) return;
  f.seek(f.size() - 128);
  uint8_t v1[128];
  if (f.read(v1, 128) != 128 || memcmp(v1, "TAG", 3) != 0) return;
  if (m.titre.isEmpty())   m.titre = rogner(v1 + 3, 30);
  if (m.artiste.isEmpty()) m.artiste = rogner(v1 + 33, 30);
  if (m.album.isEmpty())   m.album = rogner(v1 + 63, 30);
  if (!m.piste && v1[125] == 0) m.piste = v1[126];
}

// commande serie 'i' : affiche l'en-tete ID3 et la liste des cadres d'un MP3
void diagnosticTags(const String &chemin) {
  File f = SD.open(chemin);
  if (!f) { log("Tags : fichier introuvable " + chemin); return; }
  uint8_t h[10];
  f.read(h, 10);
  String hex;
  for (int i = 0; i < 10; i++) { char b[4]; sprintf(b, "%02X ", h[i]); hex += b; }
  log("Tags " + chemin + " (" + String(f.size()) + " o) debut : " + hex);
  if (!memcmp(h, "ID3", 3)) {
    uint32_t fin = 10 + synchsafe(h + 6), pos = 10;
    log("ID3v2." + String(h[3]) + " flags " + String(h[5], HEX) + " taille " + String(fin));
    int n = 0;
    while (pos + 10 <= fin && n++ < 40) {
      uint8_t fh[10];
      f.seek(pos); f.read(fh, 10);
      if (fh[0] == 0) { log("  padding a " + String(pos)); break; }
      char id[5] = {0}; memcpy(id, fh, 4);
      log("  " + String(id) + " pos " + String(pos) + " taille syncsafe " + String(synchsafe(fh + 4)) +
          " / normale " + String(be32(fh + 4)));
      pos += 10 + (h[3] == 4 ? synchsafe(fh + 4) : be32(fh + 4));
    }
  }
  // qualite : 1re trame MPEG apres le tag (debit, frequence, VBR/CBR via l'en-tete Xing/Info)
  uint32_t debut = !memcmp(h, "ID3", 3) ? 10 + synchsafe(h + 6) : 0;
  static uint8_t tr[1500];
  f.seek(debut);
  int lu = f.read(tr, sizeof(tr));
  int i = 0;
  while (i + 4 < lu && !(tr[i] == 0xFF && (tr[i + 1] & 0xE0) == 0xE0)) i++;
  if (i + 4 < lu) {
    static const int debitsV1[] = {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0};
    static const int freqs[] = {44100, 48000, 32000, 0};
    bool mpeg1 = (tr[i + 1] & 0x18) == 0x18, mono = (tr[i + 3] >> 6) == 3;
    int debit = debitsV1[tr[i + 2] >> 4], freq = freqs[(tr[i + 2] >> 2) & 3];
    log(String("Trame a ") + (debut + i) + (mpeg1 ? " MPEG1" : " MPEG2/2.5") + " Layer" + String(4 - ((tr[i + 1] >> 1) & 3)) +
        " " + String(debit) + " kbit/s " + String(freq) + " Hz " + (mono ? "mono" : "stereo"));
    int x = i + (mpeg1 ? (mono ? 21 : 36) : (mono ? 13 : 21));
    if (x + 12 < lu && (!memcmp(tr + x, "Xing", 4) || !memcmp(tr + x, "Info", 4)) && (tr[x + 7] & 1) && freq) {
      uint32_t trames = be32(tr + x + 8);
      float duree = trames * 1152.0 / freq;
      log(String(!memcmp(tr + x, "Xing", 4) ? "VBR" : "CBR") + " : " + String(duree, 0) + " s, debit moyen " +
          String((f.size() - debut) * 8 / duree / 1000, 0) + " kbit/s");
    }
  }
  f.seek(f.size() - 128);
  uint8_t v1[3]; f.read(v1, 3);
  log(String("ID3v1 : ") + (memcmp(v1, "TAG", 3) ? "non" : "oui"));
  f.close();
  Morceau m = analyser(chemin);
  log("Resultat : titre=" + m.titre + " | artiste=" + m.artiste + " | album=" + m.album + " | piste=" + String(m.piste));
}

// lit les tags d'un MP3 ; sans tag : titre = nom du fichier, artiste = nom du dossier
Morceau analyser(const String &chemin) {
  Morceau m;
  m.chemin = chemin;
  m.piste = 0;
  m.apic = 0;
  File f = SD.open(chemin);
  if (f) {
    lireId3v2(f, m);
    lireId3v1(f, m);
    f.close();
  }
  if (m.titre.isEmpty()) {
    m.titre = chemin.substring(chemin.lastIndexOf('/') + 1);
    m.titre = m.titre.substring(0, m.titre.length() - 4);   // enleve ".mp3"
  }
  if (m.artiste.isEmpty()) {
    String dossier = chemin.substring(0, chemin.lastIndexOf('/'));
    m.artiste = dossier == "/Music" ? "Artiste inconnu" : dossier.substring(dossier.lastIndexOf('/') + 1);
  }
  if (m.album.isEmpty()) m.album = "Album inconnu";
  return m;
}

void parcourir(const String &dossier) {
  File dir = SD.open(dossier);
  if (!dir) return;
  File f = dir.openNextFile();
  while (f) {
    String chemin = dossier + "/" + f.name();
    bool estDossier = f.isDirectory();
    String nom = f.name(); nom.toLowerCase();
    f.close();
    if (estDossier) parcourir(chemin);
    else if (nom.endsWith(".mp3") && !nom.startsWith("._")) {   // "._xxx" = fichiers caches du Mac
      biblio.push_back(analyser(chemin));
      if (biblio.size() % 10 == 0) afficherChargement(biblio.size());
    }
    f = dir.openNextFile();
  }
}

void chargerBibliotheque() {
  biblio.clear();
  uint32_t t0 = millis();
  parcourir("/Music");
  log(String(biblio.size()) + " morceaux lus en " + String(millis() - t0) + " ms");
}

// --- Listes triees pour les menus ---
bool avant(const String &a, const String &b) { return strcasecmp(a.c_str(), b.c_str()) < 0; }

std::vector<String> listeArtistes() {
  std::vector<String> r;
  for (auto &m : biblio) {
    bool connu = false;
    for (auto &a : r) if (a.equalsIgnoreCase(m.artiste)) { connu = true; break; }
    if (!connu) r.push_back(m.artiste);
  }
  std::sort(r.begin(), r.end(), avant);
  return r;
}

// albums (d'un artiste, ou tous si artiste vide) ; artistesAlbums recoit l'artiste de chaque album
std::vector<String> listeAlbums(const String &artiste, std::vector<String> *artistesAlbums) {
  std::vector<std::pair<String, String>> r;   // (album, artiste)
  for (auto &m : biblio) {
    if (artiste.length() && !m.artiste.equalsIgnoreCase(artiste)) continue;
    bool connu = false;
    for (auto &a : r) if (a.first.equalsIgnoreCase(m.album) && a.second.equalsIgnoreCase(m.artiste)) { connu = true; break; }
    if (!connu) r.push_back({m.album, m.artiste});
  }
  std::sort(r.begin(), r.end(), [](const std::pair<String, String> &a, const std::pair<String, String> &b) {
    return avant(a.first, b.first);
  });
  std::vector<String> noms;
  for (auto &a : r) {
    noms.push_back(a.first);
    if (artistesAlbums) artistesAlbums->push_back(a.second);
  }
  return noms;
}

// indices des morceaux filtres : dans un album, ordre des pistes ; sinon ordre alphabetique
std::vector<int> listeMorceaux(const String &artiste, const String &album) {
  std::vector<int> r;
  for (int i = 0; i < (int)biblio.size(); i++) {
    if (artiste.length() && !biblio[i].artiste.equalsIgnoreCase(artiste)) continue;
    if (album.length() && !biblio[i].album.equalsIgnoreCase(album)) continue;
    r.push_back(i);
  }
  if (album.length())
    std::sort(r.begin(), r.end(), [](int a, int b) {
      if (biblio[a].piste != biblio[b].piste) return biblio[a].piste < biblio[b].piste;
      return avant(biblio[a].titre, biblio[b].titre);
    });
  else
    std::sort(r.begin(), r.end(), [](int a, int b) { return avant(biblio[a].titre, biblio[b].titre); });
  return r;
}
