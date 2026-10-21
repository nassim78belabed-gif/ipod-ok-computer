# usage : serie.py <attente_s> "cmd1" "cmd2" ... (port COM3, DTR/RTS a 0 pour ne pas redemarrer la carte)
import serial, sys, time
s = serial.Serial()
s.port = 'COM3'; s.baudrate = 115200; s.timeout = 0.2
s.dtr = False; s.rts = False
s.open()
time.sleep(float(sys.argv[1]))
for cmd in sys.argv[2:]:
    s.write((cmd + '\n').encode())
    fin = time.time() + 3
    while time.time() < fin:
        l = s.readline()
        if l:
            print(l.decode('utf-8', 'replace').rstrip()); fin = time.time() + 1.5
s.close()
