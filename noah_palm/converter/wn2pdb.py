# Copyright 2003 Krzysztof Kowalczyk

# History:
# 05/30/2003 - started

# Convert dictionary files to Noah for Palm *.pdb format

from __future__ import generators
import sys,os,string,struct,pickle,time
import palmdb

simpleDicts = [
    ['ddlatin.txt', 'latin', 'lating.pdb' ],
    ['Pol-Eng.txt', 'pol-eng', 'pol-eng.pdb'] ]


def _pickleToFile(obj,file):
    """Pickle (in binary mode) a given object obj to a file"""
    fo = open(file,"wb")
    pickle.dump(obj,fo,True)
    fo.close()

def _unpickleFromFile(file):
    fo = open(file,"rb")
    obj = pickle.load(fo)
    fo.close()

def _timeCall(stmt=""):
    """Debug only function, times how much it takes to execute stmt"""
    tStart = time.clock()
    eval(stmt)
    tEnd = time.clock()
    tDur = tEnd - tStart
    return tDur

def _readEngPolWords():
    """Reads head words from english-polish dictionary (in eng_pol.txt)
    Returns a hash with eng-pol words as keys"""
    engPol = r'c:\kjk\src\mine\dict_data\eng_pol.txt'
    fo = open(engPol)
    wordsHash = {}
    for l in fo.readlines():
        try:
            if l[0].isalpha():
                if l[-1] == '\n':
                    l = l[:-1]
                # turns out that this version is more pickle-efficient than
                # doing wordsHash[l] = l
                wordsHash[l] = 1
        except:
            # skip the cases where l is empty so l[0] will produce an exception
            # Q: is it faster than explicitly checking for len(l)>0 ?
            pass
    fo.close()
    return wordsHash

def readEngPolWords():
    """Reads head words from english-polish dictionary. It'll either read them
    from engPol file or a pickled hash (if it was computed during previous run).
    Getting pre-calculated, pickled data is many times faster (1500 in my tests).
    Returns a hash with eng-pol words as keys"""
    pickledFile = r'eng_pol_words.pickle'
    wordsHash = None
    try:
        wordsHash = _unpickleFromFile(pickledFile)
    except:
        pass
    if wordsHash == None:
        wordsHash = _readEngPolWords()
        _pickleToFile(wordsHash,pickledFile)
    return wordsHash

def _commonPrefixLen(s1, s2):
    """Return the length of the common prefix for strings s1 and s2 
    e.g. 2 for "blah" and "bloom" (length of "bl")"""
    maxCommon = min(len(s1),len(s2))
    for r in range(maxCommon):
        if s1[r] != s2[r]:
            return r
    return maxCommon

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
def _getStateName(state):
    return ["SS_NONE", "SS_HAS_LEMMA", "SS_READING_DEF"][state]

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
                raise ValueError, "reached an end of a file but the current state is %s and not SS_NONE" % (_getStateName[state],)
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
        self.reset()

    def compressWord(self,word):
        prefixLen = _commonPrefixLen(self.prevWord,word)
        prefixLen = min(_PREFIX_LEN_LIMIT,prefixLen)
        self.lastInCommon = prefixLen
        self.prevWord = word
        if prefixLen==0:
            return word
        else:
            return chr(prefixLen) + word[prefixLen:]

    def getLastInCommon(self):
        return self.lastInCommon

    def reset(self):
        self.prevWord = ''
        self.lastInCommon = 0

class FreqTable:
    def __init__(self,dx,dy):
        self.freqTable = [0 for i in range(dx*dy)]
        self.dx = dx
        self.dy = dy
    def inc(self,x,y):
        self.freqTable[x+y*dy] += 1
    def incStr(self,str):
        x = ord(str[0])
        for c in str[1:]:
            y = ord(c)
            self.inc(x,y)
            x = y

class Compressor(object):
    def __init__(self):
        pass
# what I need to be able to do:
# - compress strings (str->str)
# - decompress strings (str->str)

# Given a list of strings return the data representing those
# strings as packed in the format of records for pdb file
# i.e. a tuple of the following records:
# - list of records with packed strings
# - record with word cache
# - record with data needed to uncompress strings
def GetPdbDataForPackedStringsFromStrings(strs):
    recsWithPackedStrings = []
    recWithWordCache = palmdb.PDBRecordFromData(None)
    recWithDecompressionData = palmdb.PDBRecordFromData(None)
    return (recsWithPackedStrings,recWithWordCache,recWithDecompressionData)

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

# Generates all the information needed for packed words:
# - pdb records with packed words
class WordsPacker:
    def __init__(self):
        pass

class PackInfo:
    def __init__(self):
        pass
    def _reserverCodes(self,codesCount):
        assert codesCount <= 255
    def _getStrByCode(self,str):
        pass

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

class CachingGen:
    def __init__(self,origGen):
        self.origGen = origGen
        self.fFirstRun = True
        self.cached = []
    def gen(self):
        if self.fFirstRun:
            for el in self.origGen():
                self.cached.append(el)
                yield el
            self.fFirstRun = False
        else:
            for el in self.cached:
                yield el

if __name__ == "__main__":
    if _isProfile():
        mainProf()
    else:
        #test_CompressedWordsGen()
        #testPickle()
        #testTiming()
        #main()
        print "this is only a test"
        tst()

# The plan: let's do wn_mini_f.pdb
