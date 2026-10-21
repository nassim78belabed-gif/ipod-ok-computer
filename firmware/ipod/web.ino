// Partie web : l'ESP32 heberge une page (telephone / PC sur le meme reseau)
//   - envoi de MP3 sur la SD (ranges dans /Music/<artiste>/<album>/)
//   - si TELECOMMANDE_WEB = 1 (tests) : telecommande, bibliotheque et morceau en cours
// Version finale : WiFi eteint, allume seulement dans le menu "Transfert MP3" (musique en pause)
//
// WiFi : identifiants lus dans /wifi.txt sur la SD (ligne 1 = nom du reseau, ligne 2 = mot de passe),
// sinon dans wifi_config.h (non versionne). Sans reseau : l'ESP32 cree son propre WiFi "iPod-OK-Computer".

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#if __has_include("wifi_config.h")
#include "wifi_config.h"   // #define WIFI_SSID "..." et #define WIFI_MDP "..."
#endif

#define AP_SSID "iPod-OK-Computer"
#define AP_MDP  "okcomputer"   // 8 caracteres minimum

WebServer serveur(80);
bool webActif = false, routesPretes = false;
int nbRecus = 0;
String adresseWeb, reseauWeb;
File fichierEnvoi;
String resultatEnvoi;
const char *CHEMIN_ENVOI = "/Music/_envoi.tmp";
bool envoiEaster = false;   // l'envoi en cours est un easter.txt (ecrit directement dans le dossier ?d=...)
String cheminEaster;

#include "page_web.h"   // la page HTML

// ------------------------------------------------------------------ WiFi
bool lireIdentifiants(String &ssid, String &mdp) {
  File f = SD.open("/wifi.txt");
  if (f) {
    ssid = f.readStringUntil('\n'); ssid.trim();
    mdp = f.readStringUntil('\n'); mdp.trim();
    f.close();
    if (ssid.length()) return true;
  }
#ifdef WIFI_SSID
  ssid = WIFI_SSID; mdp = WIFI_MDP;
  return true;
#else
  return false;
#endif
}

void initWifi() {
  String ssid, mdp;
  WiFi.setHostname("ipod");
  if (lireIdentifiants(ssid, mdp)) {
    afficherWifi("Connexion a " + ssid + "...");
    static bool evenement = false;
    if (!evenement) {   // raison du refus (15 = mauvais mot de passe, 201 = reseau introuvable, 202/204 = authentification)
      evenement = true;
      WiFi.onEvent([](WiFiEvent_t, WiFiEventInfo_t info) {
        log("WiFi : deconnecte, raison " + String(info.wifi_sta_disconnected.reason));
      }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    }
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), mdp.c_str());
    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 12000) delay(200);
  }
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.setSleep(false);   // reponses plus rapides
    reseauWeb = ssid;
    adresseWeb = WiFi.localIP().toString();
  } else {
    if (ssid.length()) {
      log("WiFi : echec de connexion a " + ssid + " (statut " + String(WiFi.status()) + ")");
      WiFi.disconnect();
      int n = WiFi.scanNetworks();   // le reseau est-il visible ? (l'ESP32 ne capte que le 2,4 GHz)
      bool vu = false;
      for (int i = 0; i < n; i++) if (WiFi.SSID(i) == ssid) {
        vu = true;
        log("  vu : signal " + String(WiFi.RSSI(i)) + " dBm, canal " + String(WiFi.channel(i)) +
            ", securite " + String(WiFi.encryptionType(i)));
      }
      if (!vu) log("  reseau introuvable (" + String(n) + " reseaux vus) : box en 5 GHz seulement ?");
      WiFi.scanDelete();
    }
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_MDP);
    reseauWeb = String(AP_SSID) + " (mdp " + AP_MDP + ")";
    adresseWeb = WiFi.softAPIP().toString();
  }
  WiFi.setTxPower(WIFI_POWER_8_5dBm);   // moins de pics de courant (bips, ecran blanc) ; suffit a quelques metres
  MDNS.end();
  MDNS.begin("ipod");   // http://ipod.local
  MDNS.addService("http", "tcp", 80);
  log("WiFi : " + reseauWeb + "  ->  http://" + adresseWeb + "  (ou http://ipod.local)");
}

// ------------------------------------------------------------------ JSON
String json(const String &s) {
  String r = "\"";
  for (unsigned i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '"' || c == '\\') { r += '\\'; r += c; }
    else if ((uint8_t)c < 0x20) r += ' ';
    else r += c;
  }
  return r + "\"";
}

void apiEtat() {
#if !TELECOMMANDE_WEB
  serveur.send(200, "application/json", "{\"telecommande\":false}");
  return;
#endif
  verrouiller();
  uint32_t t = audio.getAudioCurrentTime(), d = audio.getAudioFileDuration();
  int v = audio.getVolume();
  deverrouiller();
  String r = "{\"telecommande\":true,\"volmax\":" + String(VOLUME_MAX) + ",\"volume\":" + String(v) + ",\"pause\":" + (enPause ? "true" : "false");
  if (posFile >= 0) {
    Morceau &m = morceauCourant();
    r += ",\"i\":" + String(fileLecture[posFile]) + ",\"titre\":" + json(m.titre) + ",\"artiste\":" + json(m.artiste) +
         ",\"album\":" + json(m.album) + ",\"pos\":" + String(t) + ",\"duree\":" + String(d) +
         ",\"rang\":" + String(posFile + 1) + ",\"total\":" + String(fileLecture.size());
  }
  serveur.send(200, "application/json", r + "}");
}

