# dump the Noah's dictionary pdb format

# Author: Krzysztof Kowalczyk# krzyszotfk@pobox.com

from __future__ import generators
import string,struct,sys
import palmdb,structhelper

NoahProCreator = "NoAH"
WnProType = "wn20"
WnProDemoType = "wnde"
WnLiteType = "wnet"
SimpleType = "simp"

#format of the first record of the wn pro dictionary data
#typedef struct
#{
#    long    wordsCount;
#    long    synsetsCount;
#    int     wordsRecsCount;
#    int     synsetsInfoRecsCount;
#    int     wordsNumRecsCount;
#    int     defsLenRecsCount;
#    int     defsRecsCount;
#    int     maxWordLen;
#    int     maxDefLen;
#    int     maxComprDefLen;
#    int     bytesPerWordNum;
#    int     maxWordsPerSynset;
# } WnFirstRecord;
wnProFirstRecDef = ["wordsCount", "L",
               "synsetsCount", "L",
               "wordsRecsCount", "H",
               "synsetsInfoRecsCount", "H",
               "wordsNumRecsCount", "H",
               "defsLenRecsCount", "H",
               "defsRecsCount", "H",
               "maxWordLen", "H",
               "maxDefLen", "H",
               "maxComprDefLen", "H",
               "bytesPerWordNum", "H",
               "maxWordsPerSynset", "H"]

# format of the pdb with simple dictionary data
#  0  4 long wordsCount
#  4  2 int  wordsRecsCount   - number of records with compressed words
#  6  2 int  defsLenRecsCount - number of records with lengths of definitions
#  8  2 int  defsRecsCount    - number of records with definitions
# 10  2 int  maxWordLen
# 12  2 int  maxDefLen
# 14  2 int  hasPosInfoP
# 16  2 int  posRecsCount
simpleFirstRecDef = ["wordsCount", "L",
                     "wordsRecsCount", "H",
                     "defsLenRecsCount", "H",
                     "defsRecsCount", "H",
                     "maxWordLen", "H",
                     "maxDefLen", "H",
                     "hasPosInfoP", "H",
                     "posRecsCount", "H" ]

def _extractSimpleFirstRecordData(data):
    # extract data from the first record of simple dictionary data
    # Return a dict with name=>value mappings
    fmt = structhelper.GetFmtFromMetadata(simpleFirstRecDef)
    assert 18 == struct.calcsize(fmt)
    assert len(data) >= struct.calcsize(fmt)
    firstRec = structhelper.UnpackDataUsingFmt(simpleFirstRecDef,data,fmt)
    return firstRec

def _extractWnProFirstRecordData(data):
    # extract data from the first record of wn pro dictionary data
    # Return a dict with name=>value mappings
    fmt = structhelper.GetFmtFromMetadata(wnProFirstRecDef)
    assert 28 == struct.calcsize(fmt)
    assert len(data) >= struct.calcsize(fmt)
    SYN_CACHE_REC_SIZE = 8
    if len(data) > 28:
        firstRecHdr = data[0:28]
        synsetCachePacked = data[28:]
        assert (len(synsetCachePacked) % SYN_CACHE_REC_SIZE) == 0
    else:
        firstRecHdr = data
        synsetCachePacked = None
    firstRec = structhelper.UnpackDataUsingFmt(wnProFirstRecDef,firstRecHdr,fmt)
    synCache = []
    if synsetCachePacked:
        for i in range(len(synsetCachePacked)/SYN_CACHE_REC_SIZE):
            packedEntry = synsetCachePacked[SYN_CACHE_REC_SIZE*i:SYN_CACHE_REC_SIZE*(i+1)]
            unpackedTuple = struct.unpack(">LL",packedEntry)
            synCache.append(unpackedTuple)
    return (firstRec,synCache)

def _extractBigEndianInt(data):
    i = struct.unpack(">H",data[0:2])
    return (i[0],data[2:])

# There are 256 pack strings i.e. they map a byte to a string (possibly one-char string)
# This data is encoded in one record. This proc decoded this data and returns as 256-element
# array where array elements are strings
def _extractPackStrings(data):
    (maxComprStrLen,data) = _extractBigEndianInt(data)
    print "maxComprStrLen=%d" % maxComprStrLen
    stringsByLen = []
    total = 0
    for tmp in range(maxComprStrLen):
        (count,data) = _extractBigEndianInt(data)
        stringsByLen.append(count)
        total += count
    assert total == 256
    packStrings = []
    strLen = 1
    for count in stringsByLen:
        for n in range(count):
            str = data[:strLen]
            data = data[strLen:]
            packStrings.append(str)
        strLen += 1
    return packStrings

def _dumpPackStrings(packStrings,condensed):
    str = ""
    for (s,pos) in zip(packStrings,range(len(packStrings))):
        if len(s)==1 and ord(s[0])<=ord(' '):
            str = str + "%d=%d" % (pos,ord(s[0]))
        else:
            str = str + "%d='%s'" % (pos,s)
        if condensed:
            str += ", "
        else:
            str += "\n"
    print str

