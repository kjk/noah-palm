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

#define DRAW_DI_X_P     0
#define DRAW_DI_Y_P     15


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

#define PRON_DATA "AAAEAHAOAWAYB CHD DHEHEREYF G HHIHIYJHK L M N NGOWOYP R S SHT THUHUWV W Y Z ZH"

/**
 *  Version with AA, AO, ...
 */ 
static char* pronTranslateDecompresedVer1(struct _AppContext *appContext, unsigned char *decompressed)
{
    ExtensibleBuffer * buf;
    int                i=0;
    char *             ret;
    char *             tmp;
    int                pos;

    buf = ebufNew();
    if ( NULL == buf)
        return NULL;

    do
    {
        pos = (int)decompressed[i];
        if ( (0==pos) || (pos>39) )
            ebufAddChar(buf, '\0');
        else
        {
            // position of the pronunciation data in PRON_DATA string
            pos = (pos-1)*2;
            Assert( pos <= sizeof(PRON_DATA)-2 );
            tmp = &PRON_DATA[pos];
            if ( ' ' == tmp[1])
                ebufAddStrN(buf,tmp,1);
            else
                ebufAddStrN(buf,tmp,2);
            if(decompressed[i+1] >= 1 && decompressed[i+1] <= 39)
                ebufAddChar(buf,' ');
        }
    } while(decompressed[i++]!=0x00);

    ret = ebufGetTxtCopy(buf);
    ebufDelete(buf);
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
 *  Struct to move in index 
 */
typedef struct
{
    unsigned long   wordNo;
    unsigned int    recordNo;
    unsigned int    offset;
} PrIndex;

/**
 *  Sets prIndex on this one in witch is hidden pron for given wordNo
 */
static void pronGetIndex(PrIndex *prIndex, AbstractFile *file, UInt16 recordWithIndex, long wordNo)
{
    PrIndex *        prIndexNext;
    unsigned char *  data;
    long             offset = 0;
    long             maxOffset;
    
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
    PrIndex         prIndex;
    long            actWord;
    UInt16          recordWithPron;
    unsigned char   i;
    unsigned char * data;

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
    char *      pron;
    char        wordToPrarting[80];
    char        wordPart[WORD_MAX_LEN + 2];
    char *      wordTest;
    int         bufferPosition;
    int         i,j;   //i used with wordPart, j used with word;
    long        wordNoPart;
    Boolean     findSth = false;
    
#ifdef DONT_DO_PRONUNCIATION
    return false;
#endif    
    
    if(!pronGetCompresedWord(appContext,file,compresed,wordNo))
    {
        //complex pronunciation! ignore ' ', '-' and "(p)"
        bufferPosition = buf->used;
        ebufAddChar(buf,'[');
        MemMove(wordToPrarting,word,StrLen(word)+1);

        j = 0;
        while(wordToPrarting[j] != 0)
        {
            i = 0;
            while(wordToPrarting[j]!=0 && wordToPrarting[j]!=' ' && wordToPrarting[j]!='-' && wordToPrarting[j]!='(')
            {
                wordPart[i] = wordToPrarting[j];
                i++;
                j++;
            }
            wordPart[i] = 0;

            if(findSth)
                ebufAddStr(buf,"     ");
    
            if(wordToPrarting[j] == '(')
            {
                findSth = true;
                while(wordToPrarting[j++] != ')')
                    ;; //asm nop;
            }            
            
            if(wordToPrarting[j]==0 && !findSth)
                goto PronExitComplexFalse;
            //this will be very slow!!!            
            wordNoPart = dictGetFirstMatching(GetCurrentFile(appContext), wordPart);
            wordTest = dictGetWord(GetCurrentFile(appContext), wordNoPart);

            if (0 != StrNCaselessCompare(wordTest, wordPart,  (StrLen(wordPart) >= StrLen(wordTest)) ? StrLen(wordPart) : StrLen(wordTest)))
                goto PronExitComplexFalse;
            if(!pronGetCompresedWord(appContext,file,compresed,wordNoPart))
                goto PronExitComplexFalse;
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

            while(wordToPrarting[j]!=0 && (wordToPrarting[j]==' ' || wordToPrarting[j]=='-'))
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
 *  Fill buffer with help definition of phonemNo
 */
static void pronFillHelpBufferWithPhonem(struct _AppContext *appContext, ExtensibleBuffer *buf, int phonemNo)
{
    unsigned char decompresed[10];
    switch(phonemNo)
    {
        case 1:
            //AA	odd	AA D   
            decompresed[0] = 1;
            decompresed[1] = 9;
            decompresed[2] = 0;
            pronFillOneWord(appContext,buf,1, "odd", decompresed);
            break;
        case 2:
            //AE	at	AE T 
            decompresed[0] = 2;
            decompresed[1] = 31;
            decompresed[2] = 0;
            pronFillOneWord(appContext,buf,2, "at", decompresed);
            break;
        case 3:
            //AH	hut	HH AH T 
            decompresed[0] = 16;
            decompresed[1] = 3;
            decompresed[2] = 31;
            decompresed[3] = 0;
            pronFillOneWord(appContext,buf,3, "hut", decompresed);
            break;
        case 4:
            //AO	ought	AO T
            decompresed[0] = 4;
            decompresed[1] = 31;
            decompresed[2] = 0;
            pronFillOneWord(appContext,buf,4, "ought", decompresed);
            break;
        case 5:
            //AW	cow	K AW
            decompresed[0] = 20;
            decompresed[1] = 5;
            decompresed[2] = 0;
            pronFillOneWord(appContext,buf,5, "cow", decompresed);
            break;
        case 6:
            //AY	hide	HH AY D
            decompresed[0] = 16;
            decompresed[1] = 6;
            decompresed[2] = 9;
            decompresed[3] = 0;
            pronFillOneWord(appContext,buf,6, "hide", decompresed);
            break;
        case 7:
            //B	    be	B IY
            decompresed[0] = 7;
            decompresed[1] = 18;
            decompresed[2] = 0;
            pronFillOneWord(appContext,buf,7, "be", decompresed);
            break;
        case 8:
            //CH	cheese	CH IY Z
            decompresed[0] = 8;
            decompresed[1] = 18;
            decompresed[2] = 38;
            decompresed[3] = 0;
            pronFillOneWord(appContext,buf,8, "cheese", decompresed);
            break;
        case 9:
            //D	    dee	D IY
            decompresed[0] = 9;
            decompresed[1] = 18;
            decompresed[2] = 0;
            pronFillOneWord(appContext,buf,9, "dee", decompresed);
            break;
        case 10:
            //DH	thee	DH IY
            decompresed[0] = 10;
            decompresed[1] = 18;
            decompresed[2] = 0;
            pronFillOneWord(appContext,buf,10, "thee", decompresed);
            break;
        case 11:
            //EH	Ed	EH D
            decompresed[0] = 11;
            decompresed[1] = 9;
            decompresed[2] = 0;
            pronFillOneWord(appContext,buf,11, "Ed", decompresed);
            break;
        case 12:
            //ER	hurt	HH ER T
            decompresed[0] = 16;
            decompresed[1] = 12;
            decompresed[2] = 31;
            decompresed[3] = 0;
            pronFillOneWord(appContext,buf,12, "hurt", decompresed);
            break;
        case 13:
            //EY	ate	EY T
            decompresed[0] = 13;
            decompresed[1] = 31;
            decompresed[2] = 0;
            pronFillOneWord(appContext,buf,13, "ate", decompresed);
            break;
        case 14:
            //F	    fee	F IY
            decompresed[0] = 14;
            decompresed[1] = 18;
            decompresed[2] = 0;
            pronFillOneWord(appContext,buf,14, "fee", decompresed);
            break;
        case 15:
            //G	    green	G R IY N
            decompresed[0] = 15;
            decompresed[1] = 28;
            decompresed[2] = 18;
            decompresed[3] = 23;
            decompresed[4] = 0;
            pronFillOneWord(appContext,buf,15, "green", decompresed);
            break;
        case 16:
            //HH	he	HH IY
            decompresed[0] = 16;
            decompresed[1] = 18;
            decompresed[2] = 0;
            pronFillOneWord(appContext,buf,16, "he", decompresed);
            break;
        case 17:
            //IH	it	IH T
            decompresed[0] = 17;
            decompresed[1] = 31;
            decompresed[2] = 0;
            pronFillOneWord(appContext,buf,17, "it", decompresed);
            break;
        case 18:
            //IY	eat	IY T
            decompresed[0] = 18;
            decompresed[1] = 31;
            decompresed[2] = 0;
            pronFillOneWord(appContext,buf,18, "eat", decompresed);
            break;
        case 19:
            //JH	gee	JH IY
            decompresed[0] = 19;
            decompresed[1] = 18;
            decompresed[2] = 0;
            pronFillOneWord(appContext,buf,19, "gee", decompresed);
            break;
        case 20:
            //K 	key	K IY
            decompresed[0] = 20;
            decompresed[1] = 18;
            decompresed[2] = 0;
            pronFillOneWord(appContext,buf,20, "key", decompresed);
            break;
        case 21:
            //L	    lee	L IY
            decompresed[0] = 21;
            decompresed[1] = 18;
            decompresed[2] = 0;
            pronFillOneWord(appContext,buf,21, "lee", decompresed);
            break;
        case 22:
            //M	    me	M IY
            decompresed[0] = 22;
            decompresed[1] = 18;
            decompresed[2] = 0;
            pronFillOneWord(appContext,buf,22, "me", decompresed);
            break;
        case 23:
            //N	    knee	N IY
            decompresed[0] = 23;
            decompresed[1] = 18;
            decompresed[2] = 0;
            pronFillOneWord(appContext,buf,23, "knee", decompresed);
            break;
        case 24:
            //NG	ping	P IH NG
            decompresed[0] = 27;
            decompresed[1] = 17;
            decompresed[2] = 24;
            decompresed[3] = 0;
            pronFillOneWord(appContext,buf,24, "ping", decompresed);
            break;
        case 25:
            //OW	oat	OW T
            decompresed[0] = 25;
            decompresed[1] = 31;
            decompresed[2] = 0;
            pronFillOneWord(appContext,buf,25, "oat", decompresed);
            break;
        case 26:
            //OY	toy	T OY
            decompresed[0] = 31;
            decompresed[1] = 26;
            decompresed[2] = 0;
            pronFillOneWord(appContext,buf,26, "toy", decompresed);
            break;
        case 27:
            //P 	pee	P IY
            decompresed[0] = 27;
            decompresed[1] = 18;
            decompresed[2] = 0;
            pronFillOneWord(appContext,buf,27, "pee", decompresed);
            break;
        case 28:
            //R	    read	R IY D
            decompresed[0] = 28;
            decompresed[1] = 18;
            decompresed[2] = 9;
            decompresed[3] = 0;
            pronFillOneWord(appContext,buf,28, "read", decompresed);
            break;
        case 29:
            //S	    sea	S IY
            decompresed[0] = 29;
            decompresed[1] = 18;
            decompresed[2] = 0;
            pronFillOneWord(appContext,buf,29, "sea", decompresed);
            break;
        case 30:
            //SH	she	SH IY
            decompresed[0] = 30;
            decompresed[1] = 18;
            decompresed[2] = 0;
            pronFillOneWord(appContext,buf,30, "she", decompresed);
            break;
        case 31:
            //T	    tea	T IY
            decompresed[0] = 31;
            decompresed[1] = 18;
            decompresed[2] = 0;
            pronFillOneWord(appContext,buf,31, "tea", decompresed);
            break;
        case 32:
            //TH	theta	TH EY T AH
            decompresed[0] = 32;
            decompresed[1] = 13;
            decompresed[2] = 31;
            decompresed[3] = 3;
            decompresed[4] = 0;
            pronFillOneWord(appContext,buf,32, "theta", decompresed);
            break;
        case 33:
            //UH	hood	HH UH D
            decompresed[0] = 16;
            decompresed[1] = 33;
            decompresed[2] = 9;
            decompresed[3] = 0;
            pronFillOneWord(appContext,buf,33, "hood", decompresed);
            break;
        case 34:
            //UW	two	T UW
            decompresed[0] = 31;
            decompresed[1] = 34;
            decompresed[2] = 0;
            pronFillOneWord(appContext,buf,34, "two", decompresed);
            break;
        case 35:
            //V	    vee	V IY
            decompresed[0] = 35;
            decompresed[1] = 18;
            decompresed[2] = 0;
            pronFillOneWord(appContext,buf,35, "vee", decompresed);
            break;
        case 36:
            //W	    we	W IY
            decompresed[0] = 36;
            decompresed[1] = 18;
            decompresed[2] = 0;
            pronFillOneWord(appContext,buf,36, "we", decompresed);
            break;
        case 37:
            //Y	    yield	Y IY L D
            decompresed[0] = 37;
            decompresed[1] = 18;
            decompresed[2] = 21;
            decompresed[3] = 9;
            decompresed[4] = 0;
            pronFillOneWord(appContext,buf,37, "yield", decompresed);
            break;
        case 38:
            //Z	    zee	Z IY
            decompresed[0] = 38;
            decompresed[1] = 18;
            decompresed[2] = 0;
            pronFillOneWord(appContext,buf,38, "zee", decompresed);
            break;
        case 39:
            //ZH	seizure	S IY ZH ER
            decompresed[0] = 29;
            decompresed[1] = 18;
            decompresed[2] = 39;
            decompresed[3] = 12;
            decompresed[4] = 0;
            pronFillOneWord(appContext,buf,39, "seizure", decompresed);
            break;
    }
}

/**
 *  Fill buffer with help about pronunciation
 */
static void pronFillHelpBuffer(struct _AppContext *appContext, ExtensibleBuffer *buf)
{
    int i = 1;
    for(i=1;i<=39;i++)
        pronFillHelpBufferWithPhonem(appContext, buf, i);    
    ebufAddChar(buf,'\0');
}

/**
 *  Form resizer
 */
static Boolean PronunciationFormDisplayChanged(AppContext* appContext, FormType* frm) 
{
    if ( !DIA_Supported(&appContext->diaSettings) )
        return false;

    UpdateFrmBounds(frm);
    FrmSetObjectPosByID(frm, buttonOk,      -1, appContext->screenHeight-14);
    FrmSetObjectBoundsByID(frm, scrollDef, -1, -1, -1, appContext->screenHeight-18-DRAW_DI_Y_P);
    FrmDrawForm(frm);        
    return true;
}

/**
 *  Draw form buffer
 */
static int pronDrawWord(char *word, int x, int y, int maxX)
{
    int     offset = 0;
    int     fontDY;
    Int16   stringDx = maxX;
    Int16   stringLen;
    Boolean fits = false;
    fontDY = FntCharHeight();

    while(word[offset])
    {
        stringLen = StrLen(&word[offset]);
        FntCharsInWidth(&word[offset], &stringDx, &stringLen, &fits);
    
        if(fits)
        {
            WinDrawChars(&word[offset], stringLen, x, y);
            y += fontDY;
            return y;
        }
        else
        {
            while(word[stringLen + offset] != ' ' && word[stringLen + offset] != ' ' && stringLen > 0)
                stringLen--;

            stringLen++;

            if(stringLen <= 1)
            {
                stringLen = StrLen(&word[offset]);
                FntCharsInWidth(&word[offset], &stringDx, &stringLen, &fits);
            }

            WinDrawChars(&word[offset], stringLen, x, y);
            y += fontDY;
            offset += stringLen;        
        }
    }
    return y;
}

/**
 *  Form filler
 */
static void DrawPronunciationOnForm(AppContext* appContext)
{
    unsigned char compresed[PRON_COMPRESED_MAX_LEN];
    unsigned char decompresed[PRON_DECOMPRESED_MAX_LEN];
    char *      pron;
    char        wordToPrarting[80];
    char        wordPart[WORD_MAX_LEN + 2];
    char *      wordTest;
    int         bufferPosition;
    int         i,j;
    long        wordNoPart;
    Boolean     findSth = false;
    long        wordNo;
    char        *word;
    AbstractFile *file;
    ExtensibleBuffer *buf;
    ExtensibleBuffer *buf2;
    
    buf = ebufNew();
    buf2 = ebufNew();

    file = appContext->currentFile->dictData.wn->file;
    wordNo = appContext->currentWord;
    word = wn_get_word(appContext->currentFile->dictData.wn,wordNo);

    ebufAddChar(buf,FORMAT_TAG);
    ebufAddChar(buf,FORMAT_WORD);
    ebufAddStr(buf,word);
    ebufAddChar(buf,'\n');
    ebufAddChar(buf,FORMAT_TAG);
    ebufAddChar(buf,FORMAT_PRONUNCIATION);
    
    if(!pronGetCompresedWord(appContext,file,compresed,wordNo))
    {
        //complex pronunciation! ignore ' ', '-' and "(p)"
        bufferPosition = buf->used;
        ebufAddChar(buf,'[');
        MemMove(wordToPrarting,word,StrLen(word)+1);

        j = 0;
        while(wordToPrarting[j] != 0)
        {
            i = 0;
            while(wordToPrarting[j]!=0 && wordToPrarting[j]!=' ' && wordToPrarting[j]!='-' && wordToPrarting[j]!='(')
            {
                wordPart[i] = wordToPrarting[j];
                i++;
                j++;
            }
            wordPart[i] = 0;

            if(findSth)
                ebufAddStr(buf,"     ");
    
            if(wordToPrarting[j] == '(')
            {
                findSth = true;
                while(wordToPrarting[j++] != ')')
                    ;; //asm nop;
            }            
            
            //this will be very slow!!!            
            wordNoPart = dictGetFirstMatching(GetCurrentFile(appContext), wordPart);
            wordTest = dictGetWord(GetCurrentFile(appContext), wordNoPart);

            pronGetCompresedWord(appContext,file,compresed,wordNoPart);
            findSth = true;
            
            pronDecomprese(decompresed,compresed);

            for(i=0;decompresed[i]!=0;i++)
                pronFillHelpBufferWithPhonem(appContext, buf2, decompresed[i]);    
                        
            pron = pronTranslateDecompresed(appContext, decompresed);

            ebufAddStr(buf,pron);
            new_free(pron);

            while(wordToPrarting[j]!=0 && (wordToPrarting[j]==' ' || wordToPrarting[j]=='-'))
                j++;
        }

        ebufAddChar(buf,']');
    }
    else
    {      
        pronDecomprese(decompresed,compresed);
        for(i=0;decompresed[i]!=0;i++)
            pronFillHelpBufferWithPhonem(appContext, buf2, decompresed[i]);    
    
        pron = pronTranslateDecompresed(appContext, decompresed);
        ebufAddChar(buf,'[');
        ebufAddStr(buf,pron);
        new_free(pron);
        ebufAddChar(buf,']');
    }

    ebufAddChar(buf,'\n');

    ebufAddStrN(buf,buf2->data,buf2->used);
    ebufDelete(buf2);    

    ebufAddChar(buf,'\0');
    ebufWrapBigLines(buf,false);

    if(appContext->ptrOldDisplayPrefs==NULL)
    {
        /*we use currDispInfo to draw help. We need to restore it before exiting*/
        appContext->ptrOldDisplayPrefs = (void*) appContext->currDispInfo->data;
        //in rawTxt '\n' are replaced with '\0'. we need to replace it
        for(i=0,j=1; i<appContext->currDispInfo->size && j<appContext->currDispInfo->linesCount;i++)
            if(appContext->currDispInfo->data[i] == '\0')
            {
                appContext->currDispInfo->data[i] = '\n';
                j++;
            }
        
        appContext->currDispInfo->data = NULL;
        appContext->currDispInfo->size = 0;
    }
    else
        Assert(0);

   
    diSetRawTxt(appContext->currDispInfo, buf->data);
    appContext->firstDispLine = 0;
//    ClearDisplayRectangle(appContext);
    cbNoSelection(appContext);

    DrawDisplayInfo(appContext->currDispInfo, appContext->firstDispLine, DRAW_DI_X_P, DRAW_DI_Y_P, appContext->dispLinesCount);
    SetScrollbarState(appContext->currDispInfo, appContext->dispLinesCount, appContext->firstDispLine);

    ebufDelete(buf);    
}

/**
 *  Simple handle - only ok button
 */
Boolean PronunciationFormHandleEvent(EventType * event)
{
    FormType *  frm;
    AppContext* appContext;
    frm = FrmGetActiveForm();
    appContext=GetAppContext();
   
    switch (event->eType)
    {
        case winDisplayChangedEvent:
            PronunciationFormDisplayChanged(appContext, frm);
            SetGlobalBackColor(appContext);
            ClearRectangle(DRAW_DI_X_P, DRAW_DI_Y_P, appContext->screenWidth-FRM_RSV_W+2, appContext->screenHeight-FRM_RSV_H-DRAW_DI_Y_P);
            DrawDisplayInfo(appContext->currDispInfo, appContext->firstDispLine, DRAW_DI_X_P, DRAW_DI_Y_P, appContext->dispLinesCount);
            SetScrollbarState(appContext->currDispInfo, appContext->dispLinesCount, appContext->firstDispLine);
            return true;
        case frmOpenEvent:
            cbNoSelection(appContext);
            FrmDrawForm(frm);
            DrawPronunciationOnForm(appContext);
            return true;
        case sclExitEvent:
            if (event->data.sclRepeat.newValue != appContext->firstDispLine)
            {
                SetGlobalBackColor(appContext);
                ClearRectangle(DRAW_DI_X_P, DRAW_DI_Y_P, appContext->screenWidth-FRM_RSV_W+2, appContext->screenHeight-FRM_RSV_H-DRAW_DI_Y_P);
                appContext->firstDispLine = event->data.sclRepeat.newValue;
                DrawDisplayInfo(appContext->currDispInfo, appContext->firstDispLine, DRAW_DI_X_P, DRAW_DI_Y_P, appContext->dispLinesCount);
                SetScrollbarState(appContext->currDispInfo, appContext->dispLinesCount, appContext->firstDispLine);
            }
            return true;
        case ctlSelectEvent:
            switch (event->data.ctlSelect.controlID)
            {
                case buttonOk:
                    FrmReturnToForm(0);
                    //redraw all behind the form                    
                    diSetRawTxt(appContext->currDispInfo,(char*) appContext->ptrOldDisplayPrefs);
                    new_free(appContext->ptrOldDisplayPrefs);
                    appContext->ptrOldDisplayPrefs = NULL;
                    appContext->firstDispLine = 0;
                    cbNoSelection(appContext);
                    ClearDisplayRectangle(appContext);
                    DrawDisplayInfo(appContext->currDispInfo, appContext->firstDispLine, DRAW_DI_X, DRAW_DI_Y, appContext->dispLinesCount);

                    frm = FrmGetActiveForm();
                    FrmSetObjectBoundsByID(frm, scrollDef, -1, -1, -1, appContext->screenHeight-18);
                    SetScrollbarState(appContext->currDispInfo, appContext->dispLinesCount, appContext->firstDispLine);
                   
                    break;
                default:
                    Assert(0);
                    break;
            }
            return true;
        default:
            break;
    }
    return false;
}

/**
 *  Form loader
 */
static Err PronunciationFormLoad(AppContext* appContext)
{
    Err error=errNone;
    FormType* form=FrmInitForm(formPronunciation);
    if (!form)
    {
        error=memErrNotEnoughSpace;
        goto OnError;
    }
    error=DefaultFormInit(appContext, form);
    if (!error)
    {
        FrmSetActiveForm(form);
        FrmSetEventHandler(form, PronunciationFormHandleEvent);
    }
    else 
        FrmDeleteForm(form);
OnError:
    return error;    
}

/**
 *  Display help screen - examples of all 39 phonems
 *  Works like "DisplayHelp(AppContext* appContext)"
 */

void pronDisplayHelp(AppContext* appContext, char* pronHitted)
{
    cbNoSelection(appContext);
    FrmPopupForm(formPronunciation);
}

