/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: Szymon Knitter (szknitter@wp.pl) 
*/


/**
 * @file pronunciation.c
 * @author Szymon Knitter (szknitter@wp.pl) 
 */

#include "common.h"
#include "pronunciation.h"

/**
 *  Remove '//' if you want to work without selection
 */

//#define DONT_DO_PRONUNCIATION 1

/**
 *  Get from {AA, AO, ... } numbers... 
 */ 
static char pronRetPronNo(char a, char b)
{
    if(a == 0)  return 0;
    //find what pron is that
	if(a == 'A' && b == 'A') return 1;
	if(a == 'A' && b == 'E') return 2;
	if(a == 'A' && b == 'H') return 3;
	if(a == 'A' && b == 'O') return 4;
	if(a == 'A' && b == 'W') return 5;
	if(a == 'A' && b == 'Y') return 6;
	if(a == 'B' && b ==  0 ) return 7;
	if(a == 'C' && b == 'H') return 8;
	if(a == 'D' && b ==  0 ) return 9;
	if(a == 'D' && b == 'H') return 10;
	if(a == 'E' && b == 'H') return 11;
	if(a == 'E' && b == 'R') return 12;
	if(a == 'E' && b == 'Y') return 13;
	if(a == 'F' && b ==  0 ) return 14;
	if(a == 'G' && b ==  0 ) return 15;
	if(a == 'H' && b == 'H') return 16;
	if(a == 'I' && b == 'H') return 17;
	if(a == 'I' && b == 'Y') return 18;
	if(a == 'J' && b == 'H') return 19;
	if(a == 'K' && b ==  0 ) return 20;
	if(a == 'L' && b ==  0 ) return 21;
	if(a == 'M' && b ==  0 ) return 22;
	if(a == 'N' && b ==  0 ) return 23;
	if(a == 'N' && b == 'G') return 24;
	if(a == 'O' && b == 'W') return 25;
	if(a == 'O' && b == 'Y') return 26;
	if(a == 'P' && b ==  0 ) return 27;
	if(a == 'R' && b ==  0 ) return 28;
	if(a == 'S' && b ==  0 ) return 29;
	if(a == 'S' && b == 'H') return 30;
	if(a == 'T' && b ==  0 ) return 31;
	if(a == 'T' && b == 'H') return 32;
	if(a == 'U' && b == 'H') return 33;
	if(a == 'U' && b == 'W') return 34;
	if(a == 'V' && b ==  0 ) return 35;
	if(a == 'W' && b ==  0 ) return 36;
	if(a == 'Y' && b ==  0 ) return 37;
	if(a == 'Z' && b ==  0 ) return 38;
	if(a == 'Z' && b == 'H') return 39;
    //default
	return 0;
}

/**
 *  Version with AA, AO, ...
 */ 
