# Breakout clickwheel iPod 4G → ESP32

Petit PCB (28 x 31 mm) : connecteur FPC Amphenol **SFV8R-2STE1HLF** (0,5 mm de pas, 8 positions, contact dessus — celui qui reçoit la nappe du clickwheel) câblé vers un header 1x8 en 2,54 mm, facile à brancher sur breadboard ou vers l'ESP32.

## Pinout du header J2 (de gauche à droite)

| Broche | Signal | Rôle |
|---|---|---|
| 1 | VBAT | Alim, 3,3V compatible (pile Li-Po) |
| 2 | SCK | Horloge, générée par le clickwheel lui-même (tristate) |
| 3 | CFG1 | Entrée logique, config du débit de trames |
| 4 | BTN1 | Sortie, pulse ~30ms si bouton centre ou MENU pressé |
| 5 | NC | Broche toujours basse (non utilisée) |
| 6 | MOSI | Données clickwheel → CPU (open drain) |
| 7 | MISO | Données CPU → clickwheel |
| 8 | GND | Masse (aussi reliée aux 2 pattes mécaniques du connecteur) |

Chaque broche est étiquetée en sérigraphie sur le PCB, pas besoin de retenir l'ordre.

## ⚠️ Important : sourcing du connecteur

Le SFV8R-2STE1HLF n'est **pas dans le catalogue LCSC/JLCPCB**. Ça veut dire que "commander le PCB avec le connecteur déjà soudé" n'est pas automatique. Deux façons de faire :

1. **Global sourcing JLCPCB** : dans leur outil de devis PCBA, tu peux demander qu'ils sourcent une pièce hors catalogue eux-mêmes (frais de sourcing en plus + délai rallongé). Renseigne-toi sur leur site/chat avant de valider, je n'ai pas pu vérifier le tarif exact.
2. **Consignation** : t'achètes le connecteur toi-même (Mouser/Digikey/LCSC le vendent au détail) et tu l'envoies à JLCPCB avec ta commande PCBA pour qu'ils le soudent.

Sinon, tu peux toujours le souder toi-même à la main (flux + panne fine + tresse à dessouder, comme discuté).

## Fichiers

- `kicad/` : projet KiCad complet (schéma simplifié = juste le PCB, pas de schéma séparé fait ici)
- `gerber/gerbers_clickwheel_breakout.zip` : gerbers + perçage, prêt à uploader chez JLCPCB
- `BOM.csv` / `CPL.csv` : nomenclature et placement pour la commande d'assemblage PCBA (si tu passes par le sourcing JLCPCB ou la consignation)
- `apercu.png` : aperçu du routage

## À faire ensuite

- Souder les picots 2,54 mm sur J2 toi-même (facile, traversant classique)
- Récupérer le clickwheel + nappe (iPod 4e gen Photo/Color, cf discussion précédente)
- Brancher la nappe dans le connecteur J1 (sens : contact dessus, bien vérifier l'orientation avant de refermer le clapet du connecteur)
- Je peux préparer le code ESP32 pour lire les trames une fois que t'as tout ça en main
