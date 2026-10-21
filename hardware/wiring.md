# Câblage — iPod OK Computer Edition

## Matériel

- ESP32-S3-DevKitC-1 **N16R8** (16 Mo flash, 8 Mo PSRAM octal)
- Écran ILI9341 2,4" 240x320 SPI (module avec lecteur SD intégré, tactile non utilisé)
- DAC I2S PCM5102A (module GY-PCM5102)
- 6 boutons poussoirs vers GND (pull-up interne)

## Brochage

| Fonction | GPIO | Remarque |
|---|---|---|
| SPI SCK | 8 | partagé écran + SD |
| SPI MOSI | 17 | partagé écran + SD |
| SPI MISO | 18 | SD uniquement (SDO écran **non câblé**) |
| SD CS | 16 | |
| TFT CS | 11 | |
| TFT DC | 9 | |
| TFT RST | 10 | |
| TFT LED | — | rétroéclairage sur 3V3 direct (pas de PWM) |
| I2S BCK | 7 | PCM5102A |
| I2S DIN | 6 | PCM5102A |
| I2S LRCK | 5 | PCM5102A |
| BTN MENU | 42 | actif bas, INPUT_PULLUP |
| BTN PLAY | 41 | actif bas, INPUT_PULLUP |
| BTN PREV | 40 | actif bas, INPUT_PULLUP |
| BTN NEXT | 39 | actif bas, INPUT_PULLUP |
| BTN VOL+ | 38 | actif bas, INPUT_PULLUP |
| BTN VOL- | 1 | actif bas, INPUT_PULLUP |

## Contraintes

- GPIO 26 à 37 interdits (PSRAM octal)
- Éviter les broches de strapping 0, 3, 45, 46, l'USB 19 et 20, et l'UART0 43 et 44
- Le MISO de l'écran n'est pas câblé : l'ILI9341 ne libère pas SDO et bloquerait la SD
- Bus SPI partagé : toujours relâcher un CS avant d'activer l'autre