static char* pronTranslateDecompresedVer1(struct _AppContext *appContext, unsigned char *decompresed)
{
    ExtensibleBuffer *Buf;
    int i = 0;
    char *ret = NULL;
    
    Buf = ebufNew();
    Assert(Buf);

    do
    {
        switch(decompresed[i])
        {
            case  0: ebufAddChar(Buf,'\0'); break;
            case  1: ebufAddStr(Buf,"AA"); break;
            case  2: ebufAddStr(Buf,"AE"); break;
            case  3: ebufAddStr(Buf,"AH"); break;
            case  4: ebufAddStr(Buf,"AO"); break;
            case  5: ebufAddStr(Buf,"AW"); break;
            case  6: ebufAddStr(Buf,"AY"); break;
            case  7: ebufAddStr(Buf,"B"); break;
            case  8: ebufAddStr(Buf,"CH"); break;
            case  9: ebufAddStr(Buf,"D"); break;
            case 10: ebufAddStr(Buf,"DH"); break;
            case 11: ebufAddStr(Buf,"EH"); break;
            case 12: ebufAddStr(Buf,"ER"); break;
            case 13: ebufAddStr(Buf,"EY"); break;
            case 14: ebufAddStr(Buf,"F"); break;
            case 15: ebufAddStr(Buf,"G"); break;
            case 16: ebufAddStr(Buf,"HH"); break;
            case 17: ebufAddStr(Buf,"IH"); break;
            case 18: ebufAddStr(Buf,"IY"); break;
            case 19: ebufAddStr(Buf,"JH"); break;
            case 20: ebufAddStr(Buf,"K"); break;
            case 21: ebufAddStr(Buf,"L"); break;
            case 22: ebufAddStr(Buf,"M"); break;
            case 23: ebufAddStr(Buf,"N"); break;
            case 24: ebufAddStr(Buf,"NG"); break;
            case 25: ebufAddStr(Buf,"OW"); break;
            case 26: ebufAddStr(Buf,"OY"); break;
            case 27: ebufAddStr(Buf,"P"); break;
            case 28: ebufAddStr(Buf,"R"); break;
            case 29: ebufAddStr(Buf,"S"); break;
            case 30: ebufAddStr(Buf,"SH"); break;
            case 31: ebufAddStr(Buf,"T"); break;
            case 32: ebufAddStr(Buf,"TH"); break;
            case 33: ebufAddStr(Buf,"UH"); break;
            case 34: ebufAddStr(Buf,"UW"); break;
            case 35: ebufAddStr(Buf,"V"); break;
            case 36: ebufAddStr(Buf,"W"); break;
            case 37: ebufAddStr(Buf,"Y"); break;
            case 38: ebufAddStr(Buf,"Z"); break;
            case 39: ebufAddStr(Buf,"ZH"); break;
            default: ebufAddChar(Buf,'\0'); break;
        }    
        if(decompresed[i] >= 1 && decompresed[i] <= 39)
            if(decompresed[i+1] >= 1 && decompresed[i+1] <= 39)
                ebufAddChar(Buf,' ');
        
    }while(decompresed[i++]!=0x00);

    ret = new_malloc_zero(Buf->used);
    MemMove(ret, Buf->data, Buf->used);
    ebufDelete(Buf);
    return ret;
}

/**
 *  Translate array of phonemNo to phonem string
 *  [1,4,6] -> "AA AO AY"
 *  Return pointer to phonem string (delete it when you use it!!!)
 */
char* pronTranslateDecompresed(struct _AppContext *appContext, unsigned char *decompresed)
{
#ifdef DONT_DO_PRONUNCIATION
    return NULL;    
#endif    

    if(!appContext->prefs.displayPrefs.enablePronunciationSpecialFonts)
        return pronTranslateDecompresedVer1(appContext, decompresed);
    else
    /*In future... maybe we will have special fonts to display pronunciation*/
        return NULL;    
}

/**
 *  Move phonemes from compresed to decompresed.
 *  In compresed one char = more than one (4/3) phonem.
 *  In decompresed one char = one phonem.
 */
static void pronDecomprese(unsigned char *decompresed,unsigned char *compresed)
{
    unsigned char a,b,c,d;
    unsigned char A,B,C;
    int i,j,k;

    j = (unsigned char) compresed[0];
    i = 1;
    k = 0;
    while(j >= 3)
    {
        j -= 3;
        A = compresed[i++];
        B = compresed[i++];
        C = compresed[i++];
        //a,b,c,d <- A,B,C
        a = A & 0x3F;
        b = B & 0x3F;
        c = C & 0x3F;
        d = ((A & 0x0c0)>>2) + ((B & 0x0c0)>>4) + ((C & 0x0c0)>>6);
        
        decompresed[k++] = a;
        decompresed[k++] = b;
        decompresed[k++] = c;
        decompresed[k++] = d;
    }
    while(j > 0)
    {
        decompresed[k++] = (0x3F & (compresed[i++]));
        j--;
    }
    decompresed[k++] = 0;
}

/**
 *  Called only by "pronFillHelpBuffer" to make it shorter
 */
