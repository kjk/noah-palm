# Copyright 2003 Krzysztof Kowalczyk

# History:
# 05/30/2003 - started

# Convert dictionary files to Noah for Palm *.pdb format

from __future__ import generators
import sys,os,string,struct,structhelper,pickle,time,copy
import palmdb
import noahpdbdump # probably should extract things needed to a separate file

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
                if len(defBody) > 0:
                    defBody += " "
                defBody += string.strip(line)

def SimpleDictEntriesSortedGen(fileName):
    allEntries = {}
    for entry in SimpleDictEntriesGen(fileName):
        allEntries[entry.getLemma()] = entry
    allLemmasSorted = allEntries.keys().sort()
    for lemma in allLemmasSorted:
        yield allEntries[lemma]

profData = 'log.dat'
def mainProf():
    import hotshot
    global profData
    print "doing profiling"
    prof = hotshot.Profile(profData)
    prof.run("DoDevilDict()")
    prof.close()

def dumpProfileStats():
    import hotshot.stats
    global profData
    print "dump profiling data"
    stats = hotshot.stats.load(profData)
    stats.sort_stats("cumulative")
    stats.print_stats()

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
            return chr(0) + word
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

def testWordCompressor():
    wc = WordCompressor()
    for w in ["abasement", "abatis", "abdication"]:
        wCompressed = wc.compress(w)
        print wCompressed

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
        #UNDONE: turn those into real constants
        self.NEXT_TABLE_IDX = 0
        self.CODE_IDX = 1
        self.mainTable = self.buildCompressTable(packStrings)

    def buildCompressTable(self,packStrings):
        """given an array of pack strings build a table for fast compression"""
        table = {}
        code = 0
        for str in packStrings:
            # build an entry for a given pack string
            currTable = table
            prevTable = None
            # construct a stream of hash-tables leading to the end-point
            # for this string
            for c in str:
                if not currTable.has_key(c):
                    # first one points to a dictionary for longer strings
                    # second for the code (if this is also a terminal string)
                    currTable[c] = [{},None]
                    prevTable = currTable
                    currTable = currTable[c][self.NEXT_TABLE_IDX]
                else:
                    prevTable = currTable
                    currTable = currTable[c][self.NEXT_TABLE_IDX]
            # put the code of the string in the end-point
            prevTable[c][self.CODE_IDX] = code
            code += 1
        return table

    def _findCodeForStr(self,str):
        currTable = self.mainTable
        lastValidCode = -1
        for c in str:
            if currTable.has_key(c):
                if currTable[c][self.CODE_IDX] != None:
                    lastValidCode = currTable[c][self.CODE_IDX]
                currTable = currTable[c][self.NEXT_TABLE_IDX]
            else:
                assert lastValidCode != -1
                return lastValidCode
        # the whole string matched
        assert lastValidCode != -1
        return lastValidCode

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

def _getSmallestFreq( l ):
    m = l[0] # min value
    for el in l[1:]:
        if el[0] < m:
            m = el[0]
    return m

def _getPosOfThisFreq(l,freq):
    n = 0
    for el in l:
        if el[0] == freq:
            return n
        n += 1
    assert 0, "didn't find this freq in the list"

def _freqSortFunc(x,y):
    freqX = x[0]
    freqY = y[0]
    if freqX > freqY:
        return -1
    elif freqX < freqY:
        return 1
    else:
        return 0

def _freqSortFuncByCompression(x,y):
    freqX = x[0]
    strX = x[1]
    valX = freqX*(len(strX)-1)
    freqY = y[0]
    strY = y[1]
    valY = freqY*(len(strY)-1)
    if valX > valY:
        return -1
    elif valX < valY:
        return 1
    else:
        return 0 

def _sortByLenFunc(x,y):
    if len(x) > len(y):
        return 1
    elif len(x) < len(y):
        return -1
    else:
        return 0

def dumpListByFreq(l):
    lCopy = copy.copy(l)
    lCopy.sort( _freqSortFunc )
    for el in lCopy:
        freq = el[0]
        s = el[1]
        print "freq: %d, '%s'" % (freq,s)

def sortStringsByLen(l):
    l.sort(_sortByLenFunc )

def sortFreqList(l):
    """List l contains elements that are 2-element arrays, first element is a
    number (frequency) and second is a string. Sort the list in-place by frequency"""
    #l.sort( _freqSortFunc )
    l.sort( _freqSortFuncByCompression )

