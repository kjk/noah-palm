# dump the Noah's dictionary pdb format

# Author: Krzysztof Kowalczyk

#from __future__ import generators
import palmdb,struct
import structhelper

NoahProCreator = "NoAH"
WnProType = "wn20"
WnProDemoType = "wnde"
WnLiteType = "wnet"
SimpleType = "simp"

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

def _extractWnProFirstRecordData(data):
    # extract data from the first record. It's defined by the following
    # C struct. Return a dict with name=>value mappings
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
    fmt = structhelper.GetFmtFromMetadata(wnProFirstRecDef,True)
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

def _dumpWnProData(db):
    firstRecData = db.records[0].data
    (firstRec,synCache) = _extractWnProFirstRecordData(firstRecData)
    for name in structhelper.iterlist(wnProFirstRecDef,step=2):
        print "%s: %d" % (name, firstRec[name])
    for s in synCache:
        print "synsetNo=%d, wordsCount=%d" % s

def _dumpWnLiteData(db):
    pass

def _dumpSimpleData(db):
    pass

def _dumpNoahPdb(fileName):
    print "Dumping file %s" % fileName
    db = palmdb.PDB(fileName)
    print "Name: %s" % db.name
    print "Number of records: %d" % len(db.records)
    if db.creator != NoahProCreator:
        print "This is not database for Noah Pro (creator(%s) is not %s)" % (db.creator,NoahProCreator)
        return
    print "Creator: %s" % db.creator
    print "DbType: %s" % db.dbType
    recNo = 0
    for rec in db.records:
        print "Rec %2d, size: %4d" % (recNo,rec.size)
        recNo += 1
    if db.dbType == "wn20":
        _dumpWnProData(db)
    elif db.dbType == "wnet":
        _dumpWnLiteData(db)
    elif db.dbType == "simp":
        _dumpSimpleData(db)
    else:
        print "Unknown dbType ('%s'), must be 'wn20' or 'wnet' or 'simp'" % db.dbType

_fileMediumNew = "c:\\kjk\\src\\mine\\noah_dicts\\pdb\\new_converts\\wn_medium.pdb"
_fileMediumOld = "c:\\kjk\\src\\mine\\noah_dicts\\pdb\\originals\\wn_medium.pdb"

if __name__ == '__main__':
    _dumpNoahPdb( _fileMediumOld )

