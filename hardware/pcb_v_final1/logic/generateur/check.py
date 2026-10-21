import pcbnew, sys, math, collections
b = pcbnew.LoadBoard(sys.argv[1])
def pt(v): return (v.x, v.y)
items = collections.defaultdict(list)   # net -> list of ('pad',ref,num,pos) / ('trk',a,b,w)
for f in b.GetFootprints():
    for p in f.Pads():
        n = p.GetNetname()
        if n: items[n].append(('pad', f.GetReference() + '.' + p.GetNumber(), pt(p.GetPosition()), p.GetSize().x/2))
for t in b.GetTracks():
    if t.Type() == pcbnew.PCB_TRACE_T:
        items[t.GetNetname()].append(('trk', pt(t.GetStart()), pt(t.GetEnd()), t.GetWidth()/2))
def dseg(p, a, c):
    ax, ay = a; cx, cy = c; px, py = p
    dx, dy = cx-ax, cy-ay; L = dx*dx+dy*dy
    t = 0 if L == 0 else max(0, min(1, ((px-ax)*dx+(py-ay)*dy)/L))
    return math.hypot(px-(ax+t*dx), py-(ay+t*dy))
bad = 0
for n, its in sorted(items.items()):
    if n in ('GND', ''): continue
    parent = list(range(len(its)))
    def find(i):
        while parent[i] != i: parent[i] = parent[parent[i]]; i = parent[i]
        return i
    def touch(x, y):
        tol = 20000
        if x[0] == 'pad' and y[0] == 'pad': return math.dist(x[2], y[2]) < tol
        if x[0] == 'pad': x, y = y, x
        if x[0] == 'trk' and y[0] == 'pad':
            return dseg(y[2], x[1], x[2]) < max(tol, y[3]*0.3)
        # trk-trk : endpoints of one on segment of other
        for e in (x[1], x[2]):
            if dseg(e, y[1], y[2]) < tol: return True
        for e in (y[1], y[2]):
            if dseg(e, x[1], x[2]) < tol: return True
        return False
    for i in range(len(its)):
        for j in range(i):
            if touch(its[i], its[j]): parent[find(i)] = find(j)
    comps = collections.defaultdict(list)
    for i, it in enumerate(its):
        if it[0] == 'pad': comps[find(i)].append(it[1])
    if len(comps) > 1:
        bad += 1
        print('NON ROUTE', n, [sorted(v) for v in comps.values()])
print('nets non connectes:', bad)
