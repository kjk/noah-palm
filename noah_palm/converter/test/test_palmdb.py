# Unit testing for palmdb.py

# Author: Krzysztof Kowalczyk
# History:
#   2003-06-04 Started
#   2003-06-06 stuff added

import sys
sys.path.append("..")
sys.path.append(".")

import unittest
import palmdb

_fileMediumNew = "c:\\kjk\\src\\mine\\noah_dicts\\pdb\\new_converts\\wn_medium.pdb"
#_fileMediumNew = self.db = palmdb.PDB("c:\\kjk\\src\\mine\\noah_dbs\\newly_created\\wn_medium.pdb")

# creator and dbType must be exactly 4 bytes in len, test if code
# rejects shorter/longer values
def _SetInvalidCreator1(db):
    db.creator = "blaha"
def _SetInvalidCreator2(db):
    db.creator = "bla"
def _SetInvalidDbType1(db):
    db.dbType = "reheh"
def _SetInvalidDbType2(db):
    db.dbType = "reh"

class PDBTest(unittest.TestCase):
    def test_Header(self):
        self.db = palmdb.PDB(_fileMediumNew)
        db = self.db
        self.assertEqual("WN  medium", db.name)
        self.assertEqual("NoAH", db.creator)
        self.assertEqual(33, len(db.records))
        self.assertEqual("wn20", db.dbType)
        newName = "Test"
        db.name = newName
        self.assertEqual( newName, db.name )
        self.assertRaises(AssertionError, _SetInvalidCreator1, db)
        self.assertRaises(AssertionError, _SetInvalidCreator2, db)
        self.assertRaises(AssertionError, _SetInvalidDbType1, db)
        self.assertRaises(AssertionError, _SetInvalidDbType2, db)
        newCreator = "LeoN"
        db.creator = newCreator
        self.assertEqual( newCreator, db.creator )
        newType = "BreH"
        db.dbType = newType
        self.assertEqual(newType,db.dbType)

    def test_CreateNew(self):
        db = palmdb.PDB()
        db.dbType = "type"
        db.creator = "crea"
        db.name = "kjk test db"
        db.saveAs( "test_0rec.pdb", True )
        recs = db.records
        recs.append( palmdb.PDBRecordFromData("Hello") )
        db.saveAs( "test_1rec.pdb", True )

if __name__ == '__main__':
    unittest.main()
