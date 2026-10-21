import sys, json, math, collections, os
import pcbnew
from pcbnew import FromMM as mm, ToMM, VECTOR2I, EDA_ANGLE, DEGREES_T
import pcbgen, routelib

cfgfile = sys.argv[1]; out = sys.argv[2]; passes = int(sys.argv[3]) if len(sys.argv) > 3 else 40
cfg = json.load(open(cfgfile))
b, rows, pos = pcbgen.build(cfg, with_outline=True)
n = routelib.run_freerouting(b, '/tmp/fr_final_%d' % os.getpid(), passes=passes, timeout=900, keep_gnd=bool(os.environ.get('KEEPGND')), layers=int(os.environ.get('LAYERS', '1')), skip_pads=cfg.get('no_route', []))
bad, length = routelib.check(b)
print('pistes B.Cu:', n, ' nets non routes:', [x[0] for x in bad])

# ---- straps (fils de liaison cote composants, F.Cu) pour les reseaux non routes
straps = []
from shapely.geometry import Point, LineString
def _padgeoms():
    out = []
    for fp in b.GetFootprints():
        for p in fp.Pads():
            pos = p.GetPosition(); sz = p.GetSize()
            out.append((p.GetNetname(), Point(ToMM(pos.x), ToMM(pos.y)).buffer(max(ToMM(sz.x), ToMM(sz.y)) / 2)))
    return out
_PG = _padgeoms()
def _clear(poly, netname):
    ln = LineString(poly).buffer(0.4)
    return all(n == netname or not g.intersects(ln) for n, g in _PG)
def add_strap(pa, pb, netname):
    A = (ToMM(pa.x), ToMM(pa.y)); B_ = (ToMM(pb.x), ToMM(pb.y))
    cands = [[A, B_]]
    dx, dy = B_[0] - A[0], B_[1] - A[1]; L = math.hypot(dx, dy) or 1
    nx, ny = -dy / L, dx / L
    for d in (3.6, -3.6, 5.5, -5.5, 8, -8):
        cands.append([A, (A[0] + nx * d, A[1] + ny * d), (B_[0] + nx * d, B_[1] + ny * d), B_])
    pts = next((c for c in cands if _clear(c, netname)), cands[0])
    for k in range(len(pts) - 1):
        t = pcbnew.PCB_TRACK(b)
        t.SetStart(VECTOR2I(mm(pts[k][0]), mm(pts[k][1]))); t.SetEnd(VECTOR2I(mm(pts[k+1][0]), mm(pts[k+1][1])))
        t.SetLayer(pcbnew.F_Cu); t.SetWidth(mm(0.8)); t.SetNet(b.FindNet(netname)); b.Add(t)
    straps.append((netname, A[0], A[1], B_[0], B_[1], [list(p) for p in pts]))

def padpos(refnum):
    ref, num = refnum.rsplit('.', 1)
    for f in b.GetFootprints():
        if f.GetReference() == ref:
            for p in f.Pads():
                if p.GetNumber() == num: return p.GetPosition()
for netname, comps in bad:
    # Kruskal sur les composantes : relie les paires de pastilles les plus proches
    cl = [list(c) for c in comps]
    while len(cl) > 1:
        best = None
        for i in range(len(cl)):
            for j in range(i):
                for a in cl[i]:
                    for c in cl[j]:
                        pa, pc = padpos(a), padpos(c)
                        d = math.dist((pa.x, pa.y), (pc.x, pc.y))
                        if best is None or d < best[0]: best = (d, i, j, pa, pc)
        d, i, j, pa, pc = best
        add_strap(pa, pc, netname)
        cl[j] += cl[i]; del cl[i]
print('straps:', len(straps))

# ---- sauvegarde intermediaire puis rechargement (fill zone fiable)
pcbnew.SaveBoard('/tmp/_pre_%d.kicad_pcb' % os.getpid(), b)
b = pcbnew.LoadBoard('/tmp/_pre_%d.kicad_pcb' % os.getpid())
gnd = b.FindNet('GND')
bb = b.GetBoardEdgesBoundingBox()
def make_zone(layer=pcbnew.B_Cu):
    z = pcbnew.ZONE(b); z.SetLayer(layer); z.SetNet(gnd)
    z.SetLocalClearance(mm(0.5)); z.SetMinThickness(mm(0.8))
    z.SetPadConnection(pcbnew.ZONE_CONNECTION_FULL)
    z.SetThermalReliefGap(mm(0.5)); z.SetThermalReliefSpokeWidth(mm(0.8))
    ins = mm(1.0)
    ol = pcbnew.SHAPE_LINE_CHAIN()
    for (x, y) in [(bb.GetLeft()+ins, bb.GetTop()+ins), (bb.GetRight()-ins, bb.GetTop()+ins),
                   (bb.GetRight()-ins, bb.GetBottom()-ins), (bb.GetLeft()+ins, bb.GetBottom()-ins)]:
        ol.Append(VECTOR2I(x, y))
    ol.SetClosed(True); z.Outline().AddOutline(ol)
    return z
