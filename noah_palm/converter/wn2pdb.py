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

posTxtToNum = { "n.":    0, 
                "n":     0, 
                "adj.":  1,
                "v.t.":  1,
                "v.i.":  1,
                "vi":    1,
                "v.":    1,
                "v":     1,
                "vt":    1,
                "adj.":  2,
                "adj":   2,
                "adv.":  3,
                "adv":   3,
                "pp.":   3,
                "n or adj" : 0,
                "exp": 0,
                "excl":    0,
                "acr":    0,
                "exc":    0,
                "abb":    0,
                "ab":    0,
                "gree":    0,
                "number":  0,
                "number.":  0,
                "syn": 0,
                "int": 0,
                "exclamation" : 0,
                "suffix":  0

 }

def _isPos(str):
    """Return True if this is a valid part of speech"""
    return posTxtToNum.has_key( string.lower(str) )


def _numericPos(posTxt):
    key = string.lower(posTxt)
    if posTxtToNum.has_key( key ):
        return posTxtToNum[key]
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
            #UNDONE: not sure which one is correct
            return chr(0) + word
            #return word
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

def _dumpTable(table):
    keys = table.keys()
    keys.sort()
    for t in keys:
        v = table[t]
        if t.isalnum():
            s = "%3d (%c) = " % (ord(t),t)
        else:
            s = "%3d (.) = " % ord(t)
        sys.stdout.write(s)
        print v

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
        #print packStrings
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
                break
        # the whole string matched
        if lastValidCode == -1:
            print "Couldn't compress string '%s' (char %d,%s)" % (str,ord(c),c)
            print self.mainTable
            if self.mainTable.has_key(c):
                print "self.mainTable has key %s" % c
                print self.mainTable[c]
                if currTable == self.mainTable:
                    print "currTable is self.mainTable"
                else:
                    print "currTable is not self.mainTable"
                    print currTable
            else:
                print "self.mainTable doesn't have a key %s" % c
            #print "table:"
            #_dumpTable(self.mainTable)
            #print self.mainTable
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
    m = l[0][0] # min value
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


def freq_el_make(i,j,freq):
    return (i,j,freq)

def freq_el_freq(freq):
    return freq[2]

def freq_el_j(freq):
    return freq[1]

def freq_el_i(freq):
    return freq[0]

def _sortFreqFunc(f1,f2):
    if freq_el_freq(f1) > freq_el_freq(f2):
        return 1
    elif freq_el_freq(f1) < freq_el_freq(f2):
        return -1
    else:
        return 0

def freq_el_conflicts_p(f1,f2):
    if freq_el_i(f1) == freq_el_j(f2) or freq_el_j(f1) == freq_el_i(f2):
        return True
    else:
        return False

def freq_conflicts_p(freq_list,freq_el):
    for f_el in freq_list:
        if freq_el_conflicts_p(f_el,freq_el):
            return True
    return False

