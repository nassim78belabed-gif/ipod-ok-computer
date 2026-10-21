# iPod OK Computer Edition : ESP32-S3

![License](https://img.shields.io/badge/license-GPL--3.0-blue)
![Platform](https://img.shields.io/badge/platform-ESP32--S3-orange)

Lecteur audio fait maison, style iPod Classic : MP3 sur carte SD, écran couleur 2,4", DAC I2S et 6 boutons.

> Projet repris de zéro en septembre 2026 avec un nouveau brochage (firmware fonctionnel sur breadboard depuis fin septembre). L'ancienne version (Cover Flow, TFT_eSPI, ancien câblage) reste dans l'historique git, commit `50034ca`.

## Matériel

| Composant | Détail |
|---|---|
| Microcontrôleur | ESP32-S3-DevKitC-1 **N16R8** (16 Mo flash, 8 Mo PSRAM octal) |
| Écran | ILI9341 2,4" 240x320 SPI, module avec lecteur SD intégré (tactile non utilisé) |
| Audio | DAC I2S PCM5102A (module **CJMCU-5102**, 11 broches), sortie jack LINE OUT |
| Commandes | 6 boutons poussoirs vers GND (pull-up interne) |

## Coût estimé

Prix **estimés** (AliExpress / Amazon, à ajuster avec les vrais prix payés).

| Composant | Prix estimé |
|---|---|
| ESP32-S3-DevKitC-1 N16R8 | ~8 € |
| Écran ILI9341 2,4" avec lecteur SD | ~8 € |
| DAC PCM5102A (module CJMCU-5102) | ~5 € |
| Carte microSD 32 Go | ~5 € |
| 6 boutons poussoirs | ~2 € |
| Breadboard + fils (prototype) | ~5 € |
| **Total version breadboard** | **~33 €** |

Version finale V1, portable sur batterie :

| Composant | Prix estimé |
|---|---|
| ESP32, écran, DAC, microSD, boutons (voir ci-dessus) | ~28 € |
| Batterie LiPo YL 603450 (3,7 V, 1200 mAh, protégée) | ~8 € |
| Chargeur TP4056 | ~1 € |
| Régulateur 3,3 V, interrupteur, prise JST | ~3 € |
| PCB (2 cartes, fabrication type JLCPCB) | ~10 € |
| Boîtier imprimé en 3D | ~15 € |
| **Total version portable V1** | **~65 €** |

La V2 (molette cliquable iPod 4G à la place des boutons) ajoute le prix de la molette : ~10 à 15 € d'occasion (estimation).

## Fonctionnalités

Validées sur breadboard (firmware `firmware/ipod/`) :
- **Menus** Artistes / Albums / Morceaux, écran **Lecture en cours** (pochette, progression, volume), enchaînement des morceaux, pause, précédent / suivant, avance rapide.
- **Pochettes** : image intégrée au MP3, `/covers/<album>.jpg|.bmp`, `cover.jpg` / `folder.jpg` du dossier ; les JPG sont redimensionnés pour remplir le cadre.
- **Transfert MP3 par WiFi** : menu « Transfert MP3 », envoi de plusieurs MP3 d'un coup depuis un téléphone ou un PC, rangement automatique dans `/Music/<artiste>/<album>/` d'après les tags. Le WiFi n'est allumé que dans ce menu (sur breadboard, il fait des bips dans le son).
- **Easter eggs par album** : thème de couleurs, messages sur certaines pistes (fixes, défilants, « glitch ») et code secret aux boutons, décrits dans un fichier `easter.txt` par album (voir `easter/` pour les modèles).
- **Commandes série** : mêmes actions que les boutons (`m p < > + -`) et maintenance de la SD (liste, suppression, diagnostic des tags), via `outils/serie.py`.

Volume limité à 19/21 (au-delà, la sortie du DAC sature) ; départ à 16.

## Brochage

| Fonction | GPIO |
|---|---|
| SPI SCK / MOSI / MISO (écran + SD) | 8 / 17 / 18 |
| SD CS | 16 |
| TFT CS / DC / RST | 11 / 9 / 10 |
| I2S BCK / DIN / LRCK | 7 / 6 / 5 |
| Boutons MENU / PLAY / PREV / NEXT / VOL+ / VOL- | 42 / 41 / 40 / 39 / 38 / 1 |

Rétroéclairage de l'écran sur 3V3 direct. Détails et contraintes : [hardware/wiring.md](hardware/wiring.md).

Points importants :
- **Le MISO de l'écran n'est pas câblé** : l'ILI9341 ne libère pas SDO et bloquerait la SD.
- **Bus SPI partagé** : toujours relâcher un CS avant d'activer l'autre.
- GPIO 26 à 37 interdits (PSRAM octal).

## PCB et boîtier

- **PCB v_final1** ([hardware/pcb_v_final1](hardware/pcb_v_final1/LISEZ-MOI.md)) : 2 cartes reliées par câble.
  - Carte logique (77 x 95 mm) : ESP32-S3 DevKit + PCM5102A + connecteurs vers l'écran, la SD et la façade boutons.
  - Carte façade (42,6 x 72,2 mm) : les 6 boutons en grille 2x3.
  - Le module écran ILI9341+SD reste externe, câblé à la carte logique.
  - Pas de schéma KiCad : les cartes ont été générées directement en PCB par script (`generateur/`). [CONNEXIONS.md](hardware/pcb_v_final1/CONNEXIONS.md) liste toutes les connexions et tient lieu de schéma.
- **Boîtier v1** ([hardware/boitier_v1](hardware/boitier_v1/LISEZ-MOI.md)) : 2 coques (avant + arrière), 83 x 131 x 27 mm, générées en CadQuery à partir des cotes des PCB. **Provisoire** : plusieurs cotes (écran, hauteurs, découpes USB-C et jack) sont estimées et à mesurer avant impression.

## Avancement

| Étape | État |
|---|---|
| Carte : puce, flash 16 Mo, PSRAM 8 Mo | ✅ testé (`test_carte`) |
| 1. SD : montage, liste, écriture/relecture | ✅ testé (`test_sd`, `liste_sd`) |
| Lecture des tags ID3 et du format MP3 | ✅ testé (`tags_sd`) |
| 2. Écran + SD + pochette | ✅ testé (`test_ecran_sd`) |
| 3. Boutons avec anti-rebond | ⏳ géré par le firmware, test dédié prêt (`test_boutons`) mais pas lancé |
| 4. Lecture MP3 vers le PCM5102A en I2S | ✅ testé (`test_bip`, `test_audio`) |
| 5. Interface iPod : Artistes, Albums, Morceaux, Lecture en cours, volume | ✅ firmware `firmware/ipod/` |
| 6. Transfert MP3 par WiFi | ✅ validé (25 MP3, ~0,3 Mo/s) |
| Easter eggs par album | ✅ essayés sur la carte |
| Améliorations d'interface (moteur d'affichage en PSRAM, polices, Cover Flow, playlists…) | ⏳ prévues |
| PCB : support DAC 11 broches, alimentation, découplage, pistes I2S courtes | ⏳ corrections à faire avant fabrication |
| V1 sur batterie (LiPo YL 603450, chargeur, mesure de tension) | ⏳ à ajouter au PCB |
| V2 : molette cliquable iPod 4G | ⏳ plus tard |

Tests faits sur breadboard uniquement : le PCB n'est pas encore fabriqué.

## Compilation

Outils : Arduino IDE 2 ou [arduino-cli](https://arduino.github.io/arduino-cli/), core **esp32 by Espressif 3.x**.

Bibliothèques : **Adafruit ILI9341** (+ Adafruit GFX, Adafruit BusIO), **TJpg_Decoder** (pochettes JPG). Audio : **ESP32-audioI2S** (schreibfaux) 4.0.0, **copie patchée incluse dans le dépôt** (`firmware/ipod/src/ESP32-audioI2S/`, correctif d'une division par zéro sur certains MP3, voir son `LISEZMOI.md`) : rien à installer pour le firmware principal.

Réglages de la carte :

| Option | Valeur |
|---|---|
| Carte | ESP32S3 Dev Module |
| PSRAM | OPI PSRAM |
| Flash Size | 16 Mo |
| Partition | 16M Flash (3MB APP/9.9MB FATFS) |
| USB CDC On Boot | selon le port USB, voir ci-dessous |

**USB CDC On Boot** dépend du port USB branché :
- port **« COM »** (puce CH343) : désactivé, `Serial` sort sur l'UART0 ;
- port **« USB »** (USB natif) : activé, `Serial` sort sur l'USB natif.

Le firmware et les sketches de test écrivent sur `Serial` et `Serial0` : les messages restent lisibles quel que soit le port.

Exemple avec arduino-cli (port COM10) :

```
arduino-cli compile -u -p COM10 --fqbn esp32:esp32:esp32s3:PSRAM=opi,FlashSize=16M,CDCOnBoot=default,PartitionScheme=app3M_fat9M_16MB firmware/ipod
```

Remplacer `CDCOnBoot=default` par `CDCOnBoot=cdc` pour activer le CDC.

## Firmware

`firmware/ipod/` : `ipod.ino` (navigation, audio, boutons), `bibliotheque.ino` (tags ID3), `ecran.ino` (affichage), `easter.ino` (easter eggs), `web.ino` + `page_web.h` (WiFi, transfert), `ipod.h` (réglages).

## Sketches de test

| Sketch | Rôle |
|---|---|
| `firmware/tests/test_carte` | Infos puce, flash, test PSRAM, état des broches boutons |
| `firmware/tests/test_sd` | Montage SD, taille, liste de la racine, écriture/relecture de `test.txt` |
| `firmware/tests/liste_sd` | Liste récursive de toute la carte SD |
| `firmware/tests/tags_sd` | Tags ID3 (v1, v2.2 à 2.4) et format MPEG des MP3 de `/Music` |
| `firmware/tests/test_ecran_sd` | Écran (couleurs, texte, pochette) + liste de la SD |
| `firmware/tests/test_boutons` | Les 6 boutons avec anti-rebond (prêt, pas encore lancé) |
| `firmware/tests/test_bip` | Bip I2S vers le DAC |
| `firmware/tests/test_audio` | Lecture d'un MP3 de la SD vers le DAC |

## Carte SD

- Format **FAT32**
- Musique rangée dans `/Music/Artiste/Album/`, fichiers MP3 avec tags ID3 complets (titre, artiste, album, numéro de piste)
- MP3 testés : MPEG1 Layer III, 44,1 ou 48 kHz, stéréo

Pour ajouter de la musique sans retirer la carte : menu **Transfert MP3** sur l'iPod, puis page web (l'adresse s'affiche à l'écran) ou `curl -F "f=@fichier.mp3" http://<adresse>/api/envoi`.

Pour de bons fichiers : albums achetés sans DRM (Bandcamp, Qobuz, 7digital) ou CD rippés (Exact Audio Copy, fre:ac), qui arrivent avec tags et pochette.

## Structure du dépôt

```
iPod-ESP32-S3/
├── firmware/
│   ├── ipod/          # Firmware principal
│   └── tests/         # Sketches de test (un dossier par sketch)
├── easter/            # Modèles de fichiers easter.txt (un par album)
├── outils/            # Scripts : commandes série, encodage de CD, journal série
├── hardware/
│   ├── wiring.md      # Brochage et contraintes
│   ├── pcb_v_final1/  # PCB actuel : carte logique + carte boutons (KiCad, gerbers, STEP, générateurs)
│   │   └── CONNEXIONS.md  # Liste des connexions (tient lieu de schéma)
│   ├── boitier_v1/    # Boîtier provisoire (STEP/STL, script CadQuery gen_case.py)
│   └── archives/      # Anciennes versions (v7, v8, breakout clickwheel), référence uniquement
└── LICENSE
```

## Licence

GPL-3.0, voir [LICENSE](LICENSE).
