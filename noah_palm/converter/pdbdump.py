# dump info about PDB file

# Author: Krzysztof Kowalczyk
# History:
#   2003-06-06 Started

# Todo:
#  - interpreting Noah's dictionary databases

import palmdb

def dumpPDBInfo(fileName):
    print "Dumping file %s" % fileName
    db = palmdb.PDB(fileName)
    print "Name: %s" % db.name
    print "Number of records: %d" % len(db.records)
    recNo = 0
    for rec in db.records:
        print "Rec %2d, size: %4d" % (recNo,rec.size)
        recNo += 1

_fileMediumOld = "c:\\kjk\\src\\mine\\noah_dicts\\pdb\\originals\\wn_medium.pdb"
_fileMediumNew = "c:\\kjk\\src\\mine\\noah_dicts\\pdb\\new_converts\\wn_medium.pdb"
_fileMediumNew2 = "c:\\kjk\\src\\mine\\noah_dicts\\pdb\\new_converts_fast\\wn_medium_f.pdb"
if __name__ == '__main__':
    dumpPDBInfo( _fileMediumOld )
    dumpPDBInfo( _fileMediumNew )
    dumpPDBInfo( _fileMediumNew2 )