class Freq:
    def __init__(self,dx=256,dy=256):
        self.dx = dx
        self.dy = dy
        self.freq = [[0 for y in range(self.dy)] for x in range(self.dx)]
        self.freqFlat = [0 for x in range(max(self.dx,self.dy))]

    def clearFreq(self):
        for x in range(self.dx):
            for y in range(self.dy):
                self.freq[x][y] = 0
        for x in range(len(self.freqFlat)):
            self.freqFlat[x] = 0

    def addStr(self,s):
        x = ord(s[0])
        for c in s[1:]:
            self.freqFlat[x] += 1
            y = ord(c)
            self.freq[x][y] += 1
            x = y
        self.freqFlat[x] += 1

    def val(self,x,y):
        assert (x>=0) and (x<self.dx)
        assert (y>=0) and (y<self.dy)
        return self.freq[x][y]

    def buildFreqForStrings(self,strList):
        self.clearFreq()
        for s in strList:
            self.addStr(s)

    def getMaxFreq(self,maxElements):
        # gather all frequencies in a list
        l = []
        for x in range(self.dx):
            for y in range(self.dy):
                freq = self.freq[x][y]
                if freq != 0:
                    l.append(freq_el_make(x,y,freq))
        l.sort(_sortFreqFunc)
        res = []
        for f_el in l:
            if not freq_conflicts_p(res,f_el):
                res.append(f_el)
                if len(res) >= maxElements:
                    break
        res.reverse()
        return res

    def getMostFrequent(self,n):
        """find n most frequent strings from freqTable and return as a list
        whose elements are [frequency, string]"""
        result = []
        currThresh = 1 # UNDONE: was -1, why?
        for x in range(self.dx):
            for y in range(self.dy):
                freq = self.freq[x][y]
                if freq >= currThresh:
                    # need to insert or replace element from the list of most
                    # frequently used elements
                    if len(result) < n:
                        # add to the list since we don't have n elements yet
                        result.append( [freq, chr(x)+chr(y)] )
                        if len(result) == n:
                            # when we finally found all elements, we need to
                            # adjust threshold from permitting anything to the
                            # real threshold
                            currThresh = _getSmallestFreq(result)
                    else:
                        if freq > currThresh: # we don't want to fill full table with existing freq's
                            # need to replace element on the list
                            pos = _getPosOfThisFreq(result,currThresh)
                            result[pos] = [freq, chr(x)+chr(y)]
                            currThresh = _getSmallestFreq(result)
        return result

    def getFreqsAsList(self,maxResults):
        """gather frequencies in a list whose elements are (x,y,frequency)
        and are sorted by frequency and conflicting frequencies are removed"""
        l = []
        for x in range(self.dx):
            for y in range(self.dy):
                freq = self.freq[x][y]
                if freq != 0:
                    l.append(freq_el_make(x,y,freq))
        l.sort(_sortFreqFunc)
        res = []
        for freqEl in l:
            if not freq_conflicts_p(res,freqEl):
                res.append(freqEl)
                if len(res) >= maxResults:
                    break
        res.reverse()
        return res

    def getAllValues(self):
        """Return a list of all encountered values, sorted by their value"""
        result = []
        for pos in range(len(self.freqFlat)):
            if self.freqFlat[pos] > 0:
                result.append(pos)
        #print result
        return result

class CompInfoGen:
    """Given a list of strings, generate compression info (packStrings).
    Compression is slightly worse than in the original Lisp implementation."""
    def __init__(self,strList):
        self.strList = strList
        self.table = None
        self.reservedCodes = 0
        self.FREQ_IDX = 0
        self.NEXT_TABLE_IDX = 1
        self.FULL_STR_IDX = 2
        self.STR_MAX_LEN = 5

    def reserveCodes(self,codes):
        self.reservedCodes = codes

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

    def _buildCompressionInfo(self):
        assert self.table == None
        self.table = {}
        for s in self.strList:
            self.incStr(s)

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
        self._buildCompressionInfo()
        codesLeft = 256-self.reservedCodes
        packStrings = [chr(n) for n in range(self.reservedCodes)]
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
        return packStrings

class CompInfoNoCompression:
    def __init__(self,strList):
        pass
    def reserveCodes(self,codes):
        pass
    def buildPackStrings(self):
        packStrings = [chr(c) for c in range(256)]
        return packStrings
    
class CompInfoGenWeak:
    """Given a list of strings, generate compression info (packStrings).
    It works and is simple but compression is worse than in CompInfoGen"""
    def __init__(self,strList):
        self.freqTable = Freq(256,256)
        self.strList = strList
        self.reservedCodes = 32

    def reserveCodes(self,codes):
        self.reservedCodes = codes

    def buildPackStrings(self):
        self.freqTable.buildFreqForStrings(self.strList)
        packStrings = [chr(n) for n in range(self.reservedCodes)]
        allValues = self.freqTable.getAllValues()
        for v in allValues:
            if v >= self.reservedCodes:
                assert packStrings.count(chr(v)) == 0
                packStrings.append(chr(v))
        codesLeft = 256-len(packStrings)
        flist = self.freqTable.getMostFrequent(codesLeft)
        for el in flist:
            packStrings.append( el[1] )
        assert 256 == len(packStrings)
        return packStrings

def calcNewCodesCount(usedCodes):
    assert usedCodes <= 256
    codesToMake = 256-usedCodes
    formula = ((200,50),(120,40),(60,25),(30,10),(-1,18))
    for el in formula:
        if codesToMake > el[0]:
            #print "calcNewCodesCount: codesToMake=%d, el[1]=%d" % (codesToMake, el[1])
            return min(codesToMake,el[1])
    assert 0

def printFreqList(l):
    for el in l:
        x = freq_el_i(el)
        y = freq_el_j(el)
        freq = freq_el_freq(el)
        txt = chr(x) + chr(y)
        print "%s %d (%d,%d)" % (txt,freq,x,y)