void apiBiblio() {
  // envoi en morceaux : la bibliotheque peut etre longue
  serveur.setContentLength(CONTENT_LENGTH_UNKNOWN);
  serveur.send(200, "application/json", "");
  serveur.sendContent("[");
  String bloc;
  for (size_t i = 0; i < biblio.size(); i++) {
    Morceau &m = biblio[i];
    bloc += (i ? "," : "") + String("{\"i\":") + i + ",\"t\":" + json(m.titre) + ",\"a\":" + json(m.artiste) +
            ",\"al\":" + json(m.album) + ",\"n\":" + m.piste + "}";
    if (bloc.length() > 2000) { serveur.sendContent(bloc); bloc = ""; }
  }
  serveur.sendContent(bloc + "]");
  serveur.sendContent("");
}

void apiCommande() {
  String c = serveur.arg("c");
  if (c == "p") basculerPause();
  else if (c == "n") jouer(posFile + 1);
  else if (c == "b") precedent();
  else if (c == "+") volume(1);
  else if (c == "-") volume(-1);
  apiEtat();
}

// joue le morceau i de la bibliotheque ; la file de lecture = son album
void apiJouer() {
  int i = serveur.arg("i").toInt();
  if (i < 0 || i >= (int)biblio.size()) { serveur.send(400, "text/plain", "indice invalide"); return; }
  fileLecture = listeMorceaux(biblio[i].artiste, biblio[i].album);
  int pos = std::find(fileLecture.begin(), fileLecture.end(), i) - fileLecture.begin();
  Ecran l;
  l.type = E_LECTURE; l.titre = "Lecture en cours";
  if (pile.back().type != E_LECTURE) pile.push_back(l);
  jouer(pos);
  apiEtat();
}

// sert l'image de pochette trouvee sur la SD (pas l'image integree au MP3)
void apiPochette() {
  String c = posFile >= 0 ? cheminPochetteSD(morceauCourant()) : "";
  if (c.isEmpty()) { serveur.send(404, "text/plain", "pas de pochette"); return; }
  File f = SD.open(c);
  String ext = c; ext.toLowerCase();
  serveur.sendHeader("Cache-Control", "no-cache");
  serveur.streamFile(f, ext.endsWith(".bmp") ? "image/bmp" : "image/jpeg");
  f.close();
}

// ------------------------------------------------------------------ Envoi de MP3
String nomPropre(String s) {   // caracteres interdits dans un nom de fichier FAT
  const char *interdits = "\\/:*?\"<>|";
  for (unsigned i = 0; i < s.length(); i++) if (strchr(interdits, s[i]) || (uint8_t)s[i] < 0x20) s[i] = '_';
  s.trim();
  while (s.endsWith(".")) { s.remove(s.length() - 1); s.trim(); }   // FAT : pas de point final ("Vol. 1.")
  return s.length() ? s : "Inconnu";
}

