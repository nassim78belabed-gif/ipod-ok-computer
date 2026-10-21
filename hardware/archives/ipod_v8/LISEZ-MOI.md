# iPod OK Computer — v8 mini (sans batterie / sans molette), double face

Version allégée du PCB : uniquement les composants que tu as (ESP32-S3, écran ILI9341 + SD, DAC PCM5102A, boutons). Pas de batterie, pas de charge, pas de boost, pas de molette.

Carte : **127,5 x 82,2 mm** (contre 160 x 85 mm pour la v7 complète), toujours plus grande que les 65x70 mm du TP "intro PCB".

**Fabrication : double face non traversante (F.Cu + B.Cu, pas de perçage plaqué).** Bonne nouvelle : le routeur a trouvé une solution où chaque réseau reste entièrement sur une seule face (47 pistes sur F.Cu, 56 sur B.Cu). **0 via, 0 strap, 0 fil de liaison à souder.** Il suffit de souder chaque composant traversant normalement des deux côtés (comme d'habitude sur une carte double face non plaquée) : c'est justement cette soudure des deux côtés qui fait le lien électrique entre les deux faces de cuivre à l'endroit de chaque pastille, donc rien de spécial à faire en plus.

## Câblage ESP32 ↔ périphériques

| Fonction | GPIO |
|---|---|
| SPI SCK (écran + SD) | 8 |
| SPI MOSI (écran + SD) | 17 |
| SPI MISO (SD seulement) | 18 |
| SD CS | 16 |
| TFT CS / DC / RST | 11 / 9 / 10 |
| I2S BCK / DIN / LRCK | 7 / 6 / 5 |
| Bouton MENU | 42 |
| Bouton PLAY | 41 |
| Bouton PREV | 40 |
| Bouton NEXT | 39 |
| Bouton VOL+ | 38 |
| Bouton VOL- | 1 |

GPIO 26-37 (PSRAM octale) et 0, 3, 45, 46 (strapping) ne sont pas utilisés. Chaque bouton va vers GND (câblage interne en `INPUT_PULLUP`).

## Notes modules

- **ILI9341 2.4"** : module confirmé avec 9 broches écran (VCC, GND, CS, RESET, DC, SDI/MOSI, SCK, LED, SDO/MISO, +5 broches tactile non utilisées) et 4 trous SD (SD_CS, SD_MOSI, SD_MISO, SD_SCK) déjà soudés côté module. SDO de l'écran volontairement NON connecté.
- **PCM5102A** : souder les pads XSMT = H et FMT = L. SCK relié à GND.
- **Carte SD** : FAT32, 32 Go max.
- **Décodage C3 (10 µF) / C4 (100 nF)** : condensateurs de découplage sur le rail 3V3, recommandés mais pas obligatoires. Si tu ne les as pas, tu peux laisser les emplacements vides, la carte fonctionne quand même (juste moins filtré).

## Arduino IDE

Carte ESP32S3 Dev Module, USB CDC On Boot = Enabled, Flash 16 MB, PSRAM OPI, Serial 115200.
Sketch : `firmware/sd_test/sd_test.ino` (+ `pins.h` mis à jour pour cette version).

## Procédure de test SD

Le sketch fait d'abord un sondage brut (CMD0/CMD8 à 400 kHz) :
- CMD0 doit répondre `0x01`. `0xFF` = aucune réponse : vérifier MISO (GPIO 18), CS (16), alim 3,3 V de la SD, soudures.
- Ensuite `SD.begin` à 400 kHz / 4 / 10 / 20 MHz, écriture/lecture d'un fichier, listing, test de débit 1 Mo.
- Puis alternance écran / lecture SD en boucle pour prouver que le bus partagé marche.

## Gravure double face

Un seul jeu de gerbers : `fabrication_double_face_non_traversant/gerbers_double_face.zip` (F.Cu, B.Cu, sérigraphie, contour, perçage). Grave les deux faces à la LPKF en les alignant sur le même contour/trous de fixation.

## DRC

Vérifié avec le DRC maison (drc_lite.py) : 0 net non connecté, 0 violation de clearance (min 0,4 mm), pistes 0,8 mm, pas de via. GND vérifié comme un seul bloc électriquement connecté (pas d'îlot isolé). Lance quand même le DRC officiel de KiCad avant fabrication (Inspect → Design Rules Checker, refill zones avant).

## À vérifier

- Entraxe des rangées du DevKit : 22,86 mm supposé.
- Brochage réel du DAC PCM5102A (le module a ses propres composants embarqués).
- Pas de test matériel, pas de compilation sur un vrai core ESP32 (syntaxe vérifiée seulement).
- Le schéma est fait par étiquettes (labels), pas de fils dessinés.

## Différences avec la v7

Retiré : batterie LiPo (JST), TP4056 (charge), MT3608 (boost 5V), diode 1N5819, molette EC11, pont diviseur VBAT.
Ajouté : 4 boutons (PREV, NEXT, VOL+, VOL-) à la place de la molette, pour un total de 6 boutons.
