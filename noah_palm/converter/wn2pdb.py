# Copyright 2003 Krzysztof Kowalczyk

# History:
# 05/30/2003 - started

# Convert dictionary files to Noah for Palm *.pdb format

from __future__ import generators
import sys,os,string,struct,pickle,time,copy
import palmdb

# constants for controlling builiding a word cache
MAX_RECORD_LEN = 63900
MAX_WORDS_IN_CACHE = 500
MIN_WORDS_IN_CACHE = 300

simpleDicts = [
    ['plant_dictionary.txt', 'plant.pdb'],
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

def _numericPos(posTxt):
    posTxtToNum = [ ["n.", 0],
                    ["adj.", 1],
                    ["v.t.", 1],
                    ["v.i.", 1],
                    ["v.", 1],
                    ["adj.",2],
                    ["adv.",3],
                    ["pp.",3] ]
    posTxt = string.lower(posTxt)
    for mapping in posTxtToNum:
        if posTxt == mapping[0]:
            return mapping[1]
    assert 0, "%s is not a valid pos" % posTxt

class SimpleDictEntry:
    def __init__(self,data):
        self.data = data
    def getLemma(self):
        return self.data[0]
    def getPosTxt(self):
        return self.data[1]
    def getPosNumeric(self):
        return _numericPos(self.getPosTxt())
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
            lemma = string.lower(string.strip(line))
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

def mainProf():
    import hotshot
    profData = 'log2.dat'
    prof = hotshot.Profile(profData)
    prof.run("DoPlantDict()")
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

    def compress(self,word):
        prefixLen = _commonPrefixLen(self.prevWord,word)
        prefixLen = min(_PREFIX_LEN_LIMIT,prefixLen)
        self.lastInCommon = prefixLen
        self.prevWord = word
        if prefixLen==0:
            return word
        else:
            #Undone: which one is faster?
            #return struct.pack("c%ds"%(len(w)-prefixLen),chr(prefixLen),w[prefixLen:])
            return chr(prefixLen) + word[prefixLen:]

    def fWasLastCompressed(self):
        """Return true if the last word was prefix-compressed i.e. had some prefix
        common with previous word"""
        if self.lastInCommon == 0:
            return True
        else:
            return False

    def reset(self):
        """Reset the state of the compressor so that the next word won't be compressed.
        Needed for generating word cache"""
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

# Given a list of strings return the data representing those
# strings as packed in the format of records for pdb file
# i.e. a tuple of the following records:
# - list of records with packed strings
# - record with word cache
# - record with data needed to uncompress strings
def buildPdbDataForPackedStringsFromStrings(strs):
    recsWithPackedStrings = []
    recWithWordCache = palmdb.PDBRecordFromData(None)
    recWithDecompressionData = palmdb.PDBRecordFromData(None)
    return (recsWithPackedStrings,recWithWordCache,recWithDecompressionData)

def compressedWordsGen(words):
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

class StringCompressor:
    """Compress/uncompress strings."""
    def __init__(self,packStrings):
        # packStrings must be passed in from outside
        self.packStrings = packStrings
        self.revPackStrings = copy.copy(packStrings)
        self.revPackStrings.reverse()

    def _findCodeForStr(self,str):
        code = 255
        for packStr in self.revPackStrings:
            if len(str) >= len(packStr):
                if packStr == str[:len(packStr)]:
                    return code
            code -= 1
            assert code >= 0
        assert 0, "Didn't find compression code"

    def compress(self,str):
        strLeft = str
        result = []
        while len(strLeft)>0:
            code = self._findCodeForStr(strLeft)
            result.append(code)
            strLeft = strLeft[len(self.packStrings[code]):]
        return string.join([chr(c) for c in result],"")

    def uncompress(self,str):
        result = []
        for code in str:
            result.append(self.packStrings[ord(code)])
        return string.join(result,"")

    def getCompressionRecData(self):
        """return blob for compression data (to be stored as pdb record)
        Structure of the record with compression data:
          0  2                   int     maxComprStrLen
          2  2+2*maxComprStrLen  int[]   number of coded strings of a given
                                         length (1..maxComprStrLen)
          .. ..                  char[]  compressed strings, starting with
                                         those of length 1
        """
        prevLen = len(self.packStrings[0])
        stringsOfThisLen = 1
        stringsInfo = []
        for s in self.packStrings[1:]:
            sLen = len(s)
            if prevLen == sLen:
                stringsOfThisLen += 1
            else:
                assert sLen >= prevLen
                stringsInfo.append(stringsOfThisLen)
                stringsOfThisLen = 1
                prevLen += 1
                # we might not have strings of some lengths so for
                # those we put 0 strings
                while prevLen < sLen:
                    stringsInfo.append(0)
                    prevLen += 1
                assert prevLen == sLen
        stringsInfo.append(stringsOfThisLen)
        maxComprStrLen = len(stringsInfo)
        assert maxComprStrLen == prevLen
        d1 = struct.pack(">H", maxComprStrLen)
        d2 = struct.pack(">%dH" % len(stringsInfo), *stringsInfo)
        d3 = string.join(self.packStrings,"")
        d = d1 + d2 + d3
        return d            

def _buildDummyStringCompressor():
    packStrings = []
    for n in range(256):
        packStrings.append(chr(n))
    comp = StringCompressor(packStrings)
    return comp

def buildStringCompressor(strList):
    """Given a list of strings, build compressor object optimal for compressing
    those strings"""
    # UNDONE: build real compression data from words, not a dummy
    return _buildDummyStringCompressor()

def testCompress():
    wordsToTest = ["me", "him", "blah", "zipper", "groggle", "restful", "paranoia", "glam", "groblerz"]
    comp = _buildDummyStringCompressor()
    for word in wordsToTest:
        compWord = comp.compress(word)
        assert compWord == word
        uncompWord = comp.uncompress(compWord)
        assert word == uncompWord

    comp = buildStringCompressor(wordsToTest)
    for word in wordsToTest:
        compWord = comp.compress(word)
        uncompWord = comp.uncompress(compWord)
        assert word == uncompWord

def buildWordCacheRecData(wordsCacheEntries):
    """Given a list of entries for word cache information, build pdb record data,
    which for each entry has the following format:
     0 4 ulong  word number
     4 2 uint   record number
     6 2 uint   offset in the record
    """
    recData = []
    for cacheEntry in wordsCacheEntries:
        wordNo = cacheEntry[0]
        recNo = cacheEntry[1]
        offsetInRec = cacheEntry[2]
        recData.append( struct.pack( ">LHH", wordNo, recNo,offsetInRec ) )
    return string.join(recData,"")

def buildWordsRecs(words):
    """Given a list of words return pdb records with information about those
    words i.e. a record with compression data, records with compressed words,
    record with words cache"""
    print "buildWordsRecs start"
    wordComp = WordCompressor()
    strComp = buildStringCompressor(words)
    currRecDataLen = 0
    currRecData = []
    wordsRecsData = []
    wordCacheEntries = []
    fForceWordCache = True
    fForceNewRecord = False
    maxWordLen = 0
    wordNo = 0
    wordsSinceLastCacheEntry = 0
    longestWord = ""
    for word in words:
        wordLen = len(word)
        if wordLen > maxWordLen:
            maxWordLen = wordLen
            longestWord = word
        if currRecDataLen + wordLen > MAX_RECORD_LEN:
            fForceWordCache = True
            fForceNewRecord = True
        if wordsSinceLastCacheEntry > MAX_WORDS_IN_CACHE:
            fForceWordCache = True
        compWord = wordComp.compress(word)
        if not wordComp.fWasLastCompressed() and wordsSinceLastCacheEntry > MIN_WORDS_IN_CACHE:
            fForceWordCache = True
        if fForceNewRecord:
            assert currRecDataLen > 0
            assert len(currRecData) > 0
            recDataReal = string.join(currRecData,"")
            wordsRecsData.append(recDataReal)
            currRecData = []
            currRecDataLen = 0
            fForceNewRecord = False
        if fForceWordCache:
            wordCacheEntries.append( [wordNo,len(wordsRecsData),currRecDataLen] )
            print "New cache record, word=%s, wordNo=%d, recNo=%d, offsetInRec=%d" % (word,wordNo,len(wordsRecsData),currRecDataLen)
            wordComp.reset()
            fForceWordCache = False
            wordsSinceLastCacheEntry = 0
        compWord = strComp.compress(compWord)
        currRecData.append(compWord)
        currRecDataLen += len(compWord)
        wordNo += 1
        wordsSinceLastCacheEntry += 1
    if len(currRecData) > 0:
        assert currRecDataLen > 0
        recDataReal = string.join(currRecData,"")
        wordsRecsData.append(recDataReal)
    wordCacheRecData = buildWordCacheRecData(wordCacheEntries)
    compressionRecData = strComp.getCompressionRecData()
    print "buildWordsRecs end"
    return (compressionRecData, wordCacheEntries, wordsRecsData)
    
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

def byteArrayToBlob(byteArr):
    assert len(byteArr)>0
    blob = string.join([chr(t) for t in byteArr], "")
    return blob

def buildPosRecs(dictEntries):
    """build pdb records with pos (part-of-speech) data"""
    posDataBytes = []
    posRecsData = []
    currByte = 0
    n = 0
    for dictEntry in dictEntries:
        pos = dictEntry.getPosNumeric()
        assert pos<4
        currByte = currByte << 2
        currByte = currByte | pos
        n += 1
        if n == 4:
            posDataBytes.append(currByte)
            n = 0
            currByte = 0
            if len(posDataBytes) == MAX_RECORD_LEN:
                posRecsData.append(byteArrayToBlob(posDataBytes))
    if n != 0:
        posDataBytes.append(currByte)
    if len(posDataBytes)>0:
        posRecsData.append(byteArrayToBlob(posDataBytes))
    return posRecsData

def buildDefsRecs(defsList):
    """Build pdb records related to definitions i.e. a tuple with:
    - records with definitions lengths
    - record with compression data for definitions
    - records with definitions"""
    print "buildDefsRecs start"
    strComp = buildStringCompressor(defsList)
    currDefLensRec = []
    currRec = []
    currRecLen = 0
    defRecs = []
    defLensRecs = []
    for d in defsList:
        dCompressed = strComp.compress(d)

        dLen = len(dCompressed)

        if dLen <= 255:
            currDefLensRec.append(dLen)
        else:
            currDefLensRec.append(255)
            l1 = dLen / 256
            l2 = dLen % 256
            currDefLensRec.append(l1)
            currDefLensRec.append(l2)
        if len(currDefLensRec) > MAX_RECORD_LEN:
            defLensRecs.append( byteArrayToBlob(currDefLensRec) )
            currDefLensRec = []

        if currRecLen + dLen > MAX_RECORD_LEN:
            currRecData = string.join(currRec, "")
            defRecs.append(currRecData)
            currRecLen = 0
            currRec = []
        currRec.append(dCompressed)
        currRecLen += dLen
    if currRecLen > 0:
        assert len(currRec) > 0
        currRecData = string.join(currRec,"")
        defRecs.append(currRecData)
    if len(currDefLensRec) > 0:
        defLensRecs.append( byteArrayToBlob(currDefLensRec) )
    assert len(defRecs) > 0
    assert len(defLensRecs) > 0
    compressionRecData = strComp.getCompressionRecData()
    print "buildDefsRecs end"
    return (defLensRecs,compressionRecData,defRecs)

def DoSimpleDict(sourceDataFileName, dictName, dictOutFileName):
    # Undone: maybe automatically version (e.g. filename.pdb-N or filename-N.pdb ?)
    if os.path.exists(dictOutFileName):
        print sys.stderr, "skipped creating %s as it already exists" % dictOutFileName
    print "doing %s, %s, %s" % (sourceDataFileName,dictName,dictOutFileName)

    posRecsData = buildPosRecs(SimpleDictEntriesGen(sourceDataFileName))
    print "pos recs: %d" % len(posRecsData)
    for posRecData in posRecsData:
        print "len of posRecData: %d" % len(posRecData)
    wordsCount = 0
    maxWordLen = 0
    maxDefLen = 0
    wordsList = []
    defsList = []
    for dictEntry in SimpleDictEntriesGen(sourceDataFileName):
        wordsCount += 1
        word = dictEntry.getLemma()
        definition = dictEntry.getDefinition()
        if len(definition) > maxDefLen:
            maxDefLen = len(definition)
        if len(word)>maxWordLen:
            maxWordLen = len(word)
        wordsList.append(word)
        defsList.append(definition)
        # print word
    print "wordsCount: %d " % (wordsCount)
    posRecs = buildPosRecs(SimpleDictEntriesGen(sourceDataFileName))
    hasPosInfoP = 1
    posRecsCount = len(posRecs)
    (wordsCompressDataRec, wordCacheEntries, wordsRecs) = buildWordsRecs(wordsList)
    (defsLensRecs,defCompressDataRec,defsRecs) = buildDefsRecs(defsList)
    print "len of wordsCompressDataRec=%d" % len(wordsCompressDataRec)
    print "len of defCompressDataRec=%d" % len(defCompressDataRec)
    print "%d recs with words" % len(wordsRecs)
    print "%d recs with defs" % len(defsRecs)
    print "%d recs with defs lens" % len(defsLensRecs)

class SimplePdbBuilder:
    """Simplify building pdb out of records built. Usage: build and set
    records with required data, when done call buildPdb() to retrieve
    built pdb object"""
    def __init__(self):
        # variables that need to be set from outside before
        # we can write out the full pdb file
        self.wordsCount = None
        self.maxWordLen = None
        self.maxDefLen = None
        self.wordsRecs = None
        self.wordCacheRec = None
        self.wordsCompressInfoRec = None
        self.defsCompressInfoRec = None
        self.defLensRecs = None
        self.defsRecs = None
        self.posInfoRecs = None

    def writePdb(self,fileName):
        # make sure that we were given all the info necessary to
        # create full pdb file
        assert None != self.wordsCount
        assert None != self.maxWordLen
        assert None != self.maxDefLen
        assert None != self.wordsRecs
        assert None != self.wordCacheRec
        assert None != self.wordsCompressInfoRec
        assert None != self.defsCompressInfoRec
        assert None != self.defLensRecs
        assert None != self.defsRecs
        assert None != self.posInfoRecs



def DoPlantDict():
    DoSimpleDict(r"c:\kjk\noah\dicts\plant_dictionary.txt", r"Plant dict", r"c:\plant.pdb")

def DoDevilDict():
    DoSimpleDict(r"c:\kjk\noah\dicts\devils_noah.txt", r"Devil dict", r"c:\devil.pdb")

def DoAllSimpleDicts():
    for simpleDictDef in simpleDicts:
        DoSimpleDict(simpleDictDef[0], simpleDictDef[1], simpleDictDef[2])

def _isProfile():
    return len(sys.argv)>1 and (sys.argv[1] == 'prof' or sys.argv[1] == 'profile')

def _isTest():
    #return True
    return len(sys.argv)>1 and sys.argv[1] == 'test'

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
    elif _isTest():
        testCompress()
    else:
        #testPickle()
        #testTiming()
        #main()
        DoDevilDict()