class CompInfoGen:
    """Given a list of strings, generate compression info (packStrings)"""
    def __init__(self):
        self.table = {}
        self.FREQ_IDX = 0
        self.NEXT_TABLE_IDX = 1
        self.FULL_STR_IDX = 2
        self.STR_MAX_LEN = 5

    def _incStr(self,s):
        currTable = self.table
        for c in s:
            if not currTable.has_key(c):
                currTable[c] = [0,{},""]
            prevTable = currTable
            currTable = currTable[c][self.NEXT_TABLE_IDX]
        prevTable[c][self.FREQ_IDX] += 1
        prevTable[c][self.FULL_STR_IDX] = s

    def incStr(self,s):
        for n in range(self.STR_MAX_LEN):
            strLen = n+1
            substringsCount = len(s)-strLen+1
            if substringsCount > 0:
                for i in range(substringsCount):
                    currStr = s[i:i+strLen]
                    self._incStr(currStr)

    def _getFlattenedFreqList(self,result=[],table=None):
        if table==None:
            table = self.table
        # first add all strings to result table
        for c in table.keys():
            if len(table[c][self.FULL_STR_IDX])>1:
                result.append( [table[c][self.FREQ_IDX], table[c][self.FULL_STR_IDX]] )
        for c in table.keys():
            newTable = table[c][self.NEXT_TABLE_IDX]
            # UNDONE: we could skip if new table has no keys in it, I think
            self._getFlattenedFreqList(result,newTable)
        return result

    def buildPackStrings(self):
        """Given frequency table filled in, generate packStrings for best
        compression (except it probably isn't optimal)"""
        # we need to reserve first 32
        reservedCodes = 32
        codesLeft = 256-reservedCodes
        packStrings = [chr(n) for n in range(reservedCodes)]
        # we need all one-letter ones
        for c in self.table.keys():
            packStrings.append(c)
            codesLeft -= 1
        assert codesLeft > 0
        # now select multi-letter ones for best compression
        freqList = self._getFlattenedFreqList()
        #dumpListByFreq(freqList)
        sortFreqList(freqList)
        strsToAdd = []
        for n in range(codesLeft):
            strsToAdd.append( freqList[n][1] )
            sortStringsByLen(strsToAdd)
        for el in strsToAdd:
            assert codesLeft > 0
            packStrings.append(el)
            codesLeft -= 1
        print packStrings
        return packStrings

class CompInfoGenOld:
    """Given a list of strings, generate compression info (packStrings)"""
    def __init__(self,dx=256,dy=256):
        self.freqTable = [0 for i in range(dx*dy)]
        self.dx = dx
        self.dy = dy
    def inc(self,x,y):
        self.freqTable[x+y*self.dy] += 1
    def val(self,x,y):
        return self.freqTable[x+y*self.dy]
    def incStr(self,s):
        x = ord(s[0])
        for c in s[1:]:
            y = ord(c)
            self.inc(x,y)
            x = y
    def _getStrFromPos(self,pos):
        x = pos % self.dy
        y = pos / self.dy
        s = chr(x)+chr(y)
        return s
    def _getMostFrequent(self,n):
        result = []
        currThresh = -1
        elPos = -1
        for freq in self.freqTable:
            elPos += 1
            if freq >= currThresh:
                # need to insert or replace element from the list of most
                # frequently used elements
                if len(result) < n:
                    # add to the list since we don't have n elements yet
                    result.append( [freq, self._getStrFromPos(elPos)] )
                    if len(result) == n:
                        # when we finally found all elements, we need to
                        # adjust threshold from permitting anything to the
                        # real threshold
                        currThresh = _getSmallestFreq(result)
                else:
                    if freq > currThresh: # we don't want to fill full table with existing freq's
                        # need to replace element on the list
                        pos = _getPosOfThisFreq(result,currThresh)
                        result[pos] = [freq, self._getStrFromPos(elPos)]
                        currThresh = _getSmallestFreq(result)
        return result

    def buildPackStrings(self):
        reservedCodes = 32
        #dumpListByFreq(flist)
        packStrings = [chr(n) for n in range(reservedCodes)]
        codesLeft = 256-reservedCodes
        for x in range(256):
            for y in range(256):
                if self.val(x,y) > 0:
                    #print "found for x=%d ('%s')" % (x,chr(x))
                    packStrings.append(chr(x))
                    codesLeft -= 1
                    break
        flist = self._getMostFrequent(codesLeft)
        for el in flist:
            packStrings.append( el[1] )
        assert 256 == len(packStrings)
        print packStrings
        return packStrings

