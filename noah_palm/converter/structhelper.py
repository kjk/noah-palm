# helper functions for using struct module
# The basic idea: we define a meta-data describing packed data
# in an array as:
# metaData = [ "name1", "fmt1", "name2", "fmt2" ...] where name1,name2...
# are the names of the packed data and "fmt" a format of it as understood
# by module struct (e.g. "H", "L", "32s" etc.)
# based on this metadata we can extract from packed data a dict whose keys
# are names and values are extracted values

# Author: Krzysztof Kowalczyk
from __future__ import generators
import struct,string

def iterlist(list,skip=0,step=1):
    """Iterate over a list skipping first skip values, return only n-th (step)
    value. iterlist(list,skipN=0,step=1) is equivalent to the list"""
    assert step >= 1
    toSkip = 0
    for el in list[skip:]:
        if toSkip > 0:
            toSkip -= 1
        else:
            yield el
            toSkip = step-1

def _testIterList():
    l = [0,1,2,3,4,5,6]
    for el in iterlist(l):
        print el
    for el in iterlist(l,step=2):
        print el
    for el in iterlist(l,skip=1,step=2):
        print el

def _testJoin():
    l = ["a", "b", "c", "d"]
    import string
    str = string.join( iterlist(l,step=2), "" )
    print str

def GetFmtFromMetadata(dataDef, isBigEndian):
    fmt = string.join(iterlist(dataDef,skip=1,step=2), "")
    if isBigEndian:
        fmt = ">" + fmt
    return fmt

def ExtractDataUsingFmt(dataDef,packedData,fmt):
    unpackedData = struct.unpack(fmt,packedData)
    retDict = {}
    for (name,val) in zip(iterlist(dataDef,step=2),unpackedData):
        retDict[name] = val
    return retDict

def ExtractData(dataDef, packedData, isBigEndian=False):
    """Given definition of packed data in format ['name', 'fmt', ...]
    return a dict that maps names to values extracted from packedData.
    isBigEndian says if numerics in packedData are packed in
    big endian(motorola)/network order (false by default because Intel
    uses little endian"""
    fmt = GetFmtFromMetadata(dataDef, isBigEndian)
    return ExtractDataUsingFmt(dataDef,packedData,fmt)

if __name__=="__main__":
    _testJoin()