static void pronFillOneWord(struct _AppContext *appContext, ExtensibleBuffer *buf,
                            int nr, char *word, unsigned char * decompresed)
{
    char *pron;
    char pronTag[3];
    char wordTag[3];
    unsigned char oneChar[3];
    
    pronTag[0] = wordTag[0] = FORMAT_TAG;
    pronTag[1] = FORMAT_PRONUNCIATION;
    wordTag[1] = FORMAT_WORD;
    pronTag[2] = wordTag[2] = 0;

    ebufAddStr(buf,pronTag);
    oneChar[0] = (unsigned char) nr;
    oneChar[1] = 0;
    pron = pronTranslateDecompresed(appContext, oneChar);
    ebufAddStr(buf,pron);
    new_free(pron);
    ebufAddStr(buf,wordTag);
    ebufAddStr(buf,"    ");
    ebufAddStr(buf,word);
    ebufAddStr(buf,"    ");
    ebufAddStr(buf,pronTag);
    pron = pronTranslateDecompresed(appContext, decompresed);
    ebufAddStr(buf,pron);
    new_free(pron);
    ebufAddStr(buf,"\n");
}

/**
 *  Fill buffer with help about pronunciation
 */
static void pronFillHelpBuffer(struct _AppContext *appContext, ExtensibleBuffer *buf)
{
    unsigned char decompresed[10];
    int i = 1;
    
    //AA	odd	AA D   
    decompresed[0] = 1;
    decompresed[1] = 9;
    decompresed[2] = 0;
    pronFillOneWord(appContext,buf,i++, "odd", decompresed);
    //AE	at	AE T 
    decompresed[0] = 2;
    decompresed[1] = 31;
    decompresed[2] = 0;
    pronFillOneWord(appContext,buf,i++, "at", decompresed);
    //AH	hut	HH AH T 
    decompresed[0] = 16;
    decompresed[1] = 3;
    decompresed[2] = 31;
    decompresed[3] = 0;
    pronFillOneWord(appContext,buf,i++, "hut", decompresed);
    //AO	ought	AO T
    decompresed[0] = 4;
    decompresed[1] = 31;
    decompresed[2] = 0;
    pronFillOneWord(appContext,buf,i++, "ought", decompresed);
    //AW	cow	K AW
    decompresed[0] = 20;
    decompresed[1] = 5;
    decompresed[2] = 0;
    pronFillOneWord(appContext,buf,i++, "cow", decompresed);

    //AY	hide	HH AY D
    decompresed[0] = 16;
    decompresed[1] = 6;
    decompresed[2] = 9;
    decompresed[3] = 0;
    pronFillOneWord(appContext,buf,i++, "hide", decompresed);
    //B	    be	B IY
    decompresed[0] = 7;
    decompresed[1] = 18;
    decompresed[2] = 0;
    pronFillOneWord(appContext,buf,i++, "be", decompresed);
    //CH	cheese	CH IY Z
    decompresed[0] = 8;
    decompresed[1] = 18;
    decompresed[2] = 38;
    decompresed[3] = 0;
    pronFillOneWord(appContext,buf,i++, "cheese", decompresed);
    //D	    dee	D IY
    decompresed[0] = 9;
    decompresed[1] = 18;
    decompresed[2] = 0;
    pronFillOneWord(appContext,buf,i++, "dee", decompresed);
    //DH	thee	DH IY
    decompresed[0] = 10;
    decompresed[1] = 18;
    decompresed[2] = 0;
    pronFillOneWord(appContext,buf,i++, "thee", decompresed);
    
    //EH	Ed	EH D
    decompresed[0] = 11;
    decompresed[1] = 9;
    decompresed[2] = 0;
    pronFillOneWord(appContext,buf,i++, "Ed", decompresed);
    //ER	hurt	HH ER T
    decompresed[0] = 16;
    decompresed[1] = 12;
    decompresed[2] = 31;
    decompresed[3] = 0;
    pronFillOneWord(appContext,buf,i++, "hurt", decompresed);
    //EY	ate	EY T
    decompresed[0] = 13;
    decompresed[1] = 31;
    decompresed[2] = 0;
    pronFillOneWord(appContext,buf,i++, "ate", decompresed);
    //F	    fee	F IY
    decompresed[0] = 14;
    decompresed[1] = 18;
    decompresed[2] = 0;
    pronFillOneWord(appContext,buf,i++, "fee", decompresed);
    //G	    green	G R IY N
    decompresed[0] = 15;
    decompresed[1] = 28;
    decompresed[2] = 18;
    decompresed[3] = 23;
    decompresed[4] = 0;
    pronFillOneWord(appContext,buf,i++, "green", decompresed);
    
    //HH	he	HH IY
    decompresed[0] = 16;
    decompresed[1] = 18;
    decompresed[2] = 0;
    pronFillOneWord(appContext,buf,i++, "he", decompresed);
    //IH	it	IH T
    decompresed[0] = 17;
    decompresed[1] = 31;
    decompresed[2] = 0;
    pronFillOneWord(appContext,buf,i++, "it", decompresed);
    //IY	eat	IY T
    decompresed[0] = 18;
    decompresed[1] = 31;
    decompresed[2] = 0;
    pronFillOneWord(appContext,buf,i++, "eat", decompresed);
    //JH	gee	JH IY
    decompresed[0] = 19;
    decompresed[1] = 18;
    decompresed[2] = 0;
    pronFillOneWord(appContext,buf,i++, "gee", decompresed);
    //K 	key	K IY
    decompresed[0] = 20;
    decompresed[1] = 18;
    decompresed[2] = 0;
    pronFillOneWord(appContext,buf,i++, "key", decompresed);
    
    //L	    lee	L IY
    decompresed[0] = 21;
    decompresed[1] = 18;
    decompresed[2] = 0;
    pronFillOneWord(appContext,buf,i++, "lee", decompresed);
    //M	    me	M IY
    decompresed[0] = 22;
    decompresed[1] = 18;
    decompresed[2] = 0;
    pronFillOneWord(appContext,buf,i++, "me", decompresed);
    //N	    knee	N IY
    decompresed[0] = 23;
    decompresed[1] = 18;
    decompresed[2] = 0;
    pronFillOneWord(appContext,buf,i++, "knee", decompresed);
    //NG	ping	P IH NG
    decompresed[0] = 27;
    decompresed[1] = 17;
    decompresed[2] = 24;
    decompresed[3] = 0;
    pronFillOneWord(appContext,buf,i++, "ping", decompresed);
    //OW	oat	OW T
    decompresed[0] = 25;
    decompresed[1] = 31;
    decompresed[2] = 0;
    pronFillOneWord(appContext,buf,i++, "oat", decompresed);
    
    //OY	toy	T OY
    decompresed[0] = 31;
    decompresed[1] = 26;
    decompresed[2] = 0;
    pronFillOneWord(appContext,buf,i++, "toy", decompresed);
    //P 	pee	P IY
    decompresed[0] = 27;
    decompresed[1] = 18;
    decompresed[2] = 0;
    pronFillOneWord(appContext,buf,i++, "pee", decompresed);
    //R	    read	R IY D
    decompresed[0] = 28;
    decompresed[1] = 18;
    decompresed[2] = 9;
    decompresed[3] = 0;
    pronFillOneWord(appContext,buf,i++, "read", decompresed);
    //S	    sea	S IY
    decompresed[0] = 29;
    decompresed[1] = 18;
    decompresed[2] = 0;
    pronFillOneWord(appContext,buf,i++, "sea", decompresed);
    //SH	she	SH IY
    decompresed[0] = 30;
    decompresed[1] = 18;
    decompresed[2] = 0;
    pronFillOneWord(appContext,buf,i++, "she", decompresed);
    
    //T	    tea	T IY
    decompresed[0] = 31;
    decompresed[1] = 18;
    decompresed[2] = 0;
    pronFillOneWord(appContext,buf,i++, "tea", decompresed);
    //TH	theta	TH EY T AH
    decompresed[0] = 32;
    decompresed[1] = 13;
    decompresed[2] = 31;
    decompresed[3] = 3;
    decompresed[4] = 0;
    pronFillOneWord(appContext,buf,i++, "theta", decompresed);
    //UH	hood	HH UH D
    decompresed[0] = 16;
    decompresed[1] = 33;
    decompresed[2] = 9;
    decompresed[3] = 0;
    pronFillOneWord(appContext,buf,i++, "hood", decompresed);
    //UW	two	T UW
    decompresed[0] = 31;
    decompresed[1] = 34;
    decompresed[2] = 0;
    pronFillOneWord(appContext,buf,i++, "two", decompresed);
    //V	    vee	V IY
    decompresed[0] = 35;
    decompresed[1] = 18;
    decompresed[2] = 0;
    pronFillOneWord(appContext,buf,i++, "vee", decompresed);
    
    //W	    we	W IY
    decompresed[0] = 36;
    decompresed[1] = 18;
    decompresed[2] = 0;
    pronFillOneWord(appContext,buf,i++, "we", decompresed);
    //Y	    yield	Y IY L D
    decompresed[0] = 37;
    decompresed[1] = 18;
    decompresed[2] = 21;
    decompresed[3] = 9;
    decompresed[4] = 0;
    pronFillOneWord(appContext,buf,i++, "yield", decompresed);
    //Z	    zee	Z IY
    decompresed[0] = 38;
    decompresed[1] = 18;
    decompresed[2] = 0;
    pronFillOneWord(appContext,buf,i++, "zee", decompresed);
    //ZH	seizure	S IY ZH ER
    decompresed[0] = 29;
    decompresed[1] = 18;
    decompresed[2] = 39;
    decompresed[3] = 12;
    decompresed[4] = 0;
    pronFillOneWord(appContext,buf,i++, "seizure", decompresed);

    ebufAddChar(buf,'\0');
}

