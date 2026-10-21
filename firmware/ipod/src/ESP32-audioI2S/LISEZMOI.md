# ESP32-audioI2S 4.0.0 (schreibfaux) — copie patchée

Copie des sources de la bibliothèque [ESP32-audioI2S](https://github.com/schreibfaux/ESP32-audioI2S)
version 4.0.0 (licence GPL-3.0, voir `LICENSE`), incluse dans le sketch pour que le correctif
ci-dessous ne soit pas perdu lors d'une mise à jour de la bibliothèque ou sur un autre PC.

Le firmware l'inclut avec `#include "src/ESP32-audioI2S/Audio.h"` : la bibliothèque installée
dans l'IDE Arduino n'est pas utilisée par `firmware/ipod/`.

## Correctif

`Audio.cpp`, fonction `read_ID3_Header()` (chercher « patch iPod OK Computer ») :
quand l'en-tête Xing/Info d'un MP3 donne une durée de 0, la bibliothèque divisait par zéro
(`bytes * 8 / duration`) et l'ESP32 plantait (IntegerDivideByZero). La durée et le débit ne
sont maintenant calculés que si la durée est non nulle.
