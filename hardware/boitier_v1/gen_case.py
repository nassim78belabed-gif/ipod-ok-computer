"""Boitier v1 (PROVISOIRE) pour iPod OK Computer - style iPod Classic.
Dimensions des PCB = reelles (extraites des fichiers KiCad v_final1).
Dimensions ecran / DAC / hauteur switch / position USB = ESTIMEES, a verifier.
"""
import cadquery as cq

# ---------- PARAMETRES ----------
WALL = 2.0          # epaisseur de paroi (impression 3D)
CLR = 1.0           # jeu autour des PCB
FIT = 0.20          # jeu d'assemblage general

# PCB reels (extraits de logic/pcbgen.py et buttons/pcbgen.py)
LOGIC_W, LOGIC_H = 77.01, 94.89
LOGIC_HOLES = [(4.075, 4.075), (72.935, 4.075), (4.075, 90.815), (72.935, 90.815)]  # M3, ~3.2mm
LOGIC_HOLE_D = 3.2

BTN_W, BTN_H = 42.65, 72.15
BTN_HOLES = [(4.075, 4.075), (38.575, 4.075), (4.075, 68.075), (38.575, 68.075)]  # M2, ~2.2mm
BTN_HOLE_D = 2.2
SWITCHES = [  # (x, y) locaux sur la carte facade, label
    (8.075, 8.075, 'MENU'), (28.075, 8.075, 'PLAY'),
    (8.075, 28.075, 'PREV'), (28.075, 28.075, 'NEXT'),
    (8.075, 48.075, 'VOL+'), (28.075, 48.075, 'VOL-'),
]
SW_HOLE_D = 4.0          # PROVISOIRE : diametre passage bouton (a verifier vs. capuchon reel)
SW_STANDOFF_H = 8.0      # PROVISOIRE : hauteur carte facade / face interieure du boitier

# Ecran (PROVISOIRE, module non mesure)
SCREEN_MOD_W, SCREEN_MOD_H = 77.0, 43.0     # taille module estimee
SCREEN_VIEW_W, SCREEN_VIEW_H = 48.6, 37.4   # zone active visible ILI9341 2.4" (standard datasheet)
SCREEN_POCKET_DEPTH = 2.0

GAP_SCREEN_BTN = 4.0
MARGIN_TOP, MARGIN_BOT, MARGIN_SIDE = 4.0, 4.0, 4.0

# Profondeur boitier (PROVISOIRE : hauteur DevKit embroche + marges)
BACK_DEPTH = 14.0    # compartiment carte logique + ESP32 DevKit empile
FRONT_DEPTH = 9.0    # compartiment carte facade boutons + module ecran

CORNER_BOSS_D_OUT = 7.0
CORNER_SCREW_D = 3.2   # passant cote avant
CORNER_PILOT_D = 2.6   # borgne autotaraudant cote arriere

# ---------- CALCUL DIMENSIONS EXTERNES ----------
case_w = max(LOGIC_W, SCREEN_MOD_W) + 2 * CLR + 2 * WALL
inner_h = MARGIN_TOP + SCREEN_MOD_H + GAP_SCREEN_BTN + BTN_H + MARGIN_BOT
case_h = inner_h + 2 * WALL
case_d = WALL + BACK_DEPTH + FRONT_DEPTH + WALL

print("Boitier externe: %.1f x %.1f x %.1f mm" % (case_w, case_h, case_d))

inner_w = case_w - 2 * WALL

# Origine locale : (0,0) = coin haut-gauche de la zone interieure (vue de face)
screen_x0 = (inner_w - SCREEN_MOD_W) / 2
screen_y0 = MARGIN_TOP
btn_x0 = (inner_w - BTN_W) / 2
btn_y0 = MARGIN_TOP + SCREEN_MOD_H + GAP_SCREEN_BTN
logic_x0 = (inner_w - LOGIC_W) / 2
logic_y0 = (inner_h - LOGIC_H) / 2

def corner_positions(x0, y0, w, h, inset=3.5):
    return [(x0 + inset, y0 + inset), (x0 + w - inset, y0 + inset),
            (x0 + inset, y0 + h - inset), (x0 + w - inset, y0 + h - inset)]

case_corners = corner_positions(0, 0, inner_w, inner_h, inset=4.0)

# ---------- COQUE ARRIERE ----------
back = (
    cq.Workplane("XY")
    .box(case_w, case_h, WALL + BACK_DEPTH, centered=(True, True, False))
)
# evidement interieur (creuse la coque, laisse le fond)
back = back.faces(">Z").workplane(offset=-0.001).rect(inner_w, inner_h).cutBlind(-BACK_DEPTH)

def to_case_xy(x0, y0, x, y):
    # convertit coord locale carte (origine haut-gauche, y vers le bas) en coord boitier (centre, y vers le haut)
    return (x0 + x - inner_w / 2, inner_h / 2 - (y0 + y))

