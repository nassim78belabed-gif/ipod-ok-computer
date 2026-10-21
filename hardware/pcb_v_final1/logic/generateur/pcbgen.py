"""iPod OK Computer - v_final1 : carte LOGIQUE (ESP32 + DAC + ecran/SD deportes + header boutons)."""
import json
import pcbnew
from pcbnew import FromMM as mm, VECTOR2I, EDA_ANGLE, DEGREES_T

FP = '/usr/share/kicad/footprints/'
P = 2.54
ROW = 22.86
EX, EY = 90.0, 15.0

def ey(r): return EY + (r - 1) * P

LEFT_ROWS = {4: 4, 5: 5, 6: 6, 7: 7, 8: 15, 9: 16, 10: 17, 11: 18, 12: 8,
             15: 9, 16: 10, 17: 11, 18: 12, 19: 13, 20: 14}
FLEX = ['I2S_BCK', 'I2S_DIN', 'I2S_LRCK', 'SD_CS', 'SPI_MISO', 'SPI_SCK',
        'SPI_MOSI', 'TFT_DC', 'TFT_RST', 'TFT_CS']

DEFAULT_ROWS = {'I2S_LRCK': 5, 'I2S_DIN': 6, 'I2S_BCK': 7, 'SD_CS': 9, 'SPI_MOSI': 10,
                'SPI_MISO': 11, 'SPI_SCK': 12, 'TFT_DC': 15, 'TFT_RST': 16, 'TFT_CS': 17}

DEFAULT_POS = {
    'J_DAC': (EX - 6 * P, ey(8), 180),
    'J_LCD': (EX - 24 * P, ey(20), 180),
    'J_SD4': (EX - 12 * P, ey(9), 0),
    'C3': (EX - 16 * P, ey(24), 0), 'C4': (EX - 16 * P, ey(27), 0),
    'J_BTN': (EX - 24 * P, ey(27), 0),
}
MOVABLE = list(DEFAULT_POS)
CAPD = ('Capacitor_THT', 'C_Disc_D5.0mm_W2.5mm_P5.00mm')

# header boutons : 6 signaux + GND, vers la carte facade
BTN_SIGNALS = ['BTN_MENU', 'BTN_PLAY', 'BTN_PREV', 'BTN_NEXT', 'BTN_VOLUP', 'BTN_VOLDOWN']
BTN_ROWS = {'BTN_MENU': 6, 'BTN_PLAY': 7, 'BTN_PREV': 8, 'BTN_NEXT': 9, 'BTN_VOLUP': 10, 'BTN_VOLDOWN': 4}

_FPC = {}

