# convert a file containing text to C's char * array 
# that can be included in C programs

import sys,os

if len(sys.argv) != 2:
    print "Usage: txtToC.py file"
    sys.exit(0)

fo = open(sys.argv[1], "rb")
data = fo.read()
fo.close()

def methodOne(data):
    # doesn't really work
    charsPerLine = 16 # how many characters per line
    count = 0
    outStr = "char *txt = {\n"
    for c in data:
        cAsHex = hex(ord(c))
        outStr += cAsHex
        if count == charsPerLine:
            outStr += ",\n"
            count = 0
        else:
            outStr += ", "
            count += 1
    if c!=0:
        outStr += "0x0"
    outStr += "};\n"
    return outStr

def fNeedsEscaping(c):
    if ord(c) == 0xd or ord(c) == 0xa:
        return True
    return False

def methodTwo(data):
    outStr = 'unsigned char txt[] = "'
    fWasPreviousd = False # was the previous character 0xd? (needed to work around double-0xd)
    for c in data:

        if ord(c) == 0xd and fWasPreviousD:
            fWasPreviousD = False
            continue

        if fNeedsEscaping(c):
            toAdd = "\\" + hex(ord(c))[1:] + "\" \""
            outStr += toAdd
        else:
            outStr += c
        if ord(c) == 0xd:
            fWasPreviousD = True
        else:
            fWasPreviousD = False

    outStr += "\\x0\";"
    return outStr

outStr = methodTwo(data)
print outStr;