def buildStringCompressorOld(strList):
    """Given a list of strings, build compressor object optimal for compressing
    those strings"""
    # UNDONE: build even better compression
    comp = StringCompressor(packStrings)
    return comp

def buildStringCompressor(strList):
    """Given a list of strings, build compressor object optimal for compressing
    those strings"""
    #ft = CompInfoGenOld()
    ft = CompInfoGen()
    for s in strList:
        ft.incStr(s)
    packStrings = ft.buildPackStrings()
    assert 256 == len(packStrings)
    comp = StringCompressor(packStrings)
    return comp

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
            compWord = wordComp.compress(word)
            fForceWordCache = False
            wordsSinceLastCacheEntry = 0
        compWord = strComp.compress(compWord)
        currRecData.append(compWord)
        currRecDataLen += len(compWord)
        wordNo += 1
        wordsSinceLastCacheEntry += 1
    # a hack to make decompression work: put a zero-byte (compressed) at the end
    # so that decompression knows when to stop (UNDONE: is it really necessary)
    zeroWordCompressed = strComp.compress("\x00")
    currRecData.append(zeroWordCompressed)
    currRecDataLen += len(zeroWordCompressed)
    if len(currRecData) > 0:
        assert currRecDataLen > 0
        recDataReal = string.join(currRecData,"")
        wordsRecsData.append(recDataReal)
    wordCacheRecData = buildWordCacheRecData(wordCacheEntries)
    compressionRecData = strComp.getCompressionRecData()
    print "buildWordsRecs end"
    return (compressionRecData, wordCacheRecData, wordsRecsData)
    
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

        if dLen < 255:
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

SIMPLE_TYPE             = "simp"
WORDNET_LITE_TYPE       = "wnet"
WORDNET_PRO_DEMO_TYPE   = "wnde"
WORDNET_PRO_TYPE        = "wn20"
NOAH_PRO_CREATOR        = "NoAH"
NOAH_LITE_CREATOR       = "NoaH"
THES_CREATOR            = "TheS"
ROGET_TYPE              = "rget"

class RecPrinter:
    def __init__(self):
        self.no = 0

    def pr(self, msg):
        print "rec no: %2d, %s" % (self.no,msg)
        self.no += 1

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

    def _buildFirstRecBlob(self):
        wordsRecsCount = len(self.wordsRecs)
        assert wordsRecsCount > 0
        defsLenRecsCount = len(self.defLensRecs)
        assert defsLenRecsCount > 0
        defsRecsCount = len(self.defsRecs)
        assert defsRecsCount > 0
        fHasPosInfo = 1
        posRecsCount = len(self.posInfoRecs)
        assert posRecsCount > 0
        fmt = structhelper.GetFmtFromMetadata(noahpdbdump.simpleFirstRecDef)
        blob = struct.pack( fmt,self.wordsCount, wordsRecsCount, defsLenRecsCount,
            defsRecsCount, self.maxWordLen, self.maxDefLen, fHasPosInfo, posRecsCount )
        return blob

    def writePdb(self,dictName,fileName):
        # make sure that we were given all the info necessary to
        # create full pdb file
        assert None != self.wordsCount
        assert None != self.maxWordLen
        assert self.maxWordLen > 4
        assert None != self.maxDefLen
        assert self.maxDefLen > 4
        assert None != self.wordsRecs
        assert None != self.wordCacheRec
        assert None != self.wordsCompressInfoRec
        assert None != self.defsCompressInfoRec
        assert None != self.defLensRecs
        assert None != self.defsRecs
        assert None != self.posInfoRecs
        recs = []
        rp = RecPrinter()
        recs.append( palmdb.PDBRecordFromData(self._buildFirstRecBlob()) )
        rp.pr("first rec blob")
        assert not isinstance(self.wordCacheRec,list)
        recs.append( palmdb.PDBRecordFromData(self.wordCacheRec) )
        rp.pr("wordCacheRec")
        recs.append( palmdb.PDBRecordFromData(self.wordsCompressInfoRec) )
        rp.pr("wordsCompressInfoRec")
        recs.append( palmdb.PDBRecordFromData(self.defsCompressInfoRec) )
        rp.pr("defsCompressInfoRec")
        for rec in self.wordsRecs:
            recs.append(palmdb.PDBRecordFromData(rec))
            #print rec
            rp.pr("wordsRecs")
        for rec in self.defLensRecs:
            recs.append(palmdb.PDBRecordFromData(rec))
            rp.pr("defLensRecs")
        for rec in self.defsRecs:
            recs.append(palmdb.PDBRecordFromData(rec))
            rp.pr("defsRecs")
        for rec in self.posInfoRecs:
            recs.append( palmdb.PDBRecordFromData(rec) )
            rp.pr("posInfoRecs")
        pdb = palmdb.PDB()
        pdb.name = dictName
        pdb.creator = NOAH_PRO_CREATOR
        pdb.dbType = SIMPLE_TYPE
        pdb.records = recs
        pdb.saveAs(fileName,True)
        print "consider pdb written"
        