/**
 *  Display help screen - examples of all 39 phonems
 *  Works like "DisplayHelp(AppContext* appContext)"
 */

void pronDisplayHelp(AppContext* appContext, char* pronHitted)
{
    char *rawTxt;
    ExtensibleBuffer *Buf;

    if (NULL == appContext->currDispInfo)
    {
        appContext->currDispInfo = diNew();
        if (NULL == appContext->currDispInfo)
        {
            /* TODO: we should rather exit, since this means totally
               out of ram */
            return;
        }
    }
    
    Buf = ebufNew();
    Assert(Buf);

    pronFillHelpBuffer(appContext, Buf);
    
    rawTxt = ebufGetDataPointer(Buf);
    
    diSetRawTxt(appContext->currDispInfo, rawTxt);
    //after double click on pronunciation
    appContext->firstDispLine = pronRetPronNo(pronHitted[0], pronHitted[1]) - 1;
    if(appContext->firstDispLine > 35)
        appContext->firstDispLine = 35;
    
    ClearDisplayRectangle(appContext);
    cbNoSelection(appContext);    

    DrawDisplayInfo(appContext->currDispInfo, appContext->firstDispLine, DRAW_DI_X, DRAW_DI_Y, appContext->dispLinesCount);
    SetScrollbarState(appContext->currDispInfo, appContext->dispLinesCount, appContext->firstDispLine);
    ebufDelete(Buf);
}

