# Connexions — PCB v_final1

Il n'y a pas de schéma KiCad (.kicad_sch) pour la v_final1 : les deux cartes ont été générées directement en PCB par script (`generateur/pcbgen.py`). Cette liste, extraite des fichiers `.kicad_pcb` livrés, fait office de schéma.


## Carte LOGIQUE


**C3** — 10uF

| Broche | Net |
|---|---|
| 1 | 3V3 |
| 2 | GND |

**C4** — 100nF

| Broche | Net |
|---|---|
| 1 | 3V3 |
| 2 | GND |

**J1** — ESP32-S3 J1 (gauche)

| Broche | Net |
|---|---|
| 1 | 3V3 |
| 2 | 3V3 |
| 3 | — |
| 4 | — |
| 5 | I2S_LRCK |
| 6 | I2S_DIN |
| 7 | I2S_BCK |
| 8 | — |
| 9 | SD_CS |
| 10 | SPI_MOSI |
| 11 | SPI_MISO |
| 12 | SPI_SCK |
| 13 | — |
| 14 | — |
| 15 | TFT_DC |
| 16 | TFT_RST |
| 17 | TFT_CS |
| 18 | — |
| 19 | — |
| 20 | — |
| 21 | 5V |
| 22 | GND |

**J3** — ESP32-S3 J3 (droite)

| Broche | Net |
|---|---|
| 1 | GND |
| 2 | — |
| 3 | — |
| 4 | BTN_VOLDOWN |
| 5 | — |
| 6 | BTN_MENU |
| 7 | BTN_PLAY |
| 8 | BTN_PREV |
| 9 | BTN_NEXT |
| 10 | BTN_VOLUP |
| 11 | — |
| 12 | — |
| 13 | — |
| 14 | — |
| 15 | — |
| 16 | — |
| 17 | — |
| 18 | — |
| 19 | — |
| 20 | — |
| 21 | GND |
| 22 | GND |

**J4** — VERS ECRAN: VCC GND CS RST DC MOSI SCK LED

| Broche | Net |
|---|---|
| 1 | 3V3 |
| 2 | GND |
| 3 | TFT_CS |
| 4 | TFT_RST |
| 5 | TFT_DC |
| 6 | SPI_MOSI |
| 7 | SPI_SCK |
| 8 | 3V3 |

**J5** — VERS SD ecran

| Broche | Net |
|---|---|
| 1 | SD_CS |
| 2 | SPI_MOSI |
| 3 | SPI_MISO |
| 4 | SPI_SCK |

**J6** — PCM5102A: SCK BCK DIN LCK GND VIN

| Broche | Net |
|---|---|
| 1 | GND |
| 2 | I2S_BCK |
| 3 | I2S_DIN |
| 4 | I2S_LRCK |
| 5 | GND |
| 6 | 3V3 |

**J7** — VERS FACADE: MENU PLAY PREV NEXT VOL+ VOL- GND

| Broche | Net |
|---|---|
| 1 | BTN_MENU |
| 2 | BTN_PLAY |
| 3 | BTN_PREV |
| 4 | BTN_NEXT |
| 5 | BTN_VOLUP |
| 6 | BTN_VOLDOWN |
| 7 | GND |

### Nets (Carte LOGIQUE)

| Net | Relie |
|---|---|
| 3V3 | C3.1, C4.1, J1.1, J1.2, J4.1, J4.8, J6.6 |
| 5V | J1.21 |
| BTN_MENU | J3.6, J7.1 |
| BTN_NEXT | J3.9, J7.4 |
| BTN_PLAY | J3.7, J7.2 |
| BTN_PREV | J3.8, J7.3 |
| BTN_VOLDOWN | J3.4, J7.6 |
| BTN_VOLUP | J3.10, J7.5 |
| GND | C3.2, C4.2, J1.22, J3.1, J3.21, J3.22, J4.2, J6.1, J6.5, J7.7 |
| I2S_BCK | J1.7, J6.2 |
| I2S_DIN | J1.6, J6.3 |
| I2S_LRCK | J1.5, J6.4 |
| SD_CS | J1.9, J5.1 |
| SPI_MISO | J1.11, J5.3 |
| SPI_MOSI | J1.10, J4.6, J5.2 |
| SPI_SCK | J1.12, J4.7, J5.4 |
| TFT_CS | J1.17, J4.3 |
| TFT_DC | J1.15, J4.5 |
| TFT_RST | J1.16, J4.4 |

## Carte FAÇADE BOUTONS


**J_LINK** — VERS CARTE LOGIQUE

| Broche | Net |
|---|---|
| 1 | BTN_MENU |
| 2 | BTN_PLAY |
| 3 | BTN_PREV |
| 4 | BTN_NEXT |
| 5 | BTN_VOLUP |
| 6 | BTN_VOLDOWN |
| 7 | GND |

**SW1** — MENU

| Broche | Net |
|---|---|
| 1 | BTN_MENU |
| 1 | BTN_MENU |
| 2 | GND |
| 2 | GND |

**SW2** — PLAY

| Broche | Net |
|---|---|
| 1 | BTN_PLAY |
| 1 | BTN_PLAY |
| 2 | GND |
| 2 | GND |

**SW3** — PREV

| Broche | Net |
|---|---|
| 1 | BTN_PREV |
| 1 | BTN_PREV |
| 2 | GND |
| 2 | GND |

**SW4** — NEXT

| Broche | Net |
|---|---|
| 1 | BTN_NEXT |
| 1 | BTN_NEXT |
| 2 | GND |
| 2 | GND |

**SW5** — VOL+

| Broche | Net |
|---|---|
| 1 | BTN_VOLUP |
| 1 | BTN_VOLUP |
| 2 | GND |
| 2 | GND |

**SW6** — VOL-

| Broche | Net |
|---|---|
| 1 | BTN_VOLDOWN |
| 1 | BTN_VOLDOWN |
| 2 | GND |
| 2 | GND |

### Nets (Carte FAÇADE BOUTONS)

| Net | Relie |
|---|---|
| BTN_MENU | J_LINK.1, SW1.1, SW1.1 |
| BTN_NEXT | J_LINK.4, SW4.1, SW4.1 |
| BTN_PLAY | J_LINK.2, SW2.1, SW2.1 |
| BTN_PREV | J_LINK.3, SW3.1, SW3.1 |
| BTN_VOLDOWN | J_LINK.6, SW6.1, SW6.1 |
| BTN_VOLUP | J_LINK.5, SW5.1, SW5.1 |
| GND | J_LINK.7, SW1.2, SW1.2, SW2.2, SW2.2, SW3.2, SW3.2, SW4.2, SW4.2, SW5.2, SW5.2, SW6.2, SW6.2 |
