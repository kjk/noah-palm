#!/cygdrive/c/Progra~1/Python22/./python

# Author: Krzysztof Kowalczyk (krzysztofk@pobox.com)
# Copyright (C) 2002 Krzysztof Kowalczyk (krzysztofk@pobox.com)

# Purpose: build system for Noah Pro, Noah Lite and Thesaurus
# Generate makefile on the fly and then execute it. Can build debug
# and release versions, also can do a clean build

# History:
#  2002/11/05 - started
#  2002/11/07 - added thesaurus

# Todo:
# - rewrite script.gdb to load obj/foo.out instead of foo.out
# - finish

import string, sys, os

fDoClean = 0 # should we do a clean build?

def CreateDirIfNotExists(dirName):
    try:
        os.mkdir( dirName )
    except OSError:
        pass

npd_objs = [ "noah_pro", "wn_lite_ex_support", "wn_pro_support", "simple_support", "ep_support",
             "word_compress", "mem_leak", "display_info", "common", "extensible_buffer",
             "fs", "fs_ex", "fs_mem", "fs_vfs" ]

nld_objs = [ "noah_pro", "wn_lite_ex_support", "wn_pro_support", "simple_support", "ep_support",
             "word_compress", "mem_leak", "display_info", "common", "extensible_buffer",
             "fs", "fs_ex", "fs_mem", "fs_vfs" ]

thd_objs = [ "thes", "roget_support", "word_compress", "display_info", "common",
             "mem_leak", "extensible_buffer", "fs", "fs_mem" ]


def GenObjs( objList ):
    txt = "OBJS="
    for obj in objList:
        txt += "obj/%s.o " % obj
        #txt += "%s.o " % obj
    return txt

def GenObjDepend( objList ):
    txt = ""
    for obj in objList:
        #txt += "%s.o:\n" % obj
        txt += "obj/%s.o:\n" % obj
        txt += "\t$(CC) $(CFLAGS) -c src/%s.c -o $@\n\n" % obj

    return txt

def GenNoahProMakefile():
    global npd_objs
    objList = npd_objs
    txt = """
CC = m68k-palmos-gcc
LNFLAGS = -g
CFLAGS = -g -Wall -I res -DNOAH_PRO -DEP_DICT -DWNLEX_DICT -DWN_PRO_DICT -DSIMPLE_DICT -DFS_VFS -DMEM_LEAKS -DERROR_CHECK_LEVEL=2 -DDEBUG -DSTRESS
"""
    txt += GenObjs(objList)
    txt += "\n\n"
    txt += GenObjDepend(objList)
    txt += """
noah_pro.prc: $(OBJS) obj/noah_pro.res
	m68k-palmos-multilink -gdb-script script.gdb -g -libdir /usr/m68k-palmos/lib/ -L/usr/lib/gcc-lib/m68k-palmos/2.95.3-kgpd -L/prc-tools/m68k-palmos/lib -lgcc -fid NoAH obj/*.o
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
    objList = npd_objs
    txt = """
CC = m68k-palmos-gcc
LNFLAGS = -g
CFLAGS = -g -Wall -I res
"""
    return txt

def GenThesMakefile():
    global thd_objs
    objList = thd_objs
    txt = """
CC = m68k-palmos-gcc
LNFLAGS = -g
CFLAGS = -g -Wall -DTHESAURUS -DMEM_LEAKS -I res
"""
    txt += GenObjs(objList)
    txt += "\n\n"
    txt += GenObjDepend(objList)
    txt += """
thes.prc: obj/thes.res $(OBJS)
	m68k-palmos-multilink -gdb-script script.gdb -g -libdir /usr/m68k-palmos/lib/ -L/usr/lib/gcc-lib/m68k-palmos/2.95.3-kgpd -L/prc-tools/m68k-palmos/lib -lgcc -fid TheS obj/*.o
	mv *.grc obj
	mv *.out obj
	build-prc --copy-prevention $@ "Thesaurus" "TheS" obj/*.bin obj/*.grc
	ls -la *.prc

obj/thes.res: res/thes.rcp res/thes_rcp.h
	pilrc -q -I res res/thes.rcp obj
	touch $@

clean:
	rm -rf obj/*
"""
    return txt

nameGenProcMap = [ ["noah_pro", "np", GenNoahProMakefile], ["noah_lite", "nl", GenNoahLiteMakefile], ["thes", "th", GenThesMakefile] ]

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

def GenMakefile(type):
    if type == "noah_pro":
        return GenNoahProMakefile()
    else:
        raise OSError( "hello" )

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

#print mf

fp = open( makefileFileName, "w" )
fp.write( mf )
fp.close()

CreateDirIfNotExists( "obj" )
if fDoClean:
    os.system("make -f %s clean" % makefileFileName)
os.system("make -f %s %s" % (makefileFileName,target))

