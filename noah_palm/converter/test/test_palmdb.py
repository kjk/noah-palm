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

class PDBTest(unittest.TestCase):
    def test_Header(self):
        self.db = palmdb.PDB( _fileMediumNew )
        self.assertEqual( "WN  medium", self.db.name)
        self.assertEqual(33, len(self.db.records) )
        newName = "Test"
        self.db.name = newName
        self.assertEqual( newName, self.db.name )

if __name__ == '__main__':
    unittest.main()