/**
 *  Struct to move in index 
 */
typedef struct
{
    unsigned long   wordNo;
    unsigned int    recordNo;
    unsigned int    offset;
}PrIndex;

/**
 *  Sets prIndex on this one in witch is hidden pron for given wordNo
 */
static void pronGetIndex(PrIndex *prIndex, AbstractFile *file, UInt16 recordWithIndex, long wordNo)
{
    PrIndex         *prIndexNext;
    unsigned char   *data;
    long    offset = 0;
    long    maxOffset;
    
    data = (unsigned char *) fsLockRecord(file, recordWithIndex);
    maxOffset = fsGetRecordSize(file, recordWithIndex);
    
    prIndexNext = (PrIndex *) data;
   
    while((unsigned long)wordNo >= (unsigned long)prIndexNext->wordNo && offset < maxOffset)
    {
        offset += sizeof(PrIndex);
        if(offset < maxOffset)
            prIndexNext = (PrIndex *) &data[offset];
    }
    
    offset -= sizeof(PrIndex);
    prIndexNext = (PrIndex *) &data[offset];
    prIndex->wordNo = (unsigned long) prIndexNext->wordNo;
    prIndex->recordNo = prIndexNext->recordNo;
    prIndex->offset = prIndexNext->offset;
    fsUnlockRecord(file, recordWithIndex);
}

/**
 *  Get compresed pronunciation data from dictionary.
 *  Indexed by wordNo.
 *  return true if found else return false if not found.
 */
