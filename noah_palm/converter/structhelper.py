# helper functions for using struct module
# The basic idea: we define a meta-data describing packed data
# in an array as:
# metaData = [ "name1", "fmt1", "name2", "fmt2" ...] where name1,name2...
# are the names of the packed data and "fmt" a format of it as understood
# by module struct (e.g. "H", "L", "32s" etc.)
# based on this metadata we can extract from packed data a dict whose keys
# are names and values are extracted values

# Author: Krzysztof Kowalczyk
import struct

def GetFmtFromMetadata(dataDef, isBigEndian):
    if isBigEndian:
        fmt = ">"
    else:
        fmt = ""
    isFormat = False
    for d in dataDef:
        if isFormat == True:
            fmt += d
            isFormat = False
        else:
            isFormat = True
    return fmt

def ExtractDataUsingFmt(dataDef,packedData,fmt):
    unpackedData = struct.unpack(fmt,packedData)
    retDict = {}
    n = 0
    isFormat = False
    for name in dataDef:
        if isFormat:
            isFormat = False
        else:
            retDict[name] = unpackedData[n]
            n += 1
            isFormat = True
    return retDict

def ExtractData(dataDef, packedData, isBigEndian=False):
    """Given definition of packed data in format ['name', 'fmt', ...]
    return a dict that maps names to values extracted from packedData.
    isBigEndian says if numerics in packedData are packed in
    big endian(motorola)/network order (false by default because Intel
    uses little endian"""
    fmt = GetFmtFromMetadata(dataDef, isBigEndian)
    return ExtractDataUsingFmt(dataDef,packedData,fmt)

