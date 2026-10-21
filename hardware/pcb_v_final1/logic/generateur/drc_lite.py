import sys, collections
import pcbnew
from shapely.geometry import Point, LineString, box
from shapely.ops import unary_union
from shapely import affinity
b = pcbnew.LoadBoard(sys.argv[1])
CL = 0.4
mm = pcbnew.ToMM
shapes = []   # (net, geom, kind, label)
def pad_geom(p):
    pos = p.GetPosition(); x, y = mm(pos.x), mm(pos.y)
    sz = p.GetSize(); w, h = mm(sz.x), mm(sz.y)
    sh = p.GetShape()
    if sh == pcbnew.PAD_SHAPE_CIRCLE or (w == h and sh != pcbnew.PAD_SHAPE_RECT):
        g = Point(x, y).buffer(w/2)
    elif sh == pcbnew.PAD_SHAPE_RECT:
        g = box(x-w/2, y-h/2, x+w/2, y+h/2)
    else:   # ovale / autre
        r = min(w, h)/2
        if w >= h: g = LineString([(x-(w/2-r), y), (x+(w/2-r), y)]).buffer(r)
        else: g = LineString([(x, y-(h/2-r)), (x, y+(h/2-r))]).buffer(r)
    ang = p.GetOrientationDegrees()
    if ang: g = affinity.rotate(g, -ang, origin=(x, y))
    return g
for f in b.GetFootprints():
    for p in f.Pads():
        shapes.append((p.GetNetname() or ('_nc_%s_%s' % (f.GetReference(), p.GetNumber())), pad_geom(p), 'pad', f.GetReference() + '.' + p.GetNumber(), 'PAD'))
minw = 99
for t in b.GetTracks():
    if t.Type() == pcbnew.PCB_VIA_T:
        p = t.GetPosition(); shapes.append((t.GetNetname(), Point(mm(p.x), mm(p.y)).buffer(mm(t.GetWidth())/2), 'pad', 'via@%.1f,%.1f' % (mm(p.x), mm(p.y)), 'PAD')); nvia = globals().get('nvia', 0) + 1; globals()['nvia'] = nvia
    if t.Type() == pcbnew.PCB_TRACE_T:
        s, e = t.GetStart(), t.GetEnd()
        g = LineString([(mm(s.x), mm(s.y)), (mm(e.x), mm(e.y))]).buffer(mm(t.GetWidth())/2, cap_style=1)
        lay = 'B' if t.GetLayer() == pcbnew.B_Cu else 'F'
        shapes.append((t.GetNetname(), g, 'trk', 'track@%.1f,%.1f' % (mm(s.x), mm(s.y)), lay))
        minw = min(minw, mm(t.GetWidth()))
print('vias:', globals().get('nvia', 0)); print('largeur mini de piste: %.2f mm' % minw)
# clearances entre reseaux differents (meme couche cuivre ; les pastilles TH sont sur les 2 couches)
viol = 0; mind = 99
for i in range(len(shapes)):
    for j in range(i):
        a, c = shapes[i], shapes[j]
        if a[0] == c[0]: continue
        if a[4] != 'PAD' and c[4] != 'PAD' and a[4] != c[4]: continue
        if a[2] == 'pad' and c[2] == 'pad' and a[3].split('.')[0] == c[3].split('.')[0]:
            pass
        d = a[1].distance(c[1])
        mind = min(mind, d)
        if d < CL - 0.02:
            viol += 1
            if viol <= 15: print('  VIOLATION %.3f mm : %s (%s) <-> %s (%s)' % (d, a[3], a[0], c[3], c[0]))
print('distance mini entre reseaux differents: %.3f mm ; violations < %.2f: %d' % (mind, CL, viol))
bb = b.GetBoardEdgesBoundingBox()
print('carte %.1f x %.1f mm' % (mm(bb.GetWidth()), mm(bb.GetHeight())))
