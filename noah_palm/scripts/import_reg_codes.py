# Copyright: Krzysztof Kowalczyk
# Owner: Krzysztof Kowalczyk
#
# Purpose:
#   Import data from *.csv file with reg codes generated by inoah_gen_reg_codes.py
#   into a database so that the server can properly authenticate users.

import sys,os,csv,MySQLdb,_mysql_exceptions
import inoah_gen_reg_codes

g_conn = MySQLdb.Connect(host='localhost', user='root', passwd='', db='inoahdb')
g_cur = g_conn.cursor()

def dbEscape(txt):
    # it's silly that we need connection just for escaping strings
    global g_conn
    return g_conn.escape_string(txt)

def sql_qq(txt):
    return dbEscape(txt)

def getOneResult(conn,query):
    cur = conn.cursor()
    cur.execute(query)
    row = cur.fetchone()
    res = row[0]
    cur.close()
    return res

def fCodeExists(reg_code):
    global g_conn
    query = "SELECT COUNT(*) FROM reg_codes WHERE reg_code='%s';" % sql_qq(reg_code)
    count = int(getOneResult(g_conn,query))
    assert 0==count or 1==count
    if 0==count:
        return False
    return True

def getCodesInDBCount():
    global g_conn
    query = "SELECT COUNT(*) FROM reg_codes;"
    count = int(getOneResult(g_conn,query))
    return count

def main():
    global g_conn, g_cur
    codes = inoah_gen_reg_codes.readPreviousCodes()

    # verify codes are in correct format
    for c in codes.keys():
        reg_code = c
        assert 12 == len(reg_code)
        purpose = codes[c][0]
        assert purpose in ["h","pg", "es"] or "s:" == purpose[:2]
        assert len(purpose) < 200 # the column is 255, use 200 just in case

    insertedCount = 0
    existingCount = 0
    for c in codes.keys():
        reg_code = c
        purpose = codes[c][0]
        date = codes[c][1]
        # print "code='%s', purpose='%s', date='%s'" % (reg_code, purpose, date)
        query = "INSERT INTO reg_codes VALUES ('%s','%s','%s', 'f');" % \
            (sql_qq(reg_code), sql_qq(purpose), sql_qq(date))
        if not fCodeExists(reg_code):
            g_cur.execute(query)
            insertedCount += 1
        else:
            existingCount += 1

    print "New codes inserted: %d" % insertedCount
    print "Existing codes    : %d" % existingCount

    codesInDBCount = getCodesInDBCount()
    if codesInDBCount != insertedCount + existingCount:
        print "Total codes in the db: %d" % codesInDBCount
        expectedCount = insertedCount+existingCount
        print "Should be:           : %d (inserted count + existing count)" % expectedCount

if __name__=="__main__":
    main()