def _getStrFromCodes(codes,s):
    prevLen = len(s)
    while True:
        tmp = ""
        for c in s:
            if codes[ord(c)] == None:
                tmp += c
            else:
                tmp += codes[ord(c)]
        if len(tmp) == prevLen:
            break
        s = tmp
        prevLen = len(s)
    return tmp

def _codeStringByCodeTable(txt,codeTable):
    strOutAsList = [codeTable[ord(c)] for c in txt]
    strOut = string.join(strOutAsList,"")
    return strOut

def _decodeStringByCodeTable(txt,codeTable):
    try:
        strOutAsList = [chr(codeTable.index(c)) for c in txt]
    except ValueError:
        print "txt = %s, c=%s" % (txt,c)
        raise ValueError, "list.index(x): x not in list"
    return string.join(strOutAsList, "")

def _testCodeTable():
    testStrings = ["ab", "rubra", "genoatara", "hello", "this is a test string"]
    codeTable = [None for t in range(256)]
    code = 0
    for s in testStrings:
        for c in s:
            if codeTable[ord(c)] == None:
                codeTable[ord(c)] = chr(code)
                code += 1
    codedStrings = [_codeStringByCodeTable(s,codeTable) for s in testStrings]
    decodedStrings = [_decodeStringByCodeTable(s,codeTable) for s in codedStrings]
    for (orig,decoded) in zip(testStrings,decodedStrings):
        #print "orig: %s" % orig
        #print "deco: %s" % decoded
        assert orig == decoded

class CompInfoGenNew:
    """Given a list of strings, generate compression info (packStrings). This
    is slow but should be the most efficient version."""
    def __init__(self,strList):
        self.strList = strList
        self.reservedCodes = 0

    def reserveCodes(self,codes):
        self.reservedCodes = codes

    def buildPackStrings(self):
        freqTable = Freq(256,256)
        strList = self.strList
        freqTable.buildFreqForStrings(strList)
        packStrings = [chr(n) for n in range(self.reservedCodes)]
        allValues = freqTable.getAllValues()
        for v in allValues:
            if v >= self.reservedCodes:
                assert packStrings.count(chr(v)) == 0
                packStrings.append(chr(v))
        currCode = len(packStrings)

        codeTable = [None for t in range(256)]
        code = 0
        for c in packStrings:
            assert len(c) == 1
            codeTable[ord(c)] = chr(code)
            code += 1
        assert code == currCode
        strList = [_codeStringByCodeTable(s,codeTable) for s in strList]
        freqTable.buildFreqForStrings(strList)
        pairs = []
        sys.stdout.write("found code: ")
        while True:
            flist = freqTable.getMostFrequent(1)
            codeStr = flist[0][1]
            #print "code = %d, code string: %s" % (currCode,_decodeStringByCodeTable(codeStr,codeTable))
            sys.stdout.write( "%d, " % currCode )
            pairs.append( (codeStr,currCode) )
            replaceChar = chr(currCode)
            strList = [s.replace(codeStr,replaceChar) for s in strList]
            currCode += 1
            if currCode == 256:
                break
            assert currCode < 256
            freqTable.buildFreqForStrings(strList)
        codes = [None for t in range(256)]
        sys.stdout.write( "\nstrCode: " )
        for p in pairs:
            strPair = p[0]
            code = p[1]
            codes[code] = strPair
            strCode = _getStrFromCodes(codes,strPair)
            strCode = _decodeStringByCodeTable(strCode,codeTable)
            sys.stdout.write( "'%s', " % strCode)
            packStrings.append(strCode)
        sortStringsByLen(packStrings)
        #print packStrings
        return packStrings