def build(cfg=None, with_outline=True):
    cfg = cfg or {}
    rows = dict(DEFAULT_ROWS); rows.update(cfg.get('rows', {}))
    pos = dict(DEFAULT_POS); pos.update({k: tuple(v) for k, v in cfg.get('pos', {}).items()})
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

    def hdr(ref, kind, npins, x, y, rot, labels, value=''):
        lib = 'Connector_PinSocket_2.54mm' if kind == 'S' else 'Connector_PinHeader_2.54mm'
        nm = ('PinSocket' if kind == 'S' else 'PinHeader') + '_1x%02d_P2.54mm_Vertical' % npins
        return add(ref, lib, nm, x, y, rot, value or ref, labels)

    def text(s, x, y, size=1.2, layer=pcbnew.F_SilkS, rot=0):
        t = pcbnew.PCB_TEXT(b)
        t.SetText(s); t.SetPosition(VECTOR2I(mm(x), mm(y)))
        t.SetLayer(layer); t.SetTextSize(VECTOR2I(mm(size), mm(size)))
        t.SetTextThickness(mm(size * 0.15))
        t.SetTextAngle(EDA_ANGLE(rot, DEGREES_T))
        b.Add(t)

    # ESP32-S3-DevKitC-1
    J1 = {1: '3V3', 2: '3V3', 21: '5V', 22: 'GND'}
    for sig, r in rows.items(): J1[r] = sig
    J3 = {1: 'GND', 21: 'GND', 22: 'GND'}
    for sig, r in BTN_ROWS.items(): J3[r] = sig
    hdr('J1', 'S', 22, EX, EY, 0, {str(k): v for k, v in J1.items()}, 'ESP32-S3 J1 (gauche)')
    hdr('J3', 'S', 22, EX + ROW, EY, 0, {str(k): v for k, v in J3.items()}, 'ESP32-S3 J3 (droite)')

    # DAC
    hdr('J_DAC', 'S', 6, *pos['J_DAC'][:2], pos['J_DAC'][2],
        {'1': 'GND', '2': 'I2S_BCK', '3': 'I2S_DIN', '4': 'I2S_LRCK', '5': 'GND', '6': '3V3'},
        'PCM5102A: SCK BCK DIN LCK GND VIN')

    # header vers ecran deporte (8 signaux actifs, pin9=SDO non cablee)
    hdr('J_LCD', 'S', 8, *pos['J_LCD'][:2], pos['J_LCD'][2],
        {'1': '3V3', '2': 'GND', '3': 'TFT_CS', '4': 'TFT_RST', '5': 'TFT_DC', '6': 'SPI_MOSI',
         '7': 'SPI_SCK', '8': '3V3'}, 'VERS ECRAN: VCC GND CS RST DC MOSI SCK LED')
    # header vers SD du module ecran
    hdr('J_SD4', 'S', 4, *pos['J_SD4'][:2], pos['J_SD4'][2],
        {'1': 'SD_CS', '2': 'SPI_MOSI', '3': 'SPI_MISO', '4': 'SPI_SCK'}, 'VERS SD ecran')
    add('C3', 'Capacitor_THT', 'CP_Radial_D5.0mm_P2.00mm', *pos['C3'], '10uF', {'1': '3V3', '2': 'GND'})
    add('C4', *CAPD, *pos['C4'], '100nF', {'1': '3V3', '2': 'GND'})

    # header vers carte facade boutons (6 signaux + GND commun)
    btn_pins = {str(i + 1): BTN_SIGNALS[i] for i in range(6)}
    btn_pins['7'] = 'GND'
    hdr('J_BTN', 'H', 7, *pos['J_BTN'][:2], pos['J_BTN'][2], btn_pins,
        'VERS FACADE: MENU PLAY PREV NEXT VOL+ VOL- GND')
    text('VERS FACADE', pos['J_BTN'][0], pos['J_BTN'][1] - 3, 1.0)
    text('VERS ECRAN', pos['J_LCD'][0], pos['J_LCD'][1] - 3, 1.0)

    for pr in cfg.get('pre', []):
        pts = pr['pts']
        for k in range(len(pts) - 1):
            t = pcbnew.PCB_TRACK(b)
            t.SetStart(VECTOR2I(mm(pts[k][0]), mm(pts[k][1]))); t.SetEnd(VECTOR2I(mm(pts[k + 1][0]), mm(pts[k + 1][1])))
            t.SetLayer(pcbnew.F_Cu); t.SetWidth(mm(0.8)); t.SetNet(net(pr['net'])); t.SetLocked(True); b.Add(t)
    if not with_outline:
        return b, rows, pos
    xs = [p.GetPosition().x for f in b.GetFootprints() for p in f.Pads()]
    ys = [p.GetPosition().y for f in b.GetFootprints() for p in f.Pads()]
    M = 8; ML = cfg.get('margin_left', 8)
    x0, y0, x1, y1 = min(xs) - mm(ML), min(ys) - mm(M), max(xs) + mm(M), max(ys) + mm(M)
    for a, c in [((x0, y0), (x1, y0)), ((x1, y0), (x1, y1)), ((x1, y1), (x0, y1)), ((x0, y1), (x0, y0))]:
        s = pcbnew.PCB_SHAPE(b); s.SetShape(pcbnew.SHAPE_T_SEGMENT); s.SetLayer(pcbnew.Edge_Cuts)
        s.SetStart(VECTOR2I(*a)); s.SetEnd(VECTOR2I(*c)); s.SetWidth(mm(0.15)); b.Add(s)
    for hi, (hx, hy) in enumerate([(x0 + mm(4), y0 + mm(4)), (x1 - mm(4), y0 + mm(4)),
                                   (x0 + mm(4), y1 - mm(4)), (x1 - mm(4), y1 - mm(4))]):
        f = pcbnew.FootprintLoad(FP + 'MountingHole.pretty', 'MountingHole_3.2mm_M3')
        f.SetPosition(VECTOR2I(hx, hy)); f.SetReference('H%d' % (hi + 1)); f.Reference().SetVisible(False)
        f.Value().SetVisible(False); b.Add(f)
    return b, rows, pos

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

REFMAP = {'J_LCD': 'J4', 'J_SD4': 'J5', 'J_DAC': 'J6', 'J_BTN': 'J7'}

def apply_refmap(b):
    for f in b.GetFootprints():
        r = f.GetReference()
        if r in REFMAP: f.SetReference(REFMAP[r])
