import sys
sys.path.append("..")
sys.path.append(".")

import unittest
import wn2pdb

class Wn2PdbTest(unittest.TestCase):

    def test_isEmptyString(self):
        emptyStrings = [ "", '', "\n", '  ', "\t  \n", '\r\t\n' ]
        nonEmptyStrings = [" a ", 'a    ', '   t']
        for s in emptyStrings:
            self.assertEqual( True, wn2pdb._isEmptyString(s) )
        for s in nonEmptyStrings:
            self.assertEqual( False, wn2pdb._isEmptyString(s) )

    def test_isPos(self):
        validPos = ["n.", "adj.", "v.t.", "v.i.", "adv.", "pp."]
        invalidPos = ["n", "adj. ", "vt."]
        for s in validPos:
            self.assertEqual( True, wn2pdb._isPos(s) )
        for s in invalidPos:
            self.assertEqual( False, wn2pdb._isPos(s) )

    def test_stateNoToName(self):
        self.assertEqual(wn2pdb.ssNamesDict[wn2pdb.SS_NONE], "SS_NONE")
        self.assertEqual(wn2pdb.ssNamesDict[wn2pdb.SS_HAS_LEMMA], "SS_HAS_LEMMA")
        self.assertEqual(wn2pdb.ssNamesDict[wn2pdb.SS_READING_DEF], "SS_READING_DEF")

    def test_commonPrefixLen(self):
        data = [ ['','',0], ['a','',0], ['','b',0], ['a','b',0], ['a','a',1],
                 ['ab','a',1], ['c','cd',1], ['google','google',6],
                 ['google','googleme',6] ]
        for t in data:
            self.assertEqual(wn2pdb._commonPrefixLen(t[0],t[1]),t[2])
            self.assertEqual(wn2pdb._commonPrefixLen2(t[0],t[1]),t[2])

    def test_compressedWordsGen(self):
        # Undone: test _PREFIX_LEN_LIMIT
        words =           [ "a", "abcd",    "abru",  "abru", "beh", "", "beh", "bral"]
        compressedWords = [ "a", "\x01bcd", "\x02ru","\x04", "beh", "", "beh", "\x01ral" ]
        myCompressedWords = wn2pdb.CompressedWordsGen(words)
        self.assertEqual( words, words )
        for x,y in zip(compressedWords, myCompressedWords):
            self.assertEqual(x,y)
        # now test WordCompressor class
        wc = wn2pdb.WordCompressor()
        for w,wCompressed in zip(words,compressedWords):
            self.assertEqual(wc.compressWord(w),wCompressed)

if __name__ == '__main__':
    unittest.main()

