# Copyright: Krzysztof Kowalczyk
# Owner: Krzysztof Kowalczyk
#
# Emulates client (Palm) application by issuing requests to the server.
# For testing the server.
#
# TODO:
#   add cmd line options -get term, -get-random for driving the script from cmd line

import sys, re, socket, random, pickle, httplib, urllib

# server string must be of form "name:port"
g_serverList = ["dict-pc.arslexis.com:3000", "dict.arslexis.com:80"]

g_defaultServerNo = 0 # index within g_serverList

g_cookie = None
g_exampleDeviceInfo = "HS50616C6D204F5320456D756C61746F72:OC70616C6D:OD00000000"

DEF_ID  = "DEF"

g_commonParams = {'pv': '1', 'cv': '0.5'}

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

def getCurrentServer():
    global g_serverList,g_defaultServerNo
    print "using server: %s" % g_serverList[g_defaultServerNo]
    return g_serverList[g_defaultServerNo]

def getServerNamePort():
    srv = g_serverList[g_defaultServerNo]
    (name,port) = srv.split(":")
    port = int(port)
    return (name,port)

def buildGetCookieConn():
    global g_commonParams, g_exampleDeviceInfo
    server = getCurrentServer()
    conn = httplib.HTTPConnection(server)
    params = g_commonParams;
    conn.request("GET", "/palm.php?pv=1&cv=0.5&get_cookie=&di=%s" % g_exampleDeviceInfo)
    return conn

def buildGetRandomDefConn():
    global g_commonParams
    server = getCurrentServer()
    conn = httplib.HTTPConnection(server)
    conn.request("GET", "/palm.php?pv=1&cv=0.5&c=%s&get_random_word=" % getGlobalCookie())
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

def buildGetDefConn(term):
    global g_commonParams
    server = getCurrentServer()
    conn = httplib.HTTPConnection(server)
    params = g_commonParams;
    params = urllib.urlencode(params)
    conn.request("GET", "/palm.php?pv=1&cv=0.5&c=%s&get_word=%s" % (getGlobalCookie(),term))
    return conn

def getDef(term):
    conn = buildGetDefConn(term)
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

def doGetDef(term):
    data = getDef(term)
    print data

def doGetRandomDef():
    cookie = getGlobalCookie()
    data = getRandomDef()
    print data

def main():
    if not getGlobalCookie():
        getCookie()
    #doGetRandomDef()
    doGetDef("gesture")

if __name__=="__main__":
    try:
        unpickleState()
        main()
    finally:
        # make sure that we pickle the state even if we crash
        pickleState()
 
