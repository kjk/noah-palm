# convert the French dictionary data in XML to my simple format

import string, os.path
from xml.dom import minidom
import xml.sax.handler, xml.sax

class WordDef(object):
    def __init__(self,word):
        self.word = word
    def _setWord(self,word):
        self._word = word
    def _getWord(self):
        return self._word
    word = property(_getWord,_setWord)

    def _getMeanings(self):
        return self._meanings
    def addMeaning(self,meaning):
        self._meanings.append(meaning)
    meanings = property(_getMeanings)

def _get_attr(attrs,name):
    name = string.lower(name)
    for a in attrs:
        if string.lower(a[0]) == name:
            return a[1]
    return None

class myEntityResolver(xml.sax.handler.EntityResolver):
    def resolveEntity(self, publicId, systemId):
        print "hello"
        print publicId
        print systemId
        return xml.sax.handler.EntityResolver.resolveEntity(self,publicId,systemId)

def _dumpNodes(nodes):
    for n in nodes:
        print n
        print n.nodeName
        print n.parentNode.nodeName

def _getWord(xmldoc):
    """Extract the word from xml word definition. Word is
    /Refrence/Entree.lang/Sommaire/Fromat/text_node()  or
    /Refrence/Entree.lang/Titre/Format/text()"""
    titre = xmldoc.getElementsByTagName("Titre")
    assert len(titre) == 1
    fmt = titre[0].getElementsByTagName("Format")
    #TODO: handle (as in a0500049.xml) ../Sommaire[ /Format, /Connecteur, /Format ]
    #print fmt
    #assert len(fmt) == 1
    fmtnode = fmt[0].firstChild
    assert fmtnode.nodeType == fmtnode.TEXT_NODE
    word = fmtnode.nodeValue
    return word

def _getCateg(xmldoc):
    """Extrace grammatical category from xml word definition. It's in
    /Reference/Entree/Sommaire/Categ[type=group]/Categ[type=gram]/text()  or
    /Reference/Entree/Titre/Categ[type=group]/Categ[type=gram]/text()"""
    # TODO: handle ../Categ[type=group/Categ[type=gram]/text()
    #              ../Categ[type=group/Connectecteur/text()="et"
    #              ../Categ[type=group/Categ[type=gram]/text()
    categNodes = xmldoc.getElementsByTagName("Categ")
    categNodes = [n for n in xmldoc.getElementsByTagName("Categ") if n.attributes["type"].nodeValue=="gram"]
    #print categNodes
    #TODO above assert len(categNodes) == 2
    categ = _getTxtNodeText(categNodes[0].firstChild)
    #assert categ == _getTxtNodeText(categNodes[1].firstChild)
    return string.strip(categ)

def _getTopLevelSenses(xmldoc):
    """Return a list of <Sens> nodes that are directly under /Reference/Entree.lang/"""
    senses = xmldoc.getElementsByTagName("Sens")
    #_dumpNodes(senses)
    return [s for s in senses if s.parentNode.nodeName == "Entree.lang"]

def _getTxtNodeText(node):
    assert node.nodeType == node.TEXT_NODE
    return node.nodeValue

def _getTxtNodes(node):
    return [n for n in node.childNodes if n.nodeType == n.TEXT_NODE]

def _getTxtNodesText(node):
    txtNodes = _getTxtNodes(node)
    txt = string.join( [_getTxtNodeText(n) for n in txtNodes], "" )
    #print txt
    return txt

def _getTxtNodeChild(node):
    txtNodes = _getTxtNodes(node)
    print txtNodes
    assert len(txtNodes) == 1
    assert txtNodes[0].nodeType == txtNodes[0].TEXT_NODE
    return txtNodes[0]

def _convertToValidCateg(cat):
    catdict = { "n. masc." : "n.",
                "n. fem." : "n.",
                "n." : "n.",
                "adj." : "adj.",
                "v. trans." : "v.t.",
                "v. pronom." : "v.i."}
    return catdict[cat]

def parseEntreeLangNode(node):
    word = _getWord(node)
    categ = _convertToValidCateg(_getCateg(node))
    print "\n" + word
    print categ
    senses = _getTopLevelSenses(node)
    #_dumpNodes(senses)
    for s in senses:
        #print "%s" % string.strip(num)
        subSenses = s.getElementsByTagName("Sens")
        if len(subSenses) == 0:
            # this is ../Sens/text() scenario
            senseDef = _getTxtNodesText(s)
            print string.strip(senseDef)
        else:
            try:
                num = _getTxtNodeText(s.firstChild)
            except AssertionError:
                # the case where we have ../Sens/Sens but not ../Sens.text()/Sens where text() is num
                num = ""
            # this is ../Sens/Sens/text() scenario
            subCount = 0
            for subSense in s.getElementsByTagName("Sens"):
                senseDef = _getTxtNodesText(subSense)
                subCount += 1
                if subCount == 1:
                    print "%s %s" % (string.strip(num),string.strip(senseDef))
                else:
                    print "%s" % string.strip(senseDef)
        
def parseFile(file):
    #print "file: %s" % file
    fo = open(file,"rb")
    xmlstr = fo.read()
    fo.close()
    entitydir = { "ti" : "-",
                  "ccedil" : "c",
                  "eacute" : "e",
                  "Eacute" : "E",
                  "egrave" : "e",
                  "ecirc" : "e",
                  "Ecirc" : "E",
                  "agrave" : "a",
                  "Agrave" : "A",
                  "acirc" : "a",
                  "oelig" : "o",
                  "icirc" : "i",
                  "iuml" : "i",
                  "ugrave" : "u",
                  "#160" : "a",
                  "#39" : "'",
                  "ch" : "?", # don't know
                  "schwa" : "?", # don't know
                  "laquo" : "", # don't know
                  "raquo" : "", # don't know
                  "nsdot" : ".", # not sure
                  "nbsp" : " "}
                  
    for old in entitydir.keys():
        new = entitydir[old]
        xmlstr = string.replace(xmlstr,"&"+old+";",new)
    #print xmlstr
    xmldoc = xml.dom.minidom.parseString(xmlstr)
    for n in xmldoc.getElementsByTagName("Entree.lang"):
        parseEntreeLangNode(n)

def parseFile2(file):
    parser = xml.sax.make_parser()
    res = parser.getEntityResolver()
    print res
    myRes = myEntityResolver()
    print myRes
    parser.setEntityResolver(myRes)
    res = parser.getEntityResolver()
    print res
    xmldoc = xml.dom.minidom.parse(file,parser)
    titre = xmldoc.getElementsByTagName("Titre")
    assert len(titre) == 1
    fmt = titre[0].getElementsByTagName("Format")
    assert len(fmt) == 1
    print fmt[0].localName
    
def parseFiles():
    dir = r"c:\kjk\downloads\FrenchDictionarySample"
    files = ["v0001222.xml", "c0004317.xml", "d0503856.xml", "a0500049.xml", "c0504311.xml"]
    for f in files:
        path = os.path.join(dir,f)
        parseFile(path)

if __name__=="__main__":
    parseFiles()
