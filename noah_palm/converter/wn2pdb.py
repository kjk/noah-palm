# Copyright (C) 2001-2002 Krzysztof Kowalczyk

# History:
# 05/30/2003 - started

# Convert dictionary files to Noah for Palm *.pdb format

from __future__ import generators
import sys,os,string,struct

simpleDicts = [
    ['ddlatin.txt', 'latin', 'lating.pdb' ],
    ['Pol-Eng.txt', 'pol-eng', 'pol-eng.pdb'] ]

def _commonPrefixLen(s1, s2):
    """Return the length of the common prefix for strings s1 and s2 
    e.g. 2 for "blah" and "bloom" (length of "bl")"""
    maxCommon = min(len(s1),len(s2))
    for r in range(maxCommon):
        if s1[r] != s2[r]:
            return r
    return maxCommon

def _commonPrefixLen2(s1, s2):
    """Return the length of the common prefix for seqences s1 and s2 
    e.g. 2 for "blah" and "bloom" (length of "bl")"""
    len = 0
    for x,y in zip(s1,s2):
        if x==y: len += 1
        else: break
    return len

def _isEmptyString(str):
    if str.isspace() or len(str)==0: return True
    return False

def _isPos(str):
    """Return True if this is a valid part of speech"""
    return string.lower(str) in ["n.", "adj.", "v.t.", "v.i.", "adv.", "pp."]

class SimpleDictEntry:
    def __init__(self,data):
        self.data = data
    def getLemma(self):
        return self.data[0]
    def getPos(self):
        return self.data[1]
    def getDefinition(self):
        return self.data[2]

# states used when reading simple dictionary file
SS_NONE, SS_HAS_LEMMA, SS_READING_DEF = range(3)
ssNamesDict = { SS_NONE:"SS_NONE", SS_HAS_LEMMA:"SS_HAS_LEMMA", SS_READING_DEF:"SS_READING_DEF" }

def SimpleDictEntriesGen(fileName):
    """Iterator over entries in simple dictionary file. Returns SimpleDictEntry,
    one at a time"""
    fo = open(fileName)
    state = SS_NONE
    defBody = ""
    currLineNo = 0
    while True:
        line = fo.readline()
        currLineNo += 1
        if len(line) == 0:
            if state != SS_NONE:
                raise ValueError, "reached en of file but the current state is %s and not SS_NONE" % (ssNamesDict[state])
            break

        if state == SS_NONE:
            if _isEmptyString(line): continue
            lemma = string.strip(line)
            state = SS_HAS_LEMMA
            continue

        if state == SS_HAS_LEMMA:
            if _isEmptyString(line): continue
            pos = string.strip(line)
            if not _isPos(pos):
                raise ValueError, "invalid pos (%s) in line %d" % (pos,currLineNo)
            state = SS_READING_DEF
            defBody = ""
            continue

        if state == SS_READING_DEF:
            if _isEmptyString(line):
                if _isEmptyString(defBody):
                    raise ValueError, "definition of lemma %s on line %d is empty" % (lemma, currLineNo)
                state = SS_NONE
                dictEntry = SimpleDictEntry( [lemma,pos,defBody] )
                yield dictEntry
            else:
                defBody += string.strip(line)

def SimpleDictEntriesSortedGen(fileName):
    allEntries = {}
    for entry in SimpleDictEntriesGen(fileName):
        allEntries[entry.getLemma()] = entry
    allLemmasSorted = allEntries.keys().sort()
    for lemma in allLemmasSorted:
        yield allEntries[lemma]

def main():
    entryCount = 0
    plantSimpleDict = r'c:\kjk\src\mine\noah_dicts\dicts\plant_dictionary.txt'
    for dictEntry in SimpleDictEntriesGen(plantSimpleDict):
        entryCount += 1
    print "Entries: %d " % (entryCount)

def mainProf2():
    import profile
    profile.run("main()")

def mainProf():
    import hotshot
    profData = 'log2.dat'
    prof = hotshot.Profile(profData)
    prof.run("main()")
    prof.close()
    # this is the code to dump profiling stats
    #import hotshot.stats
    #stats = hotshot.stats.load(profData)
    #stats.sort_stats("cumulative")
    #stats.print_stats()

_PREFIX_LEN_LIMIT = 31

class WordCompressor:
    def __init__(self):
        self.prevWord = ''

    def compressWord(self,word):
        prefixLen = _commonPrefixLen(self.prevWord,word)
        prefixLen = min(_PREFIX_LEN_LIMIT,prefixLen)
        self.prevWord = word
        if prefixLen==0:
            return word
        else:
            return chr(prefixLen) + word[prefixLen:]

def CompressedWordsGen(words):
    """Generate compressed words from a collection of words"""
    prevWord = ''
    for w in words:
        prefixLen = _commonPrefixLen(prevWord,w)
        prefixLen = min(_PREFIX_LEN_LIMIT,prefixLen)
        prevWord = w
        if prefixLen==0:
            yield w
        else:
            #Undone: which one is faster?
            #yield struct.pack("c%ds"%(len(w)-prefixLen),chr(prefixLen),w[prefixLen:])
            yield chr(prefixLen) + w[prefixLen:]

def DoSimpleDict(sourceDataFileName, dictName, dictOutFileName):
    # Undone: maybe automatically version (e.g. filename.pdb-N or filename-N.pdb ?)
    if os.path.exists(dictOutFileName):
        print sys.stderr, "skipped creating %s as it already exists" % dictOutFileName

def DoAllSimpleDicts():
    for simpleDictDef in simpleDicts:
        DoSimpleDict(simpleDictDef[0], simpleDictDef[1], simpleDictDef[2])

def _isProfile():
    if len(sys.argv)>1:
        if sys.argv[1] == 'prof' or sys.argv[1] == 'profile':
            return True
    return False

if __name__ == "__main__":
    if _isProfile():
        mainProf()
    else:
        test_CompressedWordsGen()
        #main()

