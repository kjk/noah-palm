# Copyright: Krzysztof Kowalczyk
# Owner: Krzysztof Kowalczyk
#
# Purpose:
#  Generate registration codes for iNoah. Our goals:
#  - generate codes incrementally
#  - codes can be for handango, palmgear and esellerate
#  - codes should be unique and random and not easy to find
#  - generate *.sql file that we can import into inoahdb to install those regcodes
#  - generate *.txt file with codes ready to be uploaded to esellerate/pg/handango
#
# How it works.
#
# Generating codes. We plan to sell about 100.000 copies. If we want to 
# have 1% chance of guessing a regcode, we have to generate 100.000.00
# i.e. 8 digit random number. We make it 12-digit random number just
# for kicks
#
# We store generated numbers in reg_codes.csv file. It's a CSV file
# with the following fields:
# regcode, purpose, when
#   'when' is the time it was generated in the "YYYY-MM-DD hh:mm" format
#   'purpose' describes to whom it was assigned (i.e. 'pg' for PalmGear,
#             'h' for handango, 'es' for eSellerate
# and custom strings for special reg codes)
#
# We also generate a *.txt file with serial numbers ready to be uploaded to
# eSellerate/PalmGear/Handango. The format of this file:
#  eSellerate: serial number per line, at least 500 serial numbers
#  TODO: PalmGear, Handango
#
# We also generate a *.sql file that must be imported to the inoahdb.reg_codes
# table so that those codes are 
#
# Usage:
#  -count N : generate $N reg codes
#  -purpose who : reg codes usage. Valid entries:
#     - 'h' for Handango
#     - 'pg' for PalmGear
#     - 'es' for eSellerate
#  -special who: a special code. the idea is to give out codes to some people
#                (like reviewers) and be able to track what they do with the
#                software
#
# History:
#  2004-04-15  created

import sys,os,csv,random,time,StringIO

g_regCodesFileName        = "reg_codes.csv"
REG_CODE_LEN = 12

# given argument name in argName, tries to return argument value
# in command line args and removes those entries from sys.argv
# return None if not found
def getRemoveCmdArg(argName):
    argVal = None
    try:
        pos = sys.argv.index(argName)
        argVal = sys.argv[pos+1]
        sys.argv[pos:pos+2] = []
    except:
        pass
    return argVal

def fFileExists(filePath):
    try:
        st = os.stat(filePath)
    except OSError:
        # TODO: should check that Errno is 2
        return False
    return True

# generate a random reg code of a given length (in decimal numbers)
def genRegCode(regCodeLen):
    regCode = ""
    for i in range(regCodeLen):
        num=chr(random.randint(ord('0'), ord('9')))
        assert num>='0' and num<='9'
        regCode += num
    return regCode

g_curDate = None
def getStableDate():
    global g_curDate
    if None == g_curDate:
        g_curDate = time.localtime()
    return g_curDate

g_curDateTxt = None
def getStableDateTxt():
    global g_curDateTxt
    if None == g_curDateTxt:
        g_curDateTxt = time.strftime("%Y-%m-%d %H:%M", getStableDate())
    return g_curDateTxt

def getSQLFileName():
    fileName = time.strftime("%Y-%m-%d_%H_%M.sql", getStableDate())
    return fileName

# we need to read previous csv data in order to avoid generating duplicate codes
def readPreviousCodes():
    global g_regCodesFileName
    prevCodes = {}
    fo = None
    try:
        fo = open(g_regCodesFileName,"rb")
    except:
        # file doesn't exist - that's ok
        pass
    if fo:
        reader = csv.reader(fo)
        for row in reader:
            prevCodes[row[0]] = row[1:]
        fo.close()
    return prevCodes

def saveNewCodesToCsv(newCodes):
    global g_regCodesFileName
    csvFile = StringIO.StringIO()
    csvWriter = csv.writer(csvFile)
    # csvRow is: [regCode, purpose, when_generated]
    for regCode in newCodes.keys():
        csvRow = [regCode, newCodes[regCode][0], newCodes[regCode][1]]
        csvWriter.writerow(csvRow)
    csvTxt = csvFile.getvalue()
    csvFile.close()
    fo = open(g_regCodesFileName, "ab")
    fo.write(csvTxt)
    fo.close()

def saveNewCodesToSQL(newCodes):
    # TODO: should I check if the file exists? Or maybe just append?
    fo = open(getSQLFileName(), "wb")
    for code in newCodes.keys():
        purpose = newCodes[code][0]
        fo.write("INSERT INTO VALUES('%s','%s')\n" % (code, purpose))
    fo.close()

def genNewCodes(count, purpose, prevCodes):
    newCodes = {}
    dateTime = getStableDateTxt()
    for t in range(count):
         newCode = genRegCode(REG_CODE_LEN)
         while prevCodes.has_key(newCode) or newCodes.has_key(newCode):
            print "found dup: %s" % newCode
            newCode = genRegCode(REG_CODE_LEN)
         newCodes[newCode] = [purpose,dateTime]
    return newCodes

def usageAndExit():
    print "Usage: inoah_gen_reg_codes.py [-special who] [-count N -purpose who]"
    sys.exit(0)

def main():
    if fFileExists(getSQLFileName()):
        print "file %s exists; wait a minute. literally." % getSQLFileName()
        usageAndExit()

    special = getRemoveCmdArg("-special")
    if special:
        if len(sys.argv) != 1:
            print "no other arguments allowed when using -special"
            usageAndExit()
    else:
        regCodesCount = getRemoveCmdArg("-count")
        if None == regCodesCount:
            usageAndExit()
        regCodesCount = int(regCodesCount)
        validPurpose = ["pg", "h", "es"]
        purpose = getRemoveCmdArg("-purpose")
        if None == purpose:
            usage()
        if purpose not in validPurpose:
            print 'purpose cannot be %s. Must be "pg", "h" or "es"' % purpose
            usageAndExit()
    # invalid number of arguments
    if len(sys.argv) != 1:
        print "invalid argument '%s'" % sys.argv[1]
        usageAndExit()

    print "reading previous codes"
    prevCodes = readPreviousCodes()
    print "read %d old codes" % len(prevCodes)
    print "start generating new codes"

    if special:
        newCodes = genNewCodes(1, special, prevCodes)
    else:
        newCodes = genNewCodes(regCodesCount, purpose, prevCodes)
    print "generated %d new codes" % len(newCodes)
    saveNewCodesToCsv(newCodes)
    saveNewCodesToSQL(newCodes)

if __name__=="__main__":
    main()

