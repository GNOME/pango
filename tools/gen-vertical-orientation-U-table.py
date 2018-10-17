#!/usr/bin/python
#coding:utf-8
import os
import sys

if len(sys.argv) != 2:
    print('usage;./' + os.path.basename(__file__)  + ' VerticalOrientation.txt')
    sys.exit(1)

#pick up all data from text
data = []
f = open(sys.argv[1], 'r')
for line in f:
    line = line.split("#")[0].strip()
    if len(line) == 0:
        continue

    coderange, vo = line.split(";")
    vo = vo.strip()

    codes = coderange.split("..")
    if len(codes) == 1:
        st = int(codes[0], 16)
        ed = st
    else:
        st = int(codes[0], 16)
        ed = int(codes[1], 16)

    data.append([st, ed, vo])
f.close()


#compress all data, replace Tu to U and Tr to R.
compressed = []
t = []
for d in data:
    if d[2] == 'Tu': d[2] = 'U'
    if d[2] == 'Tr': d[2] = 'R'

    if t == []:
        t = d
    else:
        if t[2] == d[2] and t[1] + 1 == d[0]:
            t[1] = d[1]
        else:
            compressed.append(t)
            t = d
compressed.append(t)


#dump vo=U
for d in compressed:
    if d[2] == 'U':
        print('{0x%04X, 0x%04X},' % tuple(d[0:2]))
