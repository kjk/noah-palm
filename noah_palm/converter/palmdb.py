# Reading and writing of Palm's pdb and prc files

# Author: Krzysztof Kowalczyk krzysztofk@pobox.com
# History:
# 2003-06-04 Started

import string,sys,os,struct

class PalmDBRecord:
    def __init__(self,db):
        self.db = db  # link back to the Palm database to which we belong

    def getSize(self):
        raise ValueError("NYI")

class PalmDB:
    def __init__(self,filename=None):
        self.filename = filename
        if filename==None: return
        self.records = []
        self._readHeader()
        assert 0==self.seedId
        assert 0==self.nextRecordList

    def _readHeader(self):
        """Read the header of pdb/prc file and init class based on this data."""
        # The structure of the header:
        # offset, len, what
        #  0, 32, zero-terminated name of the database, max 31 chars in len
        # 32,  2, attributes
        # 34,  2, version
        # 36,  4, creation-date
        # 40,  4, modification-date
        # 44,  4, last-backup-date
        # 48,  4, modification-number
        # 52,  4, app-info-area
        # 56,  4, sort-info-area
        # 60,  4, db-type, 4-byte which is usually some sort of string
        # 64,  4, creator-id
        # 68,  4, seed-id, always 0 ?
        # 72,  4, next-record-list, always 0 ?
        # 76,  2, records-count, number of records in the database file
        fo = open(self.filename,"r")
        headerData = fo.read(78)
        fo.close()
        (self.name, self.attr, self.version, self.creationdate, self.modificationDate,
          self.lastBackupDate, self.modificationNum,self.appInfoArea,self.sortInfoArea,
          self.dbType, self.creatorId, self.seedId, self.nextRecordList,self.recordsCount) = struct.unpack("32sHHLLLLLL4s4sLLH", headerData)
        self.name = self.name.strip("\x00")
        
    def createNew(self,filename):
        if None == self.filename:
            raise ValueError("This is already ")

    def getName(self):
        return self.name

    def getRecords(self):
        return self.records

    def _getRecordsCount(self):
        return self.recordsCount