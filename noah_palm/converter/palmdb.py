# Reading and writing of Palm's pdb and prc files
# Pages describing the format of PDB/PRC files:
#   http://www.pda-parade.com/prog/palm/doc/pdb.html
#   http://web.mit.edu/tytso/www/pilot/prc-format.html
#   http://www.palmos.com/dev/support/docs/fileformats/FileFormatsTOC.html
#   http://www.thinkmobile.com/Resource/Top/Palmtops_and_Handhelds/Palm_OS/Programming/File_Formats/
#   http://www.eecs.harvard.edu/~nr/pilot/pdb/

# Author: Krzysztof Kowalczyk krzysztofk@pobox.com
# History:
#   2003-06-04 Started
#   2003-06-05 programming continues
#   2003-06-06 improving classes

import string,sys,os,struct,stat

def _getFileSizeFromFileObject(fo):
    """Return size of a file object."""
    return os.fstat(fo.fileno())[stat.ST_SIZE]

(REC_FLAG_DELETE, REC_FLAG_DIRTY, REC_FLAG_BUSY, REC_FLAG_SECRET) = 0x10, 0x20, 0x40, 0x80

class PDBRecord(object):
    def __init__(self): raise ValueError("cannot instantiate directly")

    def _getSize(self):
        return self._size
    size = property(_getSize)

    def _isDelete(self):
        return ((self.attrs & REC_FLAG_DELETE) == REC_FLAG_DELETE)
    def _setDelete(self,flag):
        if flag:
            self.attrs |= REC_FLAG_DELETE
        else:
            self.attrs &= ~REC_FLAG_DELETE
    isDelete = property(_isDelete,_setDelete)

    def _isDirty(self):
        return ((self.attrs & REC_FLAG_DIRTY) == REC_FLAG_DIRTY)
    def _setDirty(self,flag):
        if flag:
            self.attrs |= REC_FLAG_DIRTY
        else:
            self.attrs &= ~REC_FLAG_DIRTY
    isDirty = property(_isDirty,_setDirty)

    def _isBusy(self):
        return ((self.attrs & REC_FLAG_BUSY) == REC_FLAG_BUSY)
    def _setBusy(self,flag):
        if flag:
            self.attrs |= REC_FLAG_BUSY
        else:
            self.attrs &= ~REC_FLAG_BUSY
    isBusy = property(_isBusy,_setBusy)

    def _isSecret(self):
        return ((self.attrs & REC_FLAG_SECRET) == REC_FLAG_SECRET)
    def _setSecret(self,flag):
        if flag:
            self.attrs |= REC_FLAG_SECRET
        else:
            self.attrs &= ~REC_FLAG_SECRET
    isSecret = property(_isSecret,_setSecret)

    def _getCategory(self):
        return self.attrs & 0x0f
    def _setCategory(self,cat):
        assert cat < 16
        self.attrs |= cat
    category = property(_getCategory,_setCategory)

    def _setData(self,data):
        if data == None:
            self._size = 0
            self._data = None
        else:
            self._size = len(data)
            self._data = data
    def _getData(self):
        raise ValueError("Shouldn't be called")
    def _delData(self):
        self._setData(None)
    data = property(_getData,_setData,_delData)

    def _getAttrs(self):
        return self._attrs
    def _setAttrs(self,attrs):
        assert (attrs & 0xff) == 0
        self._attrs = attrs
    attrs = property(_getAttrs,_setAttrs)

class PDBRecordFromData(PDBRecord):
    def __init__(self,data,attrs=0):
        DBRecord.setData(data)
        self.attrs = attrs

class PDBRecordFromDisk(PDBRecord):
    """PDB record record read from disk.

    Lazily fetches the data from the file."""
    def __init__(self,fileName,attrs,offset,size):
        assert fileName != None
        assert (attrs & 0xff) == 0
        assert offset > 78
        assert size >= 0
        self._fileName = fileName
        self._offset = offset
        self._size = size
        self.attrs = attrs
        self._data = None

    def _getData(self):
        if self._data == None:
            fo = open(self._fileName, "rb")
            fo.seek( self._offset )
            self._data = fo.read( self._size )
            fo.close()
        return self._data
    data = property(_getData,PDBRecord._setData)

class PDB(object):
    def __init__(self,fileName=None):
        self._fileName = fileName
        self._name = None
        self._records = []
        if fileName==None: return
        self._readHeader()

        self._t = 5

    def _raiseInvalidFile(self):
        # TODO: better exception
        raise ValueError( "%s is not a valid PDB file" % self.fileName )

    def _readHeader(self):
        """Read the header of pdb/prc file and init class based on this data."""
        # The structure of the header:
        # offset, len, what
        #  0, 32, zero-terminated name of the database, max 31 chars in len
        # 32,  2, attributes
        #   0x0002 Read-Only
        #   0x0004 Dirty AppInfoArea
        #   0x0008 Backup this database (i.e. no conduit exists)
        #   0x0010 (16 decimal) Okay to install newer over existing copy, if present on PalmPilot
        #   0x0020 (32 decimal) Force the PalmPilot to reset after this database is installed
        #   0x0040 (64 decimal) Don't allow copy of file to be beamed to other Pilot.
        # 34,  2, version
        # 36,  4, creation-date  (number of seconds since January 1, 1904)
        # 40,  4, modification-date, set to 0
        # 44,  4, last-backup-date
        # 48,  4, modification-number
        # 52,  4, app-info-area, offset in PDB file
        # 56,  4, sort-info-area, offset in PDB file
        # 60,  4, db-type, 4-byte which is usually some sort of string
        # 64,  4, creator-id
        # 68,  4, seed-id, always 0 ?
        # 72,  4, next-record-list, set to 0, only used when in memory
        # 76,  2, records-count, number of records in the database file

        #         hdrDef = ["name", "32s",
        #                   "attr", "H",
        #                   "version", "H",
        #                   "creationDate", "L",
        #                   "modificationDate", "L",
        #                   "lastBackupDate", "L",
        #                   "modificationNum", "L",
        #                   "appInfoArea", "L",
        #                   "sortInfoArea", "L",
        #                   "dbType", "4s",
        #                   "creatorId", "4s",
        #                   "seedId", "L",
        #                   "nextRecordList", "L",
        #                   "recordsCount", "H"]

        fo = open(self._fileName,"rb")
        fileSize = _getFileSizeFromFileObject(fo)

        if fileSize <= 78: self._raiseInvalidFile() # sanity check

        headerData = fo.read(78)

        (self._name, self._attr, self._version, self._creationdate,
         self._modificationDate, self._lastBackupDate, self._modificationNum,self._appInfoArea,
         self._sortInfoArea, self._dbType, self._creatorId, self._seedId,
         self._nextRecordList,recordsCount) = struct.unpack(">32sHHLLLLLL4s4sLLH", headerData)
        self._name = self._name.strip("\x00")

        #self._header = structhelper.ExtractData(hdrDef,headerData,isBigEndian=True)
        #self._header["name"] = self._header["name"].strip("\x00")

        # sanity checkig of the pdb file
        #recordsCount = self._header["recordsCount"]
        if fileSize < 78+8*recordsCount:
            self._raiseInvalidFile()
        if self._seedId != 0:
            # TODO: not sure about that, maybe it's valid
            self._raiseInvalidFile()
        if self._nextRecordList != 0:
            # TODO: not sure about that, maybe it's valid
            self._raiseInvalidFile()

        # read info about records, need to accumulate to calc sizes from offsets
        recHeaderList = []
        (_OFFSET,_ATTR) = 0,1
        for n in range(recordsCount):
            # record header format:
            # offset, len, what
            # 0, 4, offset of the record in the file (counting from 0)
            # 4, 1, attributes of the record:
            #   0x10 (16 decimal) Secret record bit.
            #   0x20 (32 decimal) Record in use (busy bit).
            #   0x40 (64 decimal) Dirty record bit.
            #   0x80 (128, unsigned decimal) Delete record on next HotSync.
            #   The least significant four bits are used to represent the category values.
            # 5, 3, uniqueId, set to 0 (we don't care about it and don't expose it to users)
            recHeaderList.append( struct.unpack(">LB3s", fo.read(8) ) )
            if recHeaderList[n][_OFFSET] >= fileSize:
                self._raiseInvalidFile()

        for n in range(recordsCount):
            if n < recordsCount-1:
                recSize = recHeaderList[n+1][_OFFSET] - recHeaderList[n][_OFFSET];
            else:
                recSize = fileSize - recHeaderList[n][_OFFSET]
            if recSize <= 0:
                self._raiseInvalidFile()
            self._records.append( PDBRecordFromDisk(self._fileName,
                                                    recHeaderList[n][_ATTR],
                                                    recHeaderList[n][_OFFSET],
                                                    recSize) )
        fo.close()

    def _getName(self):
        return self._name
    def _setName(self,name):
        if len(name)>31:
            raise ValueError("name (%s) must be shorter than 32 chars" % name)
        self._name = name

    name = property(_getName,_setName)

    def _getCreator(self):
        assert len(self._creatorId) == 4
        return self._creatorId

    def _setCreator(self, creator):
        assert len(creator) == 4
        if len(creator) != 4:
            raise ValueError("creator must be exactly 4 bytes in length, and is '%s' (%d bytes)" % (creator,len(creator)))
        self._creatorId = creator
    creator = property(_getCreator,_setCreator)

    def _getType(self):
        assert len(self._dbType) == 4
        return self._dbType

    def _setType(self,type):
        assert len(type) == 4
        if len(type) != 4:
            raise ValueError("type must be exactly 4 bytes in length, and is '%s' (%d bytes)" % (type,len(type)))
        self._dbType = type
    dbType = property(_getType,_setType)

    def _getRecords(self):
        return self._records
    records = property(_getRecords)

    def _checkRecords(self):
        """All records should be of proper type"""
        for rec in self.records:
            assert isinstance(rec,PDBRecordFromDisk) or isinstance(rec,PDBRecordFromData)
            if not (isinstance(rec,PDBRecordFromDisk) or isinstance(rec,PDBRecordFromData)):
                raise ValueError("all records must be of type PDBRecordFromDisk or PDBRecordFromData")

    def saveAs(self,fileName,overwrite=False):
        assert fileName != None
        if not overwrite:
            # TODO: bail out if file exists
            pass
        # trick: if we're writing to the same file, force reading data in memory for PDBRecordFromDisk
        if fileName == self._fileName:
            for rec in self._records:
                # TODO: only if this is instance of PDBRecordFromDisk
                d = rec.data  # force reading data into memory
        self._checkRecords()
        raise NotImplementedError