z = make_zone(); b.Add(z)
if int(os.environ.get('LAYERS', '1')) == 2:
    b.Add(make_zone(pcbnew.F_Cu))
pcbnew.ZONE_FILLER(b).Fill(b.Zones())
gpads = [(f.GetReference() + '.' + p.GetNumber(), p) for f in b.GetFootprints() for p in f.Pads() if p.GetNetname() == 'GND']
filled = z.GetFilledPolysList(pcbnew.B_Cu)
islands = []
for k in range(filled.OutlineCount()):
    sp = pcbnew.SHAPE_POLY_SET(); sp.AddOutline(filled.COutline(k))
    islands.append((sp, sp.Area()))
def island_of(pos):
    c = [(a, k) for k, (sp, a) in enumerate(islands) if sp.Collide(pos, 0)]
    return min(c)[1] if c else None
iso = []; used = collections.Counter()
for nm, p in gpads:
    k = island_of(p.GetPosition())
    if k is None: iso.append(nm)
    else: used[k] += 1
print('GND: ilots de cuivre utilises', dict(used), ' pastilles hors plan:', iso)

# ---- serigraphie : nom de reseau a cote de chaque broche des connecteurs
def text(s, x, y, size=0.9, rot=0, layer=pcbnew.F_SilkS):
    t = pcbnew.PCB_TEXT(b); t.SetText(s); t.SetPosition(VECTOR2I(int(x), int(y)))
    t.SetLayer(layer); t.SetTextSize(VECTOR2I(mm(size), mm(size))); t.SetTextThickness(mm(size*0.15))
    t.SetTextAngle(EDA_ANGLE(rot, DEGREES_T)); b.Add(t)
skip = {'J1', 'J3', 'SW_MENU', 'SW_PLAY', 'SW_PREV', 'SW_NEXT', 'SW_VOLUP', 'SW_VOLDOWN'}
lcd_names = ['VCC', 'GND', 'CS', 'RST', 'DC', 'MOSI', 'SCK', 'LED', 'SDO', 'T_CLK', 'T_CS', 'T_DIN', 'T_DO', 'T_IRQ']
dac_names = ['SCK', 'BCK', 'DIN', 'LCK', 'GND', 'VIN']
sd_names = ['CS', 'MOSI', 'MISO', 'SCK']
for f in b.GetFootprints():
    r = f.GetReference()
    if r in ('J_LCD', 'J_DAC', 'J_SD4'):
        names = {'J_LCD': lcd_names, 'J_DAC': dac_names, 'J_SD4': sd_names}.get(r)
        for p in f.Pads():
            num = int(p.GetNumber())
            label = names[num-1] if names else p.GetNetname()
            pp = p.GetPosition()
            # a droite de la broche si les pistes arrivent par la gauche : on met le texte au-dessus/dessous
            text(label, pp.x - mm(2.6), pp.y, 0.8, 0)
for f in b.GetFootprints():
    r = f.GetReference()
    if r in ('J1', 'J3'):
        for p in f.Pads():
            nn = p.GetNetname()
            if nn: text(nn, p.GetPosition().x + (mm(2.6) if r == 'J3' else -mm(2.6)), p.GetPosition().y, 0.8)
bx0, by0 = bb.GetLeft(), bb.GetTop()
text('iPod OK Computer - v_final1 - carte FACADE BOUTONS', bx0 + mm(14), by0 + mm(3.5), 1.4)
for (nm, x0_, y0_, x1_, y1_, _pts) in straps:
    text('STRAP ' + nm, (mm(x0_) + mm(x1_)) / 2, (mm(y0_) + mm(y1_)) / 2 - mm(1.5), 0.8, 0)

pcbgen.apply_refmap(b)
b.BuildConnectivity()
pcbnew.SaveBoard(out, b)
json.dump({'straps': straps, 'bad': [x[0] for x in bad], 'gnd_isolated': iso, 'rows': rows,
           'size_mm': [ToMM(bb.GetWidth()), ToMM(bb.GetHeight())]}, open(out + '.info.json', 'w'), indent=1)
print('taille carte: %.1f x %.1f mm' % (ToMM(bb.GetWidth()), ToMM(bb.GetHeight())))
