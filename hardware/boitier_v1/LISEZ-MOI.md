# Boîtier iPod OK Computer — v1 (PROVISOIRE)

Premier jet de boîtier 2 coquilles (avant + arrière), style iPod Classic (écran en haut, boutons en bas), généré en paramétrique (CadQuery) directement à partir des **vraies dimensions et positions de trous des 2 PCB v_final1** (extraites des fichiers KiCad, pas devinées).

**Ce boîtier n'est pas prêt à imprimer tel quel** — il pose une base correcte niveau agencement/vis, mais plusieurs cotes sont estimées faute de mesures réelles de tes modules. Vérifie/corrige avant impression (détail plus bas).

## Dimensions externes

83,0 x 131,2 x 27,0 mm (L x H x P)

## Ce qui est RÉEL (extrait direct des fichiers KiCad)

- Taille carte logique : 77,0 x 94,9 mm, 4 trous M3 (perçage 3,2mm), inset 4mm des bords
- Taille carte façade boutons : 42,65 x 72,15 mm, 4 trous M2 (perçage 2,2mm), inset 4mm des bords
- Position exacte des 6 boutons sur la carte façade (grille 2x3, pas 20mm)

## Ce qui est ESTIMÉ / PROVISOIRE (à corriger avant impression)

- **Taille du module écran** : 77 x 43mm supposé (jamais mesuré sur ton module réel) → `SCREEN_MOD_W/H` dans `gen_case.py`
- **Zone visible de l'écran** (fenêtre découpée) : 48,6 x 37,4mm, cote datasheet ILI9341 2,4" générique → `SCREEN_VIEW_W/H`
- **Profondeur du boîtier** (27mm) : suppose ~14mm pour la carte logique + le DevKit ESP32 embroché dessus + ~9mm pour la carte façade + module écran. Pas mesuré, juste une estimation raisonnable.
- **Position de la découpe USB-C** (accès au port du DevKit pour prog/charge) et **découpe jack 3,5mm** (sortie du DAC) : placées à des positions arbitraires sur le côté droit de la coque arrière — à repositionner selon l'orientation réelle que tu donnes au DevKit et au module DAC une fois câblés
- **Diamètre des trous boutons** (4mm) et **hauteur des plots de la carte façade** (8mm) : dépend du capuchon réel de tes boutons SW_PUSH_6mm (hauteur totale variable 4,3 à 13mm selon le modèle) — à ajuster pour que le bouton dépasse juste ce qu'il faut

## Assemblage prévu

- 4 vis M3 aux coins, traversantes côté avant, autotaraudantes dans les plots borgnes de la coque arrière
- Carte logique : vissée sur 4 plots au fond de la coque arrière (vis M3 autotaraudantes, perçage 2,8mm dans le plot)
- Carte façade boutons : vissée sur 4 plots à l'intérieur de la coque avant (vis M2 autotaraudantes, perçage 1,8mm dans le plot)
- Module écran : posé dans une poche (2mm de profondeur) derrière la fenêtre avant, pas de vis prévue (à coller ou clipper selon ton module)

## Fichiers

- `front_shell.step` / `front_shell.stl` — coque avant (fenêtre écran + trous boutons + plots façade)
- `back_shell.step` / `back_shell.stl` — coque arrière (plots carte logique + découpes USB/jack)
- `boitier_assemble_ecarte.step` — les 2 coquilles dans un seul fichier, écartées pour visualiser l'agencement
- `gen_case.py` — script CadQuery source ; tous les paramètres sont en haut du fichier, modifiables directement (ex: change `SCREEN_MOD_W/H` puis relance `python3 gen_case.py` pour régénérer)
- `preview_iso.png`, `front_top.png`, `back_top.png` — aperçus

## Pour la suite dans Fusion / SolidWorks

Importe les `.step` (un par coque). Comme c'est fait en CadQuery/OpenCascade, ce sont des solides propres (pas juste des surfaces), donc tu peux les éditer/mesurer/coter normalement une fois importés. Si tu préfères itérer en gardant le paramétrique, dis-moi les vraies cotes (écran, hauteur DevKit empilé, hauteur bouton) et je regénère direct plutôt que de tout refaire à la main dans Fusion.

## À faire ensuite

- Mesurer ton module écran (taille module + zone visible) au pied à coulisse
- Mesurer la hauteur totale carte logique + DevKit embroché (pile-à-pile)
- Mesurer tes boutons (hauteur totale du capuchon au-dessus du PCB)
- Repositionner les découpes USB-C / jack selon l'orientation réelle des modules
- Importer les STEP des 2 PCB (`logic_v_final1.step`, `buttons_v_final1.step`) dans Fusion/SolidWorks à côté du boîtier pour vérifier que tout rentre avant impression
