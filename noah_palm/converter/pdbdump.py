# dump info about PDB file

# Author: Krzysztof Kowalczyk
# History:
#   2003-06-06 Started

# Todo:
#  - interpreting Noah's dictionary databases

def dumpPDBInfo(fileName):
    db = palmdb.PDB(fileName)
    print "Name: %s" % db.name
    db.name = "test"
    print "New name: %s" % db.name
    print "Number of records: %d" % db.getRecordsCount()
    recNo = 0
    for rec in db.records:
        print "Rec %2d, size: %4d" % (recNo,rec.size)
        recNo += 1

_fileMediumNew = "c:\\kjk\\src\\mine\\noah_dicts\\pdb\\new_converts\\wn_medium.pdb"
if __name__ == '__main__':
    dumpPDBInfo( _fileMediumNew )