# plots de fixation carte LOGIQUE (au fond de la coque arriere, hauteur = BACK_DEPTH - jeu)
LOGIC_STANDOFF_H = BACK_DEPTH - 2.0
for (hx, hy) in LOGIC_HOLES:
    cx, cy = to_case_xy(logic_x0, logic_y0, hx, hy)
    boss = (cq.Workplane("XY").workplane(offset=WALL)
            .center(cx, cy).circle(3.2).extrude(LOGIC_STANDOFF_H)
            .faces(">Z").workplane().hole(LOGIC_HOLE_D - 0.4, depth=LOGIC_STANDOFF_H + 1))
    back = back.union(boss)

# plots d'assemblage aux 4 coins (borgnes, autotaraudant M3)
for (cx0, cy0) in case_corners:
    cx = cx0 - inner_w / 2
    cy = inner_h / 2 - cy0
    boss = (cq.Workplane("XY").workplane(offset=WALL)
            .center(cx, cy).circle(CORNER_BOSS_D_OUT / 2).extrude(BACK_DEPTH - 1)
            .faces(">Z").workplane().hole(CORNER_PILOT_D, depth=BACK_DEPTH))
    back = back.union(boss)

# decoupe laterale PROVISOIRE : acces USB-C du DevKit (a repositionner selon orientation reelle)
usb_cut = (cq.Workplane("YZ").workplane(offset=-case_w / 2)
           .center(inner_h / 2 - logic_y0 - 14, WALL + BACK_DEPTH - 8)
           .rect(9.5, 4.0).extrude(WALL + 2))
back = back.cut(usb_cut)

# decoupe laterale PROVISOIRE : jack audio 3.5mm sortie DAC (a repositionner)
jack_cut = (cq.Workplane("YZ").workplane(offset=case_w / 2 - WALL - 2)
            .center(inner_h / 2 - logic_y0 + 10, WALL + BACK_DEPTH - 7)
            .circle(3.5).extrude(WALL + 4))
back = back.cut(jack_cut)

# ---------- COQUE AVANT ----------
front = (
    cq.Workplane("XY")
    .box(case_w, case_h, WALL + FRONT_DEPTH, centered=(True, True, False))
)
front = front.faces(">Z").workplane(offset=-0.001).rect(inner_w, inner_h).cutBlind(-FRONT_DEPTH)

# fenetre ecran : poche (module) + fenetre visible (traversante)
scx, scy = to_case_xy(screen_x0, screen_y0, SCREEN_MOD_W / 2, SCREEN_MOD_H / 2)
pocket = (cq.Workplane("XY").workplane(offset=WALL + FRONT_DEPTH - SCREEN_POCKET_DEPTH)
          .center(scx, scy).rect(SCREEN_MOD_W + 2 * FIT, SCREEN_MOD_H + 2 * FIT)
          .extrude(SCREEN_POCKET_DEPTH + 1))
front = front.cut(pocket)
window = (cq.Workplane("XY").workplane(offset=-0.001)
          .center(scx, scy).rect(SCREEN_VIEW_W, SCREEN_VIEW_H)
          .extrude(WALL + FRONT_DEPTH + 2))
front = front.cut(window)

# trous boutons
for (bx, by, lbl) in SWITCHES:
    cx, cy = to_case_xy(btn_x0, btn_y0, bx, by)
    hole = (cq.Workplane("XY").workplane(offset=-0.001)
            .center(cx, cy).circle(SW_HOLE_D / 2).extrude(WALL + 2))
    front = front.cut(hole)

# plots de fixation carte FACADE BOUTONS (sur face interieure de l'avant)
for (hx, hy) in BTN_HOLES:
    cx, cy = to_case_xy(btn_x0, btn_y0, hx, hy)
    boss = (cq.Workplane("XY").workplane(offset=WALL)
            .center(cx, cy).circle(2.6).extrude(SW_STANDOFF_H)
            .faces(">Z").workplane().hole(BTN_HOLE_D - 0.4, depth=SW_STANDOFF_H + 1))
    front = front.union(boss)

# plots d'assemblage aux 4 coins (passage vis M3)
for (cx0, cy0) in case_corners:
    cx = cx0 - inner_w / 2
    cy = inner_h / 2 - cy0
    boss = (cq.Workplane("XY").workplane(offset=WALL)
            .center(cx, cy).circle(CORNER_BOSS_D_OUT / 2).extrude(FRONT_DEPTH - 1)
            .faces(">Z").workplane().hole(CORNER_SCREW_D, depth=FRONT_DEPTH + 2))
    front = front.union(boss)

# ---------- EXPORT ----------
cq.exporters.export(back, "back_shell.step")
cq.exporters.export(back, "back_shell.stl")
cq.exporters.export(front, "front_shell.step")
cq.exporters.export(front, "front_shell.stl")

# assemblage pour previsualisation (avant translate en +Z pour l'ecart visuel)
assembly = cq.Assembly()
assembly.add(back, name="back", loc=cq.Location(cq.Vector(0, 0, 0)))
assembly.add(front, name="front", loc=cq.Location(cq.Vector(0, 0, WALL + BACK_DEPTH + FRONT_DEPTH)))
assembly.save("boitier_assemble_ecarte.step")

print("OK")