def unpackData(data,packStrings):
    l = []
    for c in data:
        l.append(packStrings[ord(c)])
    return string.join(l,"")

def _iterPackedChars(recData,packStrings):
    for c in recData:
        txt = packStrings[ord(c)]
        for unpackedChar in txt:
            yield unpackedChar

def _iterWords(recData,packStrings):
    currWord = ""
    prevWord = ""
    fHadWord = False
    for c in _iterPackedChars(recData,packStrings):
        # unpack a word
        inCommon = ord(c)
        if inCommon < 32:
            # this means a new word
            if fHadWord:
                yield currWord
                prevWord = currWord
            else:
                fHadWord = True
            if inCommon == 0:
                currWord = ""
            else:
                currWord = prevWord[0:inCommon]
        else:
            # didn't have anything in common with previous word
            # so we start it right away
            currWord += c
        
def _dumpRecWithPackStrings(data,condensed=False):
    packStrings = _extractPackStrings(data)
    _dumpPackStrings(packStrings,condensed)

def _dumpWnProData(db):
    firstRecData = db.records[0].data
    (firstRec,synCache) = _extractWnProFirstRecordData(firstRecData)
    for name in structhelper.iterlist(wnProFirstRecDef,step=2):
        print "%s: %d" % (name, firstRec[name])
    for s in synCache:
        print "synsetNo=%d, wordsCount=%d" % s
    print "wordsPackStrings"
    _dumpRecWithPackStrings(db.records[2].data)
    print "defsPackStrings"
    _dumpRecWithPackStrings(db.records[3].data)

def _dumpWnLiteData(db):
    pass

def _dumpWordCacheRecord(recData):
    size = len(recData)
    assert size % 8 == 0
    cacheEntries = size / 8
    for i in range(cacheEntries):
        entryData = recData[i*8:(i+1)*8]
        (wordNo,recNo,recOffset) = struct.unpack(">LHH",entryData)
        print "  word=%d,recNo=%d,recOffset=%d" % (wordNo,recNo,recOffset)

class SimpleDictionaryData:
    def __init__(self,db):
        self.db = db
        self.firstRec = None
        self.wordsPackStrings = None
        self.defsPackStrings = None
        self.wordsList = None
        self.defsDataList = None
        self.posData = None
    
    def getWordCacheRecord(self):
        return self.db.records[1].data

    def fHasPosData(self):
        fHasPos = self.getAttr("hasPosInfoP")
        return fHasPos

    def getPosData(self):
        """Return continuous pos (part-of-speech) data (i.e. if originally it's in
        more than one record, return concatenated data from all records"""
        if self.posData == None and self.fHasPosData():
            firstPosRec = 4 + self.getAttr("wordsRecsCount") + self.getAttr("defsLenRecsCount") + self.getAttr("defsRecsCount")
            data = ""
            for n in range(self.getAttr("posRecsCount")):
                data += self.db.records[firstPosRec+n].data
            assert len(data)>0
            self.posData = data
        return self.posData

    def getPosByWord(self,word):
        wordNo = self._getWordNo(word)
        pos = wordNo / 4
        posData = self.getPosData()
        b = ord(posData[pos])
        rest = wordNo % 4
        b = b >> 2*rest
        b = b & 3
        pos = ["noun", "verb", "adj.", "adv."][b]
        return pos

    def getWordsPackStrings(self):
        if self.wordsPackStrings == None:
            self.wordsPackStrings = _extractPackStrings(self.db.records[2].data)
        return self.wordsPackStrings

    def getDefsPackStrings(self):
        if self.defsPackStrings == None:
            self.defsPackStrings = _extractPackStrings(self.db.records[3].data)
        return self.defsPackStrings

    def getAttr(self,var):
        if self.firstRec == None:
            self.firstRec = _extractSimpleFirstRecordData(self.db.records[0].data)
        return self.firstRec[var]

    def getWordsRecsData(self):
        l = []
        for i in range(self.getAttr("wordsRecsCount")):
            l.append(self.db.records[4+i].data)
        return l

    def _constructWordsList(self):
        if self.wordsList == None:
            wordsList = []
            for rec in self.getWordsRecsData():
                for w in _iterWords(rec,self.getWordsPackStrings()):
                    wordsList.append(w)
            self.wordsList = wordsList
        return self.wordsList

    def _getRecsWithDefLens(self):
        recsList = []
        firstRecWithDefLens = 4 + self.getAttr("wordsRecsCount")
        recsWithDefLens = self.getAttr("defsLenRecsCount")
        for i in range(recsWithDefLens):
            recsList.append( self.db.records[firstRecWithDefLens+i].data )
        return recsList

    def _constructDefsDataList(self):
        if self.defsDataList == None:
            defsDataList = []
            recsWithDefLens = self._getRecsWithDefLens()
            currOffset = 0
            currLen = 0
            needMore = 0
            for recData in recsWithDefLens:
                for c in recData:
                    if needMore == 2:
                        needMore = 1
                        currLen = ord(c)
                    elif needMore == 1:
                        needMore = 0
                        currLen = currLen * 256 + ord(c)
                    else:
                        currLen = ord(c)
                        if currLen == 255:
                            needMore = 2
                    if needMore == 0:
                        defData = (currOffset,currLen)
                        defsDataList.append(defData)
                        currOffset += currLen
            self.defsDataList = defsDataList
        return self.defsDataList

    def _convertOffsetToRecOffset(self,offset):
        """Given a total offset of the defintion, convert it to an offset within a record
        i.e. retun record number and offset in the record"""
        rec = 4 + self.getAttr("wordsRecsCount") + self.getAttr("defsLenRecsCount")
        while offset >= len(self.db.records[rec].data):
            offset -= len(self.db.records[rec].data)
            rec += 1
        return (rec,offset)

    def getWordsList(self):
        self._constructWordsList()
        return self.wordsList

    def _getWordNo(self,word):
        wordsList = self._constructWordsList()
        n = 0
        for w in wordsList:
            if w == word:
                return n
            n += 1
        assert 0, "Word %s has not been found" % word

    def getDefByWord(self,word):
        no = self._getWordNo(word)
        return self.getDef(no)

    def getDefPosition(self,no):
        defsDataList = self._constructDefsDataList()
        data = defsDataList[no]
        offset = data[0]
        l = data[1]
        (rec,offsetInRec) = self._convertOffsetToRecOffset(offset)
        return (rec,offsetInRec,l)

    def getDefLenOffsetByWord(self,word):
        no = self._getWordNo(word)
        defsDataList = self._constructDefsDataList()
        data = defsDataList[no]
        offset = data[0]
        l = data[1]
        return (offset,l)

    def getDef(self,no):
        (rec,offsetInRec,l) = self.getDefPosition(no)
        data = self.db.records[rec].data
        compressedDef = data[offsetInRec:offsetInRec+l]
        uncompressedDef = unpackData(compressedDef,self.getDefsPackStrings())
        return uncompressedDef

