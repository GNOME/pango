#!/usr/bin/env python3

import sys

# From glib/gutf8.c:
#
#define UNICODE_VALID(Char) \
#    ((Char) < 0x110000 && (((Char) & 0xFFFFF800) != 0xD800))

def is_valid_unicode(ch):
    if ch < 0x110000 and (ch & 0xFFFFF800) != 0xD800:
        return True

    return False

if __name__ == '__main__':
    if len(sys.argv) < 2:
        sys.exit('Usage: gen-all-unicode.py OUTFILE')

    with open(sys.argv[1], 'wb') as f:
        for j in range(0, 2):
            for i in range(0, 65536):
                if is_valid_unicode(i):
                    f.write(chr(i).encode('utf-8', 'surrogatepass'))

                if j == 1:
                    f.write(b' ')

                if j == 0:
                    if i % 40 == 0 and i != 0:
                        f.write(b'\n')
                else:
                    if i % 20 == 0 and i != 0:
                        f.write(b'\n')

        f.write(b'\n')
