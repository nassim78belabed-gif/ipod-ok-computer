import serial, time, sys
# journal.py [touches] : envoie les touches (une par une), puis enregistre le moniteur serie 30 min
s = serial.Serial(); s.port='COM3'; s.baudrate=115200; s.timeout=0.5; s.dtr=False; s.rts=False; s.open()
for c in (sys.argv[1] if len(sys.argv) > 1 else ''):
    s.write(c.encode()); time.sleep(0.4)
with open('journal.txt','a',encoding='utf-8') as f:
    fin = time.time() + 1800
    while time.time() < fin:
        l = s.readline()
        if l: f.write(time.strftime('%H:%M:%S ') + l.decode('utf-8','replace')); f.flush()
