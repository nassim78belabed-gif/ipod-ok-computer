"""iPod OK Computer - v_final1 : carte FACADE (6 boutons) reliee a la carte logique par J_LINK."""
import pcbnew
from pcbnew import FromMM as mm, VECTOR2I, EDA_ANGLE, DEGREES_T

FP = '/usr/share/kicad/footprints/'

BUTTONS = [
    ('SW1', 'BTN_MENU', 'MENU'),
    ('SW2', 'BTN_PLAY', 'PLAY'),
    ('SW3', 'BTN_PREV', 'PREV'),
    ('SW4', 'BTN_NEXT', 'NEXT'),
    ('SW5', 'BTN_VOLUP', 'VOL+'),
    ('SW6', 'BTN_VOLDOWN', 'VOL-'),
]

_FPC = {}

def build(cfg=None, with_outline=True):
    cfg = cfg or {}
    pos = cfg.get('pos', {})
    b = pcbnew.BOARD()
    b.SetCopperLayerCount(2)
    ds = b.GetDesignSettings()
    nc = ds.m_NetSettings.m_DefaultNetClass
    nc.SetClearance(mm(0.4)); nc.SetTrackWidth(mm(0.8))
    nc.SetViaDiameter(mm(2.2)); nc.SetViaDrill(mm(1.0))
    ds.m_TrackMinWidth = mm(0.8); ds.m_MinClearance = mm(0.4)
    ds.m_ViasMinSize = mm(2.0); ds.m_MinThroughDrill = mm(1.0)
    ds.m_CopperEdgeClearance = mm(0.5)
    nets = {}

    def net(name):
        if name not in nets:
            n = pcbnew.NETINFO_ITEM(b, name); b.Add(n); nets[name] = n
        return nets[name]

    def add(ref, lib, name, x, y, rot=0, value='', pinnets=None):
        key = (lib, name)
        if key not in _FPC: _FPC[key] = pcbnew.FootprintLoad(FP + lib + '.pretty', name)
        f = pcbnew.Cast_to_FOOTPRINT(_FPC[key].Duplicate())
        f.SetReference(ref); f.SetValue(value or name)
        f.SetPosition(VECTOR2I(mm(x), mm(y)))
        f.SetOrientation(EDA_ANGLE(rot, DEGREES_T))
        b.Add(f)
        for p in f.Pads():
            n = (pinnets or {}).get(p.GetNumber())
            if n: p.SetNet(net(n))
        f.Value().SetVisible(False)
        return f

    def text(s, x, y, size=1.2, layer=pcbnew.F_SilkS, rot=0):
        t = pcbnew.PCB_TEXT(b)
        t.SetText(s); t.SetPosition(VECTOR2I(mm(x), mm(y)))
        t.SetLayer(layer); t.SetTextSize(VECTOR2I(mm(size), mm(size)))
        t.SetTextThickness(mm(size * 0.15))
        t.SetTextAngle(EDA_ANGLE(rot, DEGREES_T))
        b.Add(t)

    # grille 2 colonnes x 3 lignes, pas 20mm (a ajuster selon le boitier reel)
    default_grid = {
        'SW1': (10, 10), 'SW2': (30, 10),
        'SW3': (10, 30), 'SW4': (30, 30),
        'SW5': (10, 50), 'SW6': (30, 50),
    }
    grid = dict(default_grid); grid.update({k: tuple(v) for k, v in pos.items() if k in default_grid})

    for ref, sig, lbl in BUTTONS:
        x, y = grid[ref]
        add(ref, 'Button_Switch_THT', 'SW_PUSH_6mm', x, y, 0, lbl, {'1': sig, '2': 'GND'})
        text(lbl, x, y - 5, 1.0)

    # connecteur vers la carte logique (7 broches : 6 signaux + GND)
    lx, ly = pos.get('J_LINK', (20, 66))
    pin_nets = {str(i + 1): BUTTONS[i][1] for i in range(6)}
    pin_nets['7'] = 'GND'
    add('J_LINK', 'Connector_PinHeader_2.54mm', 'PinHeader_1x07_P2.54mm_Vertical', lx, ly, 90,
        'VERS CARTE LOGIQUE', pin_nets)
    text('VERS LOGIQUE', lx, ly + 6, 0.9)

    if not with_outline:
        return b, {}, {}
    xs = [p.GetPosition().x for f in b.GetFootprints() for p in f.Pads()]
    ys = [p.GetPosition().y for f in b.GetFootprints() for p in f.Pads()]
    M = 8
    x0, y0, x1, y1 = min(xs) - mm(M), min(ys) - mm(M), max(xs) + mm(M), max(ys) + mm(M)
    for a, c in [((x0, y0), (x1, y0)), ((x1, y0), (x1, y1)), ((x1, y1), (x0, y1)), ((x0, y1), (x0, y0))]:
        s = pcbnew.PCB_SHAPE(b); s.SetShape(pcbnew.SHAPE_T_SEGMENT); s.SetLayer(pcbnew.Edge_Cuts)
        s.SetStart(VECTOR2I(*a)); s.SetEnd(VECTOR2I(*c)); s.SetWidth(mm(0.15)); b.Add(s)
    for hi, (hx, hy) in enumerate([(x0 + mm(4), y0 + mm(4)), (x1 - mm(4), y0 + mm(4)),
                                   (x0 + mm(4), y1 - mm(4)), (x1 - mm(4), y1 - mm(4))]):
        f = pcbnew.FootprintLoad(FP + 'MountingHole.pretty', 'MountingHole_2.2mm_M2')
        f.SetPosition(VECTOR2I(hx, hy)); f.SetReference('H%d' % (hi + 1)); f.Reference().SetVisible(False)
        f.Value().SetVisible(False); b.Add(f)
    return b, {}, {}

def footprint_boxes(b, margin=1.6):
    out = {}
    for f in b.GetFootprints():
        xs = [pcbnew.ToMM(p.GetPosition().x) for p in f.Pads()]
        ys = [pcbnew.ToMM(p.GetPosition().y) for p in f.Pads()]
        if xs: out[f.GetReference()] = (min(xs) - margin, min(ys) - margin, max(xs) + margin, max(ys) + margin)
    return out

def overlaps(b):
    bx = footprint_boxes(b)
    ks = [k for k in bx if not k.startswith('H')]
    n = 0
    for i in range(len(ks)):
        for j in range(i):
            a, c = bx[ks[i]], bx[ks[j]]
            if a[0] < c[2] and c[0] < a[2] and a[1] < c[3] and c[1] < a[3]: n += 1
    return n

REFMAP = {}
def apply_refmap(b): pass
