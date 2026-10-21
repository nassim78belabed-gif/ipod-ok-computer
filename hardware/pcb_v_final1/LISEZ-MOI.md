# iPod OK Computer : version finale 1 (multi-cartes pour boîtier imprimé 3D)

Découpage en 2 cartes reliées par câble, pensé pour un boîtier imprimé en 3D style iPod Classic (écran en haut, boutons en bas). L'écran ILI9341+SD reste un module externe (pas de carte custom pour lui), câblé directement à la carte logique.

## Les 2 cartes

**Carte LOGIQUE** (`logic/`, 77 x 95 mm) : ESP32-S3-DevKitC-1 + DAC PCM5102A + têtes de câble vers l'écran (J4, 8 pts), la SD de l'écran (J5, 4 pts) et la façade boutons (J7, 7 pts). Va se loger où il y a de la place dans le boîtier (dos, fond...).

**Carte FAÇADE BOUTONS** (`buttons/`, 42,6 x 72,2 mm) : les 6 boutons (MENU, PLAY, PREV, NEXT, VOL+, VOL-) en grille 2x3, pas 20 mm entre eux, reliée à la carte logique par un connecteur 7 points (J_LINK : 6 signaux + GND commun). Positionne-la dans le boîtier là où tu veux les boutons, et adapte le pas de la grille dans `buttons/generateur/pcbgen.py` si besoin (variable `default_grid`), puis régénère (voir « Régénérer les cartes »).

**Écran** : le module ILI9341+SD lui-même, fixé directement derrière la fenêtre du boîtier, relié à J4/J5 de la carte logique par des fils (ou un petit connecteur JST si tu préfères du démontable).

## Liaisons entre cartes

Rien d'imposé côté connecteur : les headers sont en 2,54 mm, tu peux souder un JST-PH ou XH à la place si tu veux du câblage propre et démontable. Pas de signal rapide ni sensible dans ces liaisons (juste des boutons vers GND), donc pas de contrainte de longueur de câble particulière.

## Fichiers 3D pour Fusion 360 / SolidWorks

- `logic/logic_v_final1.step`
- `buttons/buttons_v_final1.step`

Ce sont les step du PCB (contour + trous de fixation + épaisseur de carte). Les connecteurs/boutons n'ont pas de modèle 3D dans ces STEP (footprints KiCad standards sans modèle 3D associé) : seule la plaque de PCB avec ses trous est représentée, mais c'est largement suffisant pour caler les découpes et les vis de fixation dans ton boîtier.

## État du routage

Les deux cartes routent à 100% : 0 net non routé, 0 via, 0 strap, DRC clean (piste 0,8mm, clearance 0,4mm) sur les deux.

## Fichiers

- `logic/kicad/` (`.kicad_pcb` + `.kicad_pro`), `logic/gerber/gerbers_logic.zip`, `logic/logic_v_final1.step`, `logic/apercu_logic.png`, `logic/generateur/`
- `buttons/kicad/` (`.kicad_pcb` + `.kicad_pro`), `buttons/gerber/gerbers_buttons.zip`, `buttons/buttons_v_final1.step`, `buttons/apercu_buttons.png`, `buttons/generateur/`
- `CONNEXIONS.md` : broche par broche, quel net va où, pour les 2 cartes

## Pas de schéma KiCad

Les 2 cartes ont été générées directement en PCB par script, sans passer par un schéma : il n'y a donc pas de `.kicad_sch`. `CONNEXIONS.md` (extrait des `.kicad_pcb`) fait office de schéma. Conséquence dans KiCad : ne pas lancer « Mettre à jour le PCB depuis le schéma », ça effacerait les nets.

## Régénérer les cartes (optionnel)

Les scripts de `generateur/` ont servi à produire les PCB. Ils ne sont pas nécessaires pour fabriquer, seulement pour modifier le placement puis rerouter automatiquement. Ils ont tourné sous Linux avec le Python de KiCad 7 (`pcbnew`), Java + Freerouting 1.9 (chemin du .jar via la variable `FR_JAR`), `xvfb-run` et `shapely`. Sous Windows, ça demande des adaptations (pas de `xvfb-run`).

```
cd logic/generateur
KEEPGND=1 LAYERS=2 python3 finalize2.py cfg.json out.kicad_pcb 40
python3 check.py out.kicad_pcb
```

Le placement se règle dans `cfg.json` (positions des connecteurs) et dans `pcbgen.py`.

## À faire ensuite

- Choisir/mesurer le connecteur pour les 2 liaisons carte-à-carte (JST-PH suggéré, pas obligatoire)
- Modéliser le boîtier autour des 2 STEP + du module écran dans Fusion/SolidWorks
- DRC officiel KiCad avant fabrication
- Toujours en attente : les vraies dimensions mesurées de tes modules (écran, DAC) pour ajuster précisément les découpes du boîtier
