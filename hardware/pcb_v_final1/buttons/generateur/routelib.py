import pcbnew, subprocess, os, math, collections
from pcbnew import FromMM as mm

def parse_sexp(text):
    tok = []; i = 0; n = len(text)
    while i < n:
        c = text[i]
        if c in '()': tok.append(c); i += 1
        elif c.isspace(): i += 1
        elif c == '"':
            j = text.index('"', i+1); tok.append(text[i+1:j]); i = j+1
        else:
            j = i
            while j < n and not text[j].isspace() and text[j] not in '()': j += 1
            tok.append(text[i:j]); i = j
    def rd(p):
        out = []
        while p < len(tok):
            t = tok[p]
            if t == '(':
                sub, p = rd(p+1); out.append(sub)
            elif t == ')': return out, p+1
            else: out.append(t); p += 1
        return out, p
    return rd(0)[0]

def apply_ses(b, path, layer=pcbnew.B_Cu, keep_layers=False):
    tree = parse_sexp(open(path).read())[0]
    nout = None
    for e in tree:
        if isinstance(e, list) and e and e[0] == 'routes':
            for e2 in e:
                if isinstance(e2, list) and e2 and e2[0] == 'network_out': nout = e2
    def P(x, y): return pcbnew.VECTOR2I(int(round(float(x) * 100)), int(round(-float(y) * 100)))
    n = 0
    for netl in nout[1:]:
        if not (isinstance(netl, list) and netl[0] == 'net'): continue
        nn = b.FindNet(netl[1])
        for it in netl[2:]:
            if isinstance(it, list) and it[0] == 'via' and keep_layers:
                v = pcbnew.PCB_VIA(b); v.SetPosition(P(it[2], it[3])); v.SetViaType(pcbnew.VIATYPE_THROUGH)
                v.SetWidth(mm(2.2)); v.SetDrill(mm(1.0)); v.SetNet(nn); b.Add(v); n += 1  # perce >=1mm : fil a souder a la main (non traversant)
            if isinstance(it, list) and it[0] == 'wire':
                pth = it[1]; w = int(round(float(pth[2]) * 100)); pts = pth[3:]
                for k in range(0, len(pts) - 2, 2):
                    t = pcbnew.PCB_TRACK(b)
                    t.SetStart(P(pts[k], pts[k+1])); t.SetEnd(P(pts[k+2], pts[k+3]))
                    t.SetLayer((pcbnew.F_Cu if pth[1] == 'F.Cu' else pcbnew.B_Cu) if keep_layers else layer)
                    t.SetWidth(w); t.SetNet(nn); b.Add(t); n += 1
    return n

def run_freerouting(b, workdir, passes=30, timeout=600, keep_gnd=False, layers=1, skip_pads=()):
    os.makedirs(workdir, exist_ok=True)
    gnd = b.FindNet('GND')
    gpads = [p for f in b.GetFootprints() for p in f.Pads() if p.GetNetname() == 'GND']
    if not keep_gnd:
        for p in gpads: p.SetNetCode(0)
    sk = []
    for f in b.GetFootprints():
        for p in f.Pads():
            if (f.GetReference() + '.' + p.GetNumber()) in skip_pads: sk.append((p, p.GetNet())); p.SetNetCode(0)
    b.SetCopperLayerCount(layers)
    if layers == 1:
        for t in b.GetTracks():
            if t.Type() == pcbnew.PCB_TRACE_T: t.SetLayer(pcbnew.F_Cu)
    dsn = os.path.join(workdir, 'r.dsn'); ses = os.path.join(workdir, 'r.ses')
    if os.path.exists(ses): os.remove(ses)
    ok = pcbnew.ExportSpecctraDSN(b, os.path.abspath(dsn))
    if not ok: raise RuntimeError('DSN export failed')
    subprocess.run(['xvfb-run', '-a', 'java', '-jar', os.environ.get('FR_JAR', 'freerouting-1.9.0.jar'), '-de', dsn, '-do', ses, '-mp', str(passes)],
                   capture_output=True, text=True, timeout=timeout)
    b.SetCopperLayerCount(2)
    n = apply_ses(b, ses, keep_layers=(layers == 2))
    if layers == 1:
        for t in b.GetTracks():
            if t.Type() == pcbnew.PCB_TRACE_T: t.SetLayer(pcbnew.B_Cu)
    if not keep_gnd:
        for p in gpads: p.SetNet(gnd)
    for p, nn in sk: p.SetNet(nn)
    return n

def dseg(p, a, c):
    ax, ay = a; cx, cy = c; px, py = p
    dx, dy = cx-ax, cy-ay; L = dx*dx+dy*dy
    t = 0 if L == 0 else max(0, min(1, ((px-ax)*dx+(py-ay)*dy)/L))
    return math.hypot(px-(ax+t*dx), py-(ay+t*dy))

def check(b, skip=('GND', '')):
    items = collections.defaultdict(list)
    for f in b.GetFootprints():
        for p in f.Pads():
            n = p.GetNetname()
            if n: items[n].append(('pad', f.GetReference() + '.' + p.GetNumber(), (p.GetPosition().x, p.GetPosition().y), p.GetSize().x/2))
    length = 0
    for t in b.GetTracks():
        if t.Type() == pcbnew.PCB_TRACE_T:
            items[t.GetNetname()].append(('trk', (t.GetStart().x, t.GetStart().y), (t.GetEnd().x, t.GetEnd().y), t.GetWidth()/2))
            length += t.GetLength() / 1e6
    bad = []
    for n, its in items.items():
        if n in skip: continue
        parent = list(range(len(its)))
        def find(i):
            while parent[i] != i: parent[i] = parent[parent[i]]; i = parent[i]
            return i
        def touch(x, y):
            tol = 20000
            if x[0] == 'pad' and y[0] == 'pad': return math.dist(x[2], y[2]) < tol
            if x[0] == 'pad': x, y = y, x
            if x[0] == 'trk' and y[0] == 'pad': return dseg(y[2], x[1], x[2]) < max(tol, y[3]*0.3)
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
        if len(comps) > 1: bad.append((n, [sorted(v) for v in comps.values()]))
    return bad, length