def DoSimpleDict(sourceDataFileName, dictName, dictOutFileName):
    # Undone: maybe automatically version (e.g. filename.pdb-N or filename-N.pdb ?)
    if os.path.exists(dictOutFileName):
        print sys.stderr, "skipped creating %s as it already exists" % dictOutFileName
    print "doing %s, %s, %s" % (sourceDataFileName,dictName,dictOutFileName)
    pdbBuilder = SimplePdbBuilder()
    posRecsData = buildPosRecs(SimpleDictEntriesGen(sourceDataFileName))
    pdbBuilder.posInfoRecs = posRecsData
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
    pdbBuilder.wordsCount = wordsCount
    pdbBuilder.maxWordLen = maxWordLen
    pdbBuilder.maxDefLen = maxDefLen
    print "wordsCount: %d " % (wordsCount)
    posRecs = buildPosRecs(SimpleDictEntriesGen(sourceDataFileName))
    hasPosInfoP = 1
    posRecsCount = len(posRecs)

    (wordsCompressDataRec, wordCacheEntriesRec, wordsRecs) = buildWordsRecs(wordsList)
    pdbBuilder.wordsRecs = wordsRecs
    pdbBuilder.wordCacheRec = wordCacheEntriesRec
    pdbBuilder.wordsCompressInfoRec = wordsCompressDataRec
    
    (defsLensRecs,defsCompressDataRec,defsRecs) = buildDefsRecs(defsList)
    pdbBuilder.defsCompressInfoRec = defsCompressDataRec
    pdbBuilder.defLensRecs = defsLensRecs
    pdbBuilder.defsRecs = defsRecs
    print "len of wordsCompressDataRec=%d" % len(wordsCompressDataRec)
    print "len of defsCompressDataRec=%d" % len(defsCompressDataRec)
    print "%d recs with words" % len(wordsRecs)
    print "%d recs with defs" % len(defsRecs)
    print "%d recs with defs lens" % len(defsLensRecs)
    pdbBuilder.writePdb(dictName,dictOutFileName)    

def DoPlantDict():
    DoSimpleDict(r"c:\kjk\noah\dicts\plant_dictionary.txt", r"Plant dict", r"c:\plant.pdb")

def DoDevilDict():
    #DoSimpleDict(r"c:\kjk\noah\dicts\devils_noah.txt", r"Devil dict", r"c:\devil.pdb")
    DoSimpleDict(r"c:\kjk\noah\dicts\devils_noah.txt", r"Devil dict", r"devil.pdb")

def DoAllSimpleDicts():
    for simpleDictDef in simpleDicts:
        DoSimpleDict(simpleDictDef[0], simpleDictDef[1], simpleDictDef[2])

def _isProfile():
    return len(sys.argv)>1 and sys.argv[1] in ['prof', 'profile']

def _isDumpProfile():
    return len(sys.argv)>1 and sys.argv[1] in ['dump_prof', 'dump_profile', "dumpprof", "dumpprofile"]

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
        testWordCompressor()
        #testCompress()
    elif _isDumpProfile():
        dumpProfileStats()
    else:
        #testPickle()
        #testTiming()
        #main()
        DoDevilDict()

