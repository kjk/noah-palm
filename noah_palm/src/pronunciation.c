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

#define DONT_DO_PRONUNCIATION 1


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
    unsigned char a,b,c;
    unsigned char A,B,C,D;
    int i,j;
    Boolean end = false;
    
    i = 0;
    j = 0;
    do
    {
        a = compresed[i++];
        if((a & 0x3F)!=0)
            b = compresed[i++];
        else
            b = 0x00;
        if((b & 0x3F)!=0)
            c = compresed[i++];
        else
            c = 0x00;
            
        A = a & 0x3F;
        B = b & 0x3F;
        C = c & 0x3F;
        D = (a & 0xc0)>>2 + (b & 0xc0)>>4 + (c & 0xc0)>>6;
        
        if(!end)
        {
            decompresed[j++] = A;
            if(A==0)
                end = true;        
        }
        if(!end)
        {
            decompresed[j++] = B;
            if(B==0)
                end = true;        
        }
        if(!end)
        {
            decompresed[j++] = C;
            if(C==0)
                end = true;        
        }
        if(!end)
        {
            decompresed[j++] = D;
            if(D==0)
                end = true;        
        }
    }while(!end);
}

/**
 *  Get compresed pronunciation data from dictionary.
 *  Indexed by wordNo.
 *  return true if found else return false if not found.
 */
static Boolean pronGetCompresedWord(struct _AppContext *appContext, unsigned char *compresed, long wordNo)
{
    /*
        THIS IS ONLY TEST !!!
        TODO:!!!!!!!!!!!!!!!!
    */
    unsigned char testData[40];
    int i=0;
    
    switch(wordNo%3)
    {
        case 0:
            testData[0] = 0x01;
            testData[1] = 0x02;
            testData[2] = 0x03;
            testData[3] = 0x00;
            break;
        case 1:
            testData[0] = 0x04;
            testData[1] = 0x99;
            testData[2] = 0x45;
            testData[3] = 0xc0;
            testData[4] = 0x00;
            break;
        case 2:
            testData[0] = 0x01;
            testData[1] = 0x00;
            break;
    }
    
    while(testData[i])
    {
        compresed[i] = testData[i];
        i++;
    }
    if(i>0)
        return true;
    else
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
 */
Boolean pronAddPronunciationToBuffer(struct _AppContext *appContext, ExtensibleBuffer *buf, long wordNo)
{
    unsigned char compresed[PRON_COMPRESED_MAX_LEN];
    unsigned char decompresed[PRON_DECOMPRESED_MAX_LEN];
    char *pron;
    
#ifdef DONT_DO_PRONUNCIATION
    return false;
#endif    
    
    if(!pronGetCompresedWord(appContext,compresed,wordNo))
        return false;
      
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