void gererEnvoi() {
  HTTPUpload &e = serveur.upload();
  if (e.status == UPLOAD_FILE_START) {
    String nom = e.filename; nom.toLowerCase();
    resultatEnvoi = "";
    envoiEaster = (nom == "easter.txt" || nom.endsWith(".bmp"));   // fichiers ecrits directement dans le dossier ?d=...
    if (envoiEaster) {   // /api/envoi?d=/Music/<artiste>/<album> (easter.txt) ou ?d=/spidey (images .bmp)
      String d = serveur.arg("d");
      if ((!d.startsWith("/Music/") && d != "/spidey") || d.indexOf("..") >= 0) { resultatEnvoi = "refuse : dossier ?d= invalide"; return; }
      if (d == "/spidey") SD.mkdir(d);
      cheminEaster = d + "/" + nomPropre(e.filename);
    } else if (!nom.endsWith(".mp3")) { resultatEnvoi = "refuse : pas un .mp3"; return; }
    if (envoiEaster) SD.remove(cheminEaster); else SD.remove(CHEMIN_ENVOI);
    fichierEnvoi = SD.open(envoiEaster ? cheminEaster : String(CHEMIN_ENVOI), FILE_WRITE);
    if (!fichierEnvoi) resultatEnvoi = "erreur : ecriture SD impossible";
    else log("Envoi : " + e.filename);
  } else if (e.status == UPLOAD_FILE_WRITE) {
    if (fichierEnvoi && fichierEnvoi.write(e.buf, e.currentSize) != e.currentSize) {
      resultatEnvoi = "erreur : SD pleine ?";
      fichierEnvoi.close();
    }
  } else if (e.status == UPLOAD_FILE_END) {
    if (!fichierEnvoi) return;
    fichierEnvoi.close();
    if (envoiEaster) {   // pas de rangement : le fichier est deja a sa place
      if (resultatEnvoi.length()) { SD.remove(cheminEaster); return; }
      resultatEnvoi = "ok";
      eCharge = false;   // sera relu au prochain morceau
      log("Easter envoye : " + cheminEaster);
      return;
    }
    if (resultatEnvoi.length()) { SD.remove(CHEMIN_ENVOI); return; }
    // range le fichier dans /Music/<artiste>/<album>/ d'apres ses tags
    Morceau m = analyser(CHEMIN_ENVOI);
    String artiste = m.artiste == "Artiste inconnu" ? "Divers" : nomPropre(m.artiste);
    String dossier = "/Music/" + artiste;
    SD.mkdir(dossier);
    if (m.album != "Album inconnu") {   // sans tag album : directement dans le dossier de l'artiste
      dossier += "/" + nomPropre(m.album);
      SD.mkdir(dossier);
    }
    String dest = dossier + "/" + nomPropre(e.filename);
    bool remplace = SD.exists(dest);
    if (remplace) SD.remove(dest);
    if (!SD.rename(CHEMIN_ENVOI, dest)) { resultatEnvoi = "erreur : rangement impossible"; return; }
    Morceau final = analyser(dest);
    if (remplace)   // meme fichier deja dans la bibliotheque : mise a jour de ses infos
      for (auto &b : biblio) if (b.chemin == dest) { b = final; remplace = false; break; }
    if (!remplace) {
      bool deja = false;
      for (auto &b : biblio) if (b.chemin == dest) deja = true;
      if (!deja) biblio.push_back(final);
    }
    resultatEnvoi = "ok";
    nbRecus++;
    log("Envoi termine : " + dest + " (" + String(e.totalSize / 1024) + " Ko)");
    if (pile.back().type != E_LECTURE) { construireLignes(); reinitEcran(); }
  } else if (e.status == UPLOAD_FILE_ABORTED) {
    if (fichierEnvoi) fichierEnvoi.close();
    SD.remove(CHEMIN_ENVOI);
    resultatEnvoi = "interrompu";
  }
}

// ------------------------------------------------------------------ Serveur
void initWeb() {
  initWifi();
  webActif = true;
  if (routesPretes) return;   // serveur deja configure (2e passage dans Transfert MP3)
  routesPretes = true;
  WiFi.onEvent([](WiFiEvent_t, WiFiEventInfo_t) { log("WiFi : un appareil s'est connecte"); },
               ARDUINO_EVENT_WIFI_AP_STACONNECTED);
  serveur.on("/", HTTP_GET, []() {
    log("Web : page demandee par " + serveur.client().remoteIP().toString());
    serveur.send_P(200, "text/html; charset=utf-8", PAGE_WEB);
  });
  serveur.onNotFound([]() {
    log("Web : adresse inconnue " + serveur.uri());
    serveur.sendHeader("Location", "/");   // renvoie vers la page (utile pour les detecteurs de portail)
    serveur.send(302, "text/plain", "");
  });
  serveur.on("/api/etat", HTTP_GET, apiEtat);
  serveur.on("/api/sd", HTTP_GET, []() {   // telecharge un fichier de /Music vers le PC : /api/sd?f=/Music/...
    String f = serveur.arg("f");
    if (!f.startsWith("/Music/") || f.indexOf("..") >= 0) { serveur.send(400, "text/plain", "chemin refuse"); return; }
    File fh = SD.open(f);
    if (!fh || fh.isDirectory()) { serveur.send(404, "text/plain", "introuvable"); return; }
    serveur.streamFile(fh, "application/octet-stream");
    fh.close();
  });
#if TELECOMMANDE_WEB
  serveur.on("/api/biblio", HTTP_GET, apiBiblio);
  serveur.on("/api/cmd", HTTP_POST, apiCommande);
  serveur.on("/api/jouer", HTTP_POST, apiJouer);
  serveur.on("/api/pochette", HTTP_GET, apiPochette);
#endif
  serveur.on("/api/envoi", HTTP_POST, []() {
    serveur.send(resultatEnvoi == "ok" ? 200 : 400, "text/plain", resultatEnvoi.length() ? resultatEnvoi : "rien recu");
  }, gererEnvoi);
  serveur.begin();
}

void boucleWeb() { serveur.handleClient(); }

// menu "Transfert MP3" : musique en pause, WiFi allume, page d'envoi
void entrerTransfert() {
  if (posFile >= 0 && !enPause) basculerPause();   // le WiFi fait des bips dans le son
  ouvrir(E_TRANSFERT, "Transfert", "", "");
#if !TELECOMMANDE_WEB
  initWeb();
#endif
  construireLignes();
  reinitEcran();   // le demarrage du WiFi peut blanchir l'ecran
}

void sortirTransfert() {
#if !TELECOMMANDE_WEB
  serveur.handleClient();
  WiFi.mode(WIFI_OFF);
  webActif = false;
  log("WiFi coupe");
#endif
  initEcran();     // l'ecran sera redessine par retour()
}