static Boolean pronGetCompresedWord(struct _AppContext *appContext, AbstractFile *file, unsigned char *compresed, long wordNo)
{
    PrIndex  prIndex;
    long     actWord;
    UInt16   recordWithPron;
    unsigned char i;
    unsigned char *data;
   
    //find index...
    pronGetIndex(&prIndex, file, appContext->pronData.recordWithPronIndex, wordNo);
    recordWithPron = prIndex.recordNo + appContext->pronData.firstRecordWithPron;
    //now find compresed pronunciation    
    data = (unsigned char *) fsLockRecord(file, recordWithPron);
    data += prIndex.offset;
    actWord = (long) prIndex.wordNo;
    //skip prons until wordNo reached
    while(actWord < wordNo)
    {
        actWord++;
        data += 1 + (unsigned char)data[0];
    }

    for(i=0;i <= (unsigned char)data[0];i++)
        compresed[i] = data[i];    

    fsUnlockRecord(file, recordWithPron);
    if(compresed[0] > 0)
        return true;
    return false;
}

/**
 *  MOST IMPORTANT FUNCTION!!!
 *  Adds Pronunciation of word (identified by wordNo).
 *  If pronunciation for hide = HH AY D
 *  we will add "[HH AY D]" to buffer.
 *  we don't add '\n'!!!
 *  return true if pronunciation found
 *  else return false (no pronunciation in DB, no pronunciation for word with wordNo)
 *
 *  we use "char *word" in case we can't find pronunciation, but the word is complex
 */
Boolean pronAddPronunciationToBuffer(struct _AppContext *appContext, ExtensibleBuffer *buf, AbstractFile *file, long wordNo, char *word)
{
    unsigned char compresed[PRON_COMPRESED_MAX_LEN];
    unsigned char decompresed[PRON_DECOMPRESED_MAX_LEN];
   	char wordPart[WORD_MAX_LEN + 2];
   	char *wordTest;
    char *pron;
    int  bufferPosition;
   	int  i,j;
   	long wordNoPart;
   	Boolean findSth = false;
    
#ifdef DONT_DO_PRONUNCIATION
    return false;
#endif    
    
    if(!pronGetCompresedWord(appContext,file,compresed,wordNo))
    {
        //TODO: complex pronunciation! ignore ' ', '-' and "(p)"
        bufferPosition = buf->used;
        
        ebufAddChar(buf,'[');
        
        //WinDrawChars(word,StrLen(word),20,140);
        
    	j = 0;
    	while(word[j] != 0)
    	{
    		i = 0;
    		while(word[j]!=0 && word[j]!=' ' && word[j]!='-' && word[j]!='(')
    		{
    			wordPart[i] = word[j];
    			i++;
    			j++;
    		}
    		wordPart[i] = 0;
    
    		if(word[j] == '(')
    			while(word[j++] != ')')
                    ;; //asm nop;
                        
            if(word[j]==0 && !findSth)
                goto PronExitComplexFalse;
            
            wordNoPart = dictGetFirstMatching(GetCurrentFile(appContext), wordPart);
            wordTest = dictGetWord(GetCurrentFile(appContext), wordNoPart);

            if (0 != StrNCaselessCompare(wordTest, wordPart,  (StrLen(wordPart) >= StrLen(wordTest)) ? StrLen(wordPart) : StrLen(wordTest)))
                goto PronExitComplexFalse;
            
            if(!pronGetCompresedWord(appContext,file,compresed,wordNoPart))
                goto PronExitComplexFalse;
            
            if(findSth)
                ebufAddStr(buf,"     ");
            findSth = true;
            
            pronDecomprese(decompresed,compresed);
            pron = pronTranslateDecompresed(appContext, decompresed);
            if(pron!=NULL)
            {
                ebufAddStr(buf,pron);
                new_free(pron);
            }
            else
                goto PronExitComplexFalse;

    		while(word[j]!=0 && (word[j]==' ' || word[j]=='-'))
    			j++;
    	}

        ebufAddChar(buf,']');
        return true;
PronExitComplexFalse:
        while(bufferPosition != buf->used)
            ebufDeleteChar(buf, bufferPosition);
        return false;
    }
      
    pronDecomprese(decompresed,compresed);
    
    pron = pronTranslateDecompresed(appContext, decompresed);
    
    if(pron!=NULL)
    {
        ebufAddChar(buf,'[');
        ebufAddStr(buf,pron);
        new_free(pron);
        ebufAddChar(buf,']');
        return true;
    }   
    else
        return false;
}