def _dumpSimpleData(db):
    fPrintWords = True
    fPrintDefs = False
    toDump = 999999 # -1 means infinity i.e all of them
    print "Record 0:"
    simpleDictData = SimpleDictionaryData(db)
    for name in structhelper.iterlist(simpleFirstRecDef,step=2):
        print "%s: %d" % (name, simpleDictData.getAttr(name))
    print "Record 1: record with word cache"
    _dumpWordCacheRecord(simpleDictData.getWordCacheRecord())
    print "wordsPackStrings"
    _dumpPackStrings(simpleDictData.getWordsPackStrings(),True)
    print "defsPackStrings"
    _dumpPackStrings(simpleDictData.getDefsPackStrings(),True)
    packedStrings = simpleDictData.getWordsPackStrings()
    wordsRecsCount = simpleDictData.getAttr("wordsRecsCount")
    wordNo = 0
    print "wordsRecsCount: %d" % wordsRecsCount
    for word in simpleDictData.getWordsList():
        wordNo += 1
        (offset,l) = simpleDictData.getDefLenOffsetByWord(word)
        if simpleDictData.fHasPosData():
            pos = simpleDictData.getPosByWord(word)
            if fPrintWords and toDump>0:
                #print "%s, %s (%d,%d)" % (word,pos,offset,l)
                print "! (%d) %s (%s)" % (wordNo,word,pos)
        else:
            if fPrintWords and toDump>0:
                print "! %s" % (word)
                #print "%s, (%d,%d)" % (word,offset,l)
        if toDump>0:
            toDump -= 1
        d = simpleDictData.getDefByWord(word)
        if fPrintDefs:
            print d

def _dumpGeneric(db):
    print '%d records in "%s"' % (len(db.records), db.name)
    if db.creator != NoahProCreator:
        print "This is not database for Noah Pro (creator(%s) is not %s)" % (db.creator,NoahProCreator)
        return
    print "creator/type: %s/%s" % (db.creator,db.dbType)
    recNo = 0
    for rec in db.records:
        print "Rec %2d, size: %4d" % (recNo,rec.size)
        recNo += 1

    if db.dbType == SimpleType:
        _dumpSimpleData(db)
    elif db.dbType == WnLiteType:
        _dumpWnLiteData(db)
    elif db.dbType == WnProType:
        _dumpWnProData(db)
    else:
        print "Unknown dbType ('%s'), must be 'wn20' or 'wnet' or 'simp'" % db.dbType
        

def dumpNoahPdb(fileName):
    print "Dumping file %s" % fileName
    db = palmdb.PDB(fileName)
    _dumpGeneric(db)

_fileMediumNew = "c:\\kjk\\src\\mine\\noah_dicts\\pdb\\new_converts\\wn_medium.pdb"
_fileMediumOld = "c:\\kjk\\src\\mine\\noah_dicts\\pdb\\originals\\wn_medium.pdb"
_fileDevilOrig = r"C:\kjk\src\mine\dict_data\pdb\originals\devil.pdb"

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print "Usage: noahpdbdump filename.pdb"
    else:
        fileName = sys.argv[1]
        dumpNoahPdb( fileName )
        #dumpNoahPdb( _fileMediumOld )
        #dumpNoahPdb( _fileDevilOrig )
