# Copyright: Krzysztof Kowalczyk
# Owner: Krzysztof Kowalczyk
#
# Emulates client (Palm) application by issuing requests to the server.
# For testing the server.
#
# Usage:
#  -get word
#  -getrandom
#  -newcookie
#  -recentlookups
#  -regcode reg_code
#   add cmd line options -get term, -get-random for driving the script from cmd line

import sys, re, socket, random, pickle, httplib, urllib

# server string must be of form "name:port"
g_serverList = ["dict-pc.arslexis.com:4080", "dict.arslexis.com:80", "dict-pc.local.org:80"]

g_defaultServerNo = 0 # index within g_serverList

g_cookie = None
g_exampleDeviceInfo = "HS50616C6D204F5320456D756C61746F72:OC70616C6D:OD00000000"

DEF_ID  = "DEF"

g_commonParams = {'pv': '1', 'cv': '1'}

g_pickleFileName = "client_pickled_data.dat"
def pickleState():
    global g_pickleFileName,g_cookie
    # save all the variables that we want to persist across session on disk
    fo = open(g_pickleFileName, "wb")
    pickle.dump(g_cookie,fo)
    fo.close()

def unpickleState():
    global g_cookie
    # restores all the variables that we want to persist across session from
    # the disk
    try:
        fo = open(g_pickleFileName, "rb")
    except IOError:
        # it's ok to not have the file
        return
    g_cookie = pickle.load(fo)
    fo.close()

def getGlobalCookie():
    global g_cookie
    return g_cookie

g_fShownServer = False
def getCurrentServer():
    global g_serverList,g_defaultServerNo,g_fShownServer
    if not g_fShownServer:
        print "using server: %s" % g_serverList[g_defaultServerNo]
        g_fShownServer = True
    return g_serverList[g_defaultServerNo]

def fDetectRemoveCmdFlag(flag):
    fFlagPresent = False
    try:
        pos = sys.argv.index(flag)
        fFlagPresent = True
        sys.argv[pos:pos+1] = []
    except:
        pass
    return fFlagPresent

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

def buildGetCookieConn():
    global g_commonParams, g_exampleDeviceInfo
    server = getCurrentServer()
    conn = httplib.HTTPConnection(server)
    params = g_commonParams;
    conn.request("GET", "/palm.php?pv=1&cv=1&get_cookie=&di=%s" % g_exampleDeviceInfo)
    return conn

def buildGetRandomDefConn():
    global g_commonParams
    server = getCurrentServer()
    conn = httplib.HTTPConnection(server)
    conn.request("GET", "/palm.php?pv=1&cv=1&c=%s&get_random_word=" % getGlobalCookie())
    return conn

def buildGetRecentLookupsConn():
    global g_commonParams
    server = getCurrentServer()
    conn = httplib.HTTPConnection(server)
    conn.request("GET", "/palm.php?pv=1&cv=1&c=%s&recent_lookups=" % getGlobalCookie())
    return conn

def extractCookieFromResponse(res):
    lines = res.split()
    if 2!=len(lines):
        return None
    if lines[0] != 'COOKIE':
        return None
    cookie = lines[1]
    print "COOKIE=%s" % cookie
    return cookie

def getCookie():
    global g_cookie
    conn = buildGetCookieConn()
    res = conn.getresponse()
    #print res.status, res.reason
    data = res.read()
    #print data
    conn.close()
    cookie = extractCookieFromResponse(data)
    g_cookie = cookie
    pickleState()

def buildGetDefConn(term,regCode):
    server = getCurrentServer()
    conn = httplib.HTTPConnection(server)
    params = g_commonParams;
    params['c'] = getGlobalCookie()
    params['get_word'] = term
    if regCode:
        params['rc'] = regCode
    encoded = urllib.urlencode(params)
    url = "/palm.php?" + encoded
    print "url: %s" % url
    conn.request("GET", url)
    return conn

def getDef(term,regCode):
    conn = buildGetDefConn(term,regCode)
    res = conn.getresponse()
    #print res.status, res.reason
    data = res.read()
    conn.close()
    return data

def getRandomDef():
    conn = buildGetRandomDefConn()
    res = conn.getresponse()
    #print res.status, res.reason
    data = res.read()
    conn.close()
    return data

def getRecentLookups():
    conn = buildGetRecentLookupsConn()
    res = conn.getresponse()
    #print res.status, res.reason
    data = res.read()
    conn.close()
    return data


def doGetDef(term,regCode):
    if not getGlobalCookie():
        getCookie()
    data = getDef(term,regCode)
    print data

def doGetRandomDef():
    if not getGlobalCookie():
        getCookie()
    data = getRandomDef()
    print data

def doGetRecentLookups():
    if not getGlobalCookie():
        getCookie()
    data = getRecentLookups()
    print "'%s'" % data

def usageAndExit():
    print "Usage: client.py [-newcookie] [-get word] [-getrandom] [-recentlookups] [-regcode reg_code]"
    sys.exit(0)

def main():
    if len(sys.argv)<2:
        usageAndExit()
    if fDetectRemoveCmdFlag("-newcookie"):
        getCookie()      

    regCode = getRemoveCmdArg("-regcode")
    if fDetectRemoveCmdFlag("-getrandom"):
        doGetRandomDef()

    if fDetectRemoveCmdFlag("-recentlookups"):
        doGetRecentLookups()

    word = getRemoveCmdArg("-get")
    if word:
        doGetDef(word,regCode)
    if len(sys.argv)>1:
        usageAndExit()

if __name__=="__main__":
    try:
        unpickleState()
        main()
    finally:
        # make sure that we pickle the state even if we crash
        pickleState()
 
