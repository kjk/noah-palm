#!/cygdrive/c/Progra~1/Python22/./python

# Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
# Copyright (C) 2002 Krzysztof Kowalczyk (krzysztofk@pobox.com)

# Purpose: build system for Noah Pro, Noah Lite and Thesaurus
# Generate makefile on the fly and then execute it. Can build debug
# and release versions, also can do a clean build

# History:
#  2002/11/05 - started
#  2002/11/07 - added thesaurus
#  2002/11/16 - added debug/release build distinction
#  2002/12/02 - added Noah Lite

# Todo:
# - rewrite script.gdb to load obj/foo.out instead of foo.out
# - finish

import string, sys, os

# should we do a clean build?
fDoClean = 0
# should we do a debug build? It's debug by default unless "rel"
# is given on command line
fDoDebug = 1

def CreateDirIfNotExists(dirName):
    try:
        os.mkdir( dirName )
    except OSError:
        pass

npd_objs = [ "noah_pro", "word_compress", "mem_leak", "display_info", "common", "extensible_buffer",
             "fs", "fs_ex", "fs_mem", "fs_vfs",
             "wn_lite_ex_support", "wn_pro_support", "simple_support", "ep_support" ]

nld_objs = [ "noah_lite", "word_compress", "mem_leak", "display_info", "common", "extensible_buffer",
             "fs", "fs_mem", "fs_ex", "fs_vfs",
             "wn_lite_ex_support" ]

thd_objs = [ "thes", "word_compress", "mem_leak", "display_info", "common", "extensible_buffer",
             "fs", "fs_mem", "fs_ex", "fs_vfs",
             "roget_support"]


def GenObjs( objList ):
    txt = "OBJS="
    for obj in objList:
        txt += "obj/%s.o " % obj
    return txt

def GenObjDepend( objList ):
    txt = ""
    for obj in objList:
        #txt += "%s.o:\n" % obj
        txt += "obj/%s.o:\n" % obj
        txt += "\t$(CC) $(CFLAGS) -c src/%s.c -o $@\n\n" % obj

    return txt

def GenNoahProMakefile():
    global npd_objs, fDoDebug
    objList = npd_objs
    if fDoDebug:
        txt = """
CC = m68k-palmos-gcc
LNFLAGS = -g
CFLAGS = -g  -Wall -I res -DNOAH_PRO -DEP_DICT -DWNLEX_DICT -DWN_PRO_DICT -DSIMPLE_DICT -DFS_VFS -DERROR_CHECK_LEVEL=2 -DMEM_LEAKS -DDEBUG -DSTRESS
"""
    else:
        txt = """
CC = m68k-palmos-gcc
LNFLAGS =
CFLAGS = -O2 -Wall -I res -DNOAH_PRO -DEP_DICT -DWNLEX_DICT -DWN_PRO_DICT -DSIMPLE_DICT -DFS_VFS -DERROR_CHECK_LEVEL=0
"""
    txt += GenObjs(objList)
    txt += "\n\n"
    txt += GenObjDepend(objList)
    if fDoDebug:
        txt += """
noah_pro.prc: $(OBJS) obj/noah_pro.res
	m68k-palmos-multilink -gdb-script script.gdb -g -libdir /usr/m68k-palmos/lib/ -L/usr/lib/gcc-lib/m68k-palmos/2.95.3-kgpd -L/prc-tools/m68k-palmos/lib -lgcc -fid NoAH -segmentsize 29k obj/*.o"""
    else:
        txt += """
noah_pro.prc: $(OBJS) obj/noah_pro.res
	m68k-palmos-multilink -libdir /usr/m68k-palmos/lib/ -L/usr/lib/gcc-lib/m68k-palmos/2.95.3-kgpd -L/prc-tools/m68k-palmos/lib -lgcc -fid NoAH -segmentsize 29k obj/*.o"""
    txt += """
	mv *.grc obj
	mv *.out obj
	build-prc --copy-prevention $@ "Noah Pro" "NoAH" obj/*.bin obj/*.grc
	ls -la *.prc

obj/noah_pro.res: res/noah_pro.rcp res/noah_pro_rcp.h
	pilrc -D STRESS -q -I res res/noah_pro.rcp obj
	touch $@

clean:
	rm -rf obj/* obj/*.out obj/*.grc
"""
    return txt

