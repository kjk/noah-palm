# dump the Noah's dictionary pdb format

# Author: Krzysztof Kowalczyk
# History:
#   2003-06-06 Started

# Todo:
#  - everything

import palmdb

NoahProCreator = "NoAH"
WnProType = "wn20"
WnProDemoType = "wnde"
WnLiteType = "wnet"
SimpleType = "simp"

def _dumpWnProData(db):
    pass

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

