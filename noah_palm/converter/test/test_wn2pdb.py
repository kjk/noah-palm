import sys,os
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


    def test_calcNewCodesCount(self):
        testData = ((0,50), (256-204,50), (250,6), (256,0))
        for el in testData:
            self.assertEqual(el[1],wn2pdb.calcNewCodesCount(el[0])) 

    def test_stateNoToName(self):
        self.assertEqual(wn2pdb._getStateName(wn2pdb.SS_NONE), "SS_NONE")
        self.assertEqual(wn2pdb._getStateName(wn2pdb.SS_HAS_LEMMA), "SS_HAS_LEMMA")
        self.assertEqual(wn2pdb._getStateName(wn2pdb.SS_READING_DEF), "SS_READING_DEF")

    def test_commonPrefixLen(self):
        data = [ ['','',0], ['a','',0], ['','b',0], ['a','b',0], ['a','a',1],
                 ['ab','a',1], ['c','cd',1], ['google','google',6],
                 ['google','googleme',6] ]
        for t in data:
            self.assertEqual(wn2pdb._commonPrefixLen(t[0],t[1]),t[2])

    def test_compressedWordsGen(self):
        # Undone: test _PREFIX_LEN_LIMIT
        words =           [ "a", "abcd",    "abru",  "abru", "beh", "", "beh", "bral"]
        compressedWords = [ "a", "\x01bcd", "\x02ru","\x04", "beh", "", "beh", "\x01ral" ]
        myCompressedWords = wn2pdb.compressedWordsGen(words)
        self.assertEqual( words, words )
        for word,wordCompressed in zip(compressedWords, myCompressedWords):
            self.assertEqual(word,wordCompressed)
        # now test WordCompressor class
        wc = wn2pdb.WordCompressor()
        for word,wordCompressed in zip(words,compressedWords):
            self.assertEqual(wc.compress(word),wordCompressed)

    def _testCompressionWithCompressor(self,compressor,lines):
        packStrings = compressor.buildPackStrings()
        self.assertEqual(256,len(packStrings))
        comp = wn2pdb.StringCompressor(packStrings)
        uncompressedSize = 0
        compressedSize = 0
        for line in lines:
            uncompressedSize += len(line)
            compressedLine = comp.compress(line)
            compressedSize += len(compressedLine)
            uncompressedLine = comp.uncompress(compressedLine)
            self.assertEqual(line,uncompressedLine)
        return (uncompressedSize, compressedSize)
        
    def test_compression(self):
        # to test compression we'll compress/decompress a large text file
        # i.e. wn2pdb.py script itself
        lines = getLinesFromFile()
        print ""

        comp = wn2pdb.CompInfoGenOrig(lines)
        compressionThree = self._testCompressionWithCompressor(comp, lines)
        print "compressed size for CompInfoGenOrig : %d" % compressionThree[1]

        comp = wn2pdb.CompInfoGen(lines)
        compressionOne = self._testCompressionWithCompressor(comp,lines)
        print "compressed size for CompInfoGen     : %d" % compressionOne[1]

        comp = wn2pdb.CompInfoGenWeak(lines)
        compressionTwo = self._testCompressionWithCompressor(comp,lines)
        print "compressed size for CompInfoGenWeak : %d" % compressionTwo[1]

        assert compressionOne[0] == compressionTwo[0]
        assert compressionTwo[0] == compressionThree[0]

def getLinesFromFile():
    fileName = os.path.join("..", "wn2pdb.py")
    fo = open(fileName, "rb")
    lines = fo.readlines()
    fo.close()
    return lines

if __name__ == '__main__':
    unittest.main()

