# dump the Noah's dictionary pdb format

# Author: Krzysztof Kowalczyk
# krzyszotfk@pobox.com

from __future__ import generators
import string,struct
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
    firstRec = structhelper.ExtractDataUsingFmt(simpleFirstRecDef,data,fmt)
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
    firstRec = structhelper.ExtractDataUsingFmt(wnProFirstRecDef,firstRecHdr,fmt)
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
        
def _dumpWords(recData,packStrings):
    """Given a data of a record with compressed words and pack strings
    used to compress the words, dump the words"""
    for w in _iterWords(recData,packStrings):
        print w


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

def _dumpSimpleData(db):
    print "Record 0:"
    firstRecData = db.records[0].data
    firstRec = _extractSimpleFirstRecordData(firstRecData)
    for name in structhelper.iterlist(simpleFirstRecDef,step=2):
        print "%s: %d" % (name, firstRec[name])
    print "Record 1: record with word cache"
    _dumpWordCacheRecord(db.records[1].data)
    print "wordsPackStrings"
    _dumpRecWithPackStrings(db.records[2].data,True)
    print "defsPackStrings"
    _dumpRecWithPackStrings(db.records[3].data,True)
    packedStrings = _extractPackStrings(db.records[2].data)
    _dumpWords(db.records[4].data,packedStrings)

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
    #dumpNoahPdb( _fileMediumOld )
    dumpNoahPdb( _fileDevilOrig )

