# Unit testing for pdb.py

# Author: Krzysztof Kowalczyk
# History:
# 2003-06-04 Started

import sys
sys.path.append("..")
sys.path.append(".")

import unittest
import palmdb

class PalmDBTest(unittest.TestCase):
    def test_Header(self):
        self.db = palmdb.PalmDB("c:\\kjk\\src\\mine\\noah_dbs\\newly_created\\wn_medium.pdb")
        self.assertEqual( "WN  medium", self.db.getName())

if __name__ == '__main__':
    unittest.main()