def GenNoahLiteMakefile():
    global nld_objs
    objList = nld_objs
    if fDoDebug:
        txt = """
CC = m68k-palmos-gcc
LNFLAGS = -g
CFLAGS = -g  -Wall -DNOAH_LITE -DWNLEX_DICT -DFS_VFS -DERROR_CHECK_LEVEL=2 -I res -DMEM_LEAKS -DDEBUG -DSTRESS
"""
    else:
        txt = """
CC = m68k-palmos-gcc
LNFLAGS =
CFLAGS = -O2 -Wall -DNOAH_LITE -DWNLEX_DICT -DFS_VFS -DERROR_CHECK_LEVEL=0 -I res
"""
    txt += GenObjs(objList)
    txt += "\n\n"
    txt += GenObjDepend(objList)
    if fDoDebug:
        txt += """
noah_lite.prc: $(OBJS) obj/noah_lite.res
	m68k-palmos-multilink -gdb-script script.gdb -g -libdir /usr/m68k-palmos/lib/ -L/usr/lib/gcc-lib/m68k-palmos/2.95.3-kgpd -L/prc-tools/m68k-palmos/lib -lgcc -fid KJK0 -segmentsize 29k obj/*.o"""
    else:
        txt += """
noah_lite.prc: $(OBJS) obj/noah_lite.res
	m68k-palmos-multilink -libdir /usr/m68k-palmos/lib/ -L/usr/lib/gcc-lib/m68k-palmos/2.95.3-kgpd -L/prc-tools/m68k-palmos/lib -lgcc -fid KJK0 -segmentsize 29k obj/*.o"""
    txt += """
	mv *.grc obj
	mv *.out obj
	build-prc --copy-prevention $@ "Noah Lite" "KJK0" obj/*.bin obj/*.grc
	ls -la *.prc

obj/noah_lite.res: res/noah_lite.rcp res/noah_lite_rcp.h
	pilrc -D STRESS -q -I res res/noah_lite.rcp obj
	touch $@

clean:
	rm -rf obj/* obj/*.out obj/*.grc
"""
    return txt

def GenThesMakefile():
    global thd_objs
    objList = thd_objs
    if fDoDebug:
        LNFLAG = "-g"
        CCFLAG1 = "-g"
        PILRCFLAG = "-D STRESS"
        CCFLAG = "-DMEM_LEAKS -DERROR_CHECK_LEVEL=2 -DDEBUG -DSTRESS"
    else:
        LNFLAG = ""
        CCFLAG1 = "-O2"
        PILRCFLAG = ""
        CCFLAG = "-DERROR_CHECK_LEVEL=0"
    txt = """
CC = m68k-palmos-gcc
LNFLAGS = %s
CFLAGS = %s -Wall -DTHESAURUS %s -I res
""" % (LNFLAG, CCFLAG1, CCFLAG)

    txt += GenObjs(objList)
    txt += "\n\n"
    txt += GenObjDepend(objList)
    txt += """
thes.prc: obj/thes.res $(OBJS)
	m68k-palmos-multilink -gdb-script script.gdb %s -libdir /usr/m68k-palmos/lib/ -L/usr/lib/gcc-lib/m68k-palmos/2.95.3-kgpd -L/prc-tools/m68k-palmos/lib -lgcc -fid TheS -segmentsize 29k obj/*.o""" % LNFLAG
    txt += """
	mv *.grc obj
	mv *.out obj
	build-prc --copy-prevention $@ "Thesaurus" "TheS" obj/*.bin obj/*.grc
	ls -la *.prc

obj/thes.res: res/thes.rcp res/thes_rcp.h
	pilrc -q %s -I res res/thes.rcp obj
	touch $@

clean:
	rm -rf obj/*
""" % PILRCFLAG
    return txt

nameGenProcMap = [ ["noah_pro", "noahpro", "np", GenNoahProMakefile], ["noah_lite", "nl", "noahlite", GenNoahLiteMakefile], ["thes", "th", GenThesMakefile] ]

def FindGenProcByName2(name, map):
    for n in map[:-1]:
        if n == name: return map[-1]
    return None1

def FindGenProcByName(name):
    for oneMap in nameGenProcMap:
        func = FindGenProcByName2(name, oneMap)
        if func != None:
            return func
    return None

def PrintUsageAndQuit():
    print "Usage: b.py [clean] noah_pro|thes|noah_lite|np|th|nl"
    sys.exit(0)

args = sys.argv
fDoClean = 0
fDoNoahPro = 0
fDoNoahLite = 0
fDoThes = 0
for a in args:
    if a == "clean": fDoClean = 1
    if a == "release": fDoDebug = 0
    if a == "rel": fDoDebug = 0
    if a == "noah_pro" or a == "np":
       fDoNoahPro = 1
       if fDoNoahLite or fDoThes: PrintUsageAndQuit()
    if a == "noah_lite" or a == "nl":
       fDoNoahLite = 1
       if fDoNoahPro or fDoThes: PrintUsageAndQuit()
    if a == "thes" or a == "th":
       fDoThes = 1
       if fDoNoahPro or fDoNoahLite: PrintUsageAndQuit()

if not (fDoNoahLite or fDoNoahPro or fDoThes): PrintUsageAndQuit()

if fDoNoahPro:
   mf = GenNoahProMakefile()
   makefileFileName = "noahpro.mk"
   target = "noah_pro.prc"

if fDoNoahLite:
   mf = GenNoahLiteMakefile()
   makefileFileName = "noahlite.mk"
   target = "noah_lite.prc"

if fDoThes:
   mf = GenThesMakefile()
   makefileFileName = "thes.mk"
   target = "thes.prc"

fp = open( makefileFileName, "w" )
fp.write( mf )
fp.close()

CreateDirIfNotExists( "obj" )
if fDoClean:
    os.system("make -f %s clean" % makefileFileName)
os.system("make -f %s %s" % (makefileFileName,target))

