# This scripts updates the database format of Noah Lite database from pre-v1
# version to v1+ version.

# Owner: Krzysztof Kowalczyk (krzysztofk@pobox.com)

# Changes we make:
# - change the name from "Wordnet medium" to "NoahLite medium"
# reorder fields in the first record
# from:
#typedef struct
#{
#    long    wordsCount;
#    long    synsetsCount;
#    int     synsetDefLenRecordsCount;
#    int     wordsInfoRecordsCount;
#    int     wordsRecordsCount;
#    int     synsetDefRecordsCount;
#    int     maxWordLen;
#    int     maxDefLen;
#    int     maxComprDefLen;
#    int     maxSynsetsForWord;
#    long    firstLemmaInWordInfoRec;
#} WnLiteFirstRecord;
#
# to:
#
#typedef struct
#{
#    long    synsetsCount;
#    long    wordsCount;
#
#    int     wordsInfoRecordsCount;
#    int     synsetDefLenRecordsCount;
#
#    int     synsetDefRecordsCount;
#    int     wordsRecordsCount;
#
#    int     maxDefLen;
#    int     maxWordLen;
#
#    int     maxSynsetsForWord;
#    int     maxComprDefLen;
#
#    long    firstLemmaInWordInfoRec;
#} WnLiteFirstRecord;

from __future__ import generators
import string,struct,sys
import palmdb,structhelper,noahpdbdump

srcDb = r"f:\arslexis\releases\NoahLite\0_65\noah_lite\wordnet_medium.pdb"

# we only plan to use medium database
srcExpectedName = "Wordnet medium"
dstName = "NoahLite medium"

dstDb = r"f:\wordnet_medium_new.pdb"

db = palmdb.PDB(srcDb)
if db.name != srcExpectedName:
    print "We can only modify %s database" % srcExpectedName
    sys.exit(0)

db.name = dstName

allRecords = db.records

firstRecDataBlob = allRecords[0].data
(firstRecData,restData) = noahpdbdump._extractWnLiteFirstRecordData(firstRecDataBlob)

print firstRecData

fmt = structhelper.GetFmtFromMetadata(noahpdbdump.wnLiteFirstRecDefPost065)

recZeroBlob = struct.pack( fmt, 
        firstRecData["synsetsCount"],
        firstRecData["wordsCount"],
        firstRecData["wordsInfoRecordsCount"],
        firstRecData["synsetDefLenRecordsCount"],
        firstRecData["synsetDefRecordsCount"],
        firstRecData["wordsRecordsCount"],
        firstRecData["maxDefLen"],
        firstRecData["maxWordLen"],
        firstRecData["maxSynsetsForWord"],
        firstRecData["maxComprDefLen"] )

recZeroBlob = recZeroBlob + restData
allRecords[0] = palmdb.PDBRecordFromData(recZeroBlob)

db.saveAs(dstDb,True)