class CompInfoGenOrig:
    """Given a list of strings, generate compression info (packStrings). This
    should be identical to the way original lisp version works"""
    def __init__(self,strList):
        #self.compressedStrList = None
        self.freqTable = Freq(256,256)
        self.charToCode = [0 for i in range(256)]
        self.usedCodes = 1 # zero always maps to zero
        self.oneCharCodes = 0
        self.codeToChar = [[0,0] for i in range(256)]
        self.strToCode = ['' for i in range(256)]
        self.compressionTable = [[0 for i in range(256)] for t in range(256)]
        self.reservedCodes = 0
        self.origStrings = strList

    def reserveCodes(self,codes):
        self.reservedCodes = codes

    def _codeStrings(self):
        self.strList = [self._codeString(s) for s in self.origStrings]

    def _reserveCodes(self):
        for i in range(self.reservedCodes):
            self.charToCode[i] = i
            self.codeToChar[i][0] = i
        self.usedCodes = self.reservedCodes

    def _charToCode(self,c):
        code = self.charToCode[c]
        if (code == 0) and (c != 0):
            code = self.usedCodes
            self.charToCode[c] = code
            self.codeToChar[code][0] = c
            self.usedCodes += 1
        return code

    def _codeString(self,txt):
        strOutAsList = [chr(self._charToCode(ord(c))) for c in txt]
        strOut = string.join(strOutAsList,"")
        return strOut

    def _addPair(self,x,y):
        self.compressionTable[x][y] = self.usedCodes
        if self.usedCodes > 255:
            print "self.usedCodes is %d" % self.usedCodes
        assert self.usedCodes <= 255
        self.codeToChar[self.usedCodes][0] = x
        self.codeToChar[self.usedCodes][1] = y
        self.usedCodes += 1

    def _strFromCodeIter(self,code):
        codesToProcess = [code]
        oneCharCodes = self.oneCharCodes
        while len(codesToProcess) > 0:
            # code = pop(codesToProcess)
            code = codesToProcess[-1]
            codesToProcess = codesToProcess[:-1]
            if code <= oneCharCodes:
                yield chr(self.codeToChar[code][0])
            else:
                # push(codesToProcess,self.codeToChar[code][1])
                codesToProcess.append(self.codeToChar[code][1])
                # push(codesToProcess,self.codeToChar[code][1])
                codesToProcess.append(self.codeToChar[code][0])
    
    def _stringFromCode(self,code):
        strFromCode = [c for c in self._strFromCodeIter(code)]
        s = string.join(strFromCode,"")
        return s

    def _buildPackStrings(self):
        packStrings = [self._stringFromCode(code) for code in range(256)]
        sortStringsByLen(packStrings)
        assert 256 == len(packStrings)
        return packStrings

    def _buildPackDataOnePass(self,codesToUse):
        #print "_buildPackDataOnePass(codesToUse=%d), self.usedCodes=%d" % (codesToUse,self.usedCodes)
        sortedFreq = self.freqTable.getFreqsAsList(codesToUse)
        realNewCodes = len(sortedFreq)
        newList = []
        assert 0 != realNewCodes
        for el in sortedFreq:
            self._addPair(freq_el_i(el),freq_el_j(el))
        for el in self.strList:
            newList.append( self._tmpPackString(el) )
        self.strList = newList
        self._clearCompressionTable()
        if self.usedCodes < 256:
            self.freqTable.buildFreqForStrings(self.strList)

    def _clearCompressionTable(self):
        for x in range(256):
            for y in range(256):
                self.compressionTable[x][y] = 0

    def _tmpPackString(self,s):
        result = []
        if len(s) == 0:
            return ""
        x = s[0]
        s = s[1:]
        while True:
            if len(s) == 0:
                result.append(x)
                break
            y = s[0]
            s = s[1:]
            if self.compressionTable[ord(x)][ord(y)] == 0:
                # no code for this pair, so just append first character
                result.append(x)
                x = y
            else:
                x = chr(self.compressionTable[ord(x)][ord(y)])
                result.append(x)
        resultString = string.join(result,"")
        return resultString

    def _buildPackData(self):
        self.oneCharCodes = self.usedCodes
        self.freqTable.buildFreqForStrings(self.strList)
        while self.usedCodes < 256:
            self._buildPackDataOnePass(calcNewCodesCount(self.usedCodes))
        return self._buildPackStrings()

    def buildPackStrings(self):
        self._reserveCodes()
        self._codeStrings()
        return self._buildPackData()

def buildStringCompressor(strList,codesToReserve=0):
    """Given a list of strings, build compressor object optimal for compressing
    those strings"""
    #ft = CompInfoGenOrig(strList)
    #ft = CompInfoNoCompression(strList)
    #ft = CompInfoGenWeak(strList)
    #ft = CompInfoGen(strList)
    ft = CompInfoGenNew(strList)
    ft.reserveCodes(codesToReserve)
    packStrings = ft.buildPackStrings()
    #print packStrings
    assert 256 == len(packStrings)
    comp = StringCompressor(packStrings)
    return comp

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
    strComp = buildStringCompressor(words,32)
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

def DoSlangDict():
    DoSimpleDict(r"c:\slang_dictionary.txt", r"Slang", r"slang.pdb")

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
        #DoDevilDict()
        DoSlangDict()


