# iPod OK Computer — v7 (test SD)

Carte de test pour ESP32-S3-DevKitC-1 N16R8 : écran ILI9341 + SD (slot de l'écran, bus SPI partagé) + DAC PCM5102A + boutons/molette + LiPo/TP4056/MT3608.

## Câblage ESP32 ↔ périphériques

| Fonction | GPIO |
|---|---|
| SPI SCK (écran + SD) | 8 |
| SPI MOSI (écran + SD) | 17 |
| SPI MISO (SD seulement) | 18 |
| SD CS | 16 |
| TFT CS / DC / RST | 11 / 9 / 10 |
| I2S BCK / DIN / LRCK | 7 / 6 / 5 |
| Bouton MENU / PLAY | 42 / 41 |
| Molette A / B / SW | 40 / 39 / 38 |
| VBAT (ADC, via R4/R5) | 1 |

GPIO 26-37 (PSRAM octal) et 0, 3, 45, 46 (strapping) ne sont pas utilisés.

## Notes modules

- **ILI9341** : SDO/MISO de l'écran volontairement NON connecté (il ne passe pas en haute impédance et bloquerait la SD). Si alimenté en 3,3 V, fermer le jumper J1 du module.
- **PCM5102A** : souder les pads XSMT = H et FMT = L. SCK relié à GND.
- **MT3608** : régler à 5,0 V AVANT de le brancher.
- **TP4056** : version avec protection (B+/B-, OUT).
- **Carte SD** : FAT32, 32 Go max.

## Arduino IDE

Carte ESP32S3 Dev Module, USB CDC On Boot = Enabled, Flash 16 MB, PSRAM OPI, Serial 115200.
Sketch : `firmware/sd_test/sd_test.ino` (+ `pins.h`).

## Procédure de test SD

Le sketch fait d'abord un sondage brut (CMD0/CMD8 à 400 kHz) :
- CMD0 doit répondre `0x01`. `0xFF` = aucune réponse : vérifier MISO (GPIO 18), CS (16), alim 3,3 V de la SD, soudures.
- Ensuite `SD.begin` à 400 kHz / 4 / 10 / 20 MHz, écriture/lecture d'un fichier, listing, test de débit 1 Mo.
- Puis alternance écran / lecture SD en boucle pour prouver que le bus partagé marche.

## Le strap unique

Tout est routé sur B.Cu (GND en plan de masse), sauf **1 strap** : LED de l'écran → VCC (~17,8 mm, sur F.Cu à gauche de J4).
- Fabrication LPKF simple face : fil volant côté cuivre entre LED et VCC de J4.
- Fabrication plaquée 2 faces (JLCPCB) : la piste F.Cu est du vrai cuivre.

## À vérifier

- Entraxe des rangées du DevKit : 22,86 mm supposé.
- Brochage réel de chaque module (DAC, SD, écran, TP4056, MT3608).
- Taille de la carte : **160 x 85 mm**, donc plus grande que les 65x70 mm du TP.
- Pas de test matériel, pas de compilation sur un vrai core ESP32 (syntaxe vérifiée seulement), DRC KiCad non lancé : lancer le DRC dans KiCad 9 avant fabrication.
- Le schéma est fait par étiquettes (labels), pas de fils dessinés.
