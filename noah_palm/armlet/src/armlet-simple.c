/******************************************************************************
 *
 *  I M P O R T A N T
 *
 *  If you want to test program on simulator remember to
 *  copy armlet.dll to your simulator directory
 *
 *  You can create dll using noah_palm/armlet/palmsimarmlet.dsp
 *
 *****************************************************************************/

/******************************************************************************
 *
 * File: armlet-simple.c
 *
 * Copyright (C) 2000-2003 Krzysztof Kowalczyk
 * Author: szymon knitter (szknitter@wp.pl)
 *
 * Description:
 *
 * This is changed armlet example.
 * We will use it to build armlet code for our aplications
 * NativeFunction must be first!!!
 * Read more in ../../src/armlet_structures.h
 *
 *****************************************************************************/

/****************************************************
 *
 * Number - Function - return value when executed
 * 1  - Test function - retrun 101
 * 10 - Format2OnSortedBuffer
 * 11 - Format1OnSortedBuffer
 * 20 - WhileLoopFrom Get_defs_record
 *
 ****************************************************/

#include "PceNativeCall.h"
#ifdef WIN32
	#include "SimNative.h"
//    #include <stdio.h>
#endif

unsigned long NativeFunctionAtTheEnd(const void *emulStateP, void *userData68KP, Call68KFuncType *call68KFuncP);

#ifndef WIN32
unsigned long __ARMlet_Startup__(const void *emulStateP, void *userData68KP, Call68KFuncType *call68KFuncP)
#else
unsigned long NativeFunction(const void *emulStateP, void *userData68KP, Call68KFuncType *call68KFuncP)
#endif
{
	return NativeFunctionAtTheEnd(emulStateP, userData68KP, call68KFuncP);
}

#include "../../src/armlet_structures.h"
#include "utils68k.h"


/*FROM THIS POINT YOU CAN ADD YOUR OWN FUNCTIONS*/
void Function1(void *funData)
{            
}

/*Mem allocations*/
#define sysTrapMemPtrNew	0xA013 // we need this in order to call into MemPtrNew
#define sysTrapMemPtrFree	0xA012 // we need this in order to call into MemPtrFree
//const void *emulStatePGLOBAL;
//Call68KFuncType *call68KFuncPGLOBAL;

void new_free(void * ptr,const void *emulStateP,Call68KFuncType *call68KFuncP)
{
	void* ptrSwaped;  // array of Byte
	ptrSwaped = (void *)EndianSwap32(ptr);   
    ((call68KFuncP)(emulStateP, PceNativeTrapNo(sysTrapMemPtrFree), &ptrSwaped, 4 | kPceNativeWantA0));
}

void * new_malloc(unsigned long setSize,const void *emulStateP,Call68KFuncType *call68KFuncP)
{
	unsigned long  size;     // UInt32, must be stack based
	unsigned char* bufferP;  // array of Byte
	// set the size to setSize bytes, byte swapped so it's in big-endian format
	// This will be the argument to MemPtrNew().
	size = EndianSwap32(setSize);   
	// call MemPtrNew, using the 4 bytes of size directly from the stack
	// The 4 specifies the size of the arguments, and we need to OR the size
	// with kPceNativeWantA0 because the return value is a pointer type.
	bufferP = (unsigned char *)((call68KFuncP)(emulStateP, PceNativeTrapNo(sysTrapMemPtrNew), &size, 4 | kPceNativeWantA0));

/*    //we dont need to do it
    setSize--;
    while(setSize >= 0)
    {
        bufferP[setSize--] = 0;
        
    }*/

    return (void *)bufferP;  // return a pointer to the buffer
}

#ifndef WIN32
/*memmove function*/
void memmove(char *dst, char *src, int len)
{
    int i;
    if(dst > src)
    {
        for(i=len-1;i>=0;i--)
            dst[i] = src[i];   
    }
    else
    if(dst < src)
    {
        for(i=0;i<len;i++)
            dst[i] = src[i];    
    }
}

/*strlen function*/
unsigned long strlen(char *str)
{
    unsigned long i = 0;
    while(str[i] != '\0')
        i++;
    return i;
}
#endif

/*strprintstr function <=> sprintf(dst,"%s",src);*/
void strprintstr(char *dst, char *src)
{
    int i = 0;
    while(src[i] != 0)
    {
        dst[i] = (char) src[i];
        i++;
    }
    dst[i] = 0;
}
/*straddstr function - add src to the end of dst*/
/*void straddstr(char *dst, char *src)
{
    unsigned long i,j = 0;
    i = strlen(dst);
    while(src[j]!='\0')
    {
        dst[i] = src[j];
        i++;
        j++;
    }
    dst[i] = 0;
}*/

/*strprintshortint - printf int < 1000 as asci*/
void strprintshortint(char *dst, int number)
{
    int i = 0;
    int j = 13;
    int number2;

    if(number >= 100)
    {
        number2 = number;
        j = 0;
        while(number2 >= 100)
        {
            j++;
            number2 -= 100;        
        }
        dst[i++] = j + '0';
        number = number - j*100;
    }

    if(number >= 10)
    {
        number2 = number;
        j = 0;
        while(number2 >= 10)
        {
            j++;
            number2 -= 10;        
        }
        dst[i++] = j + '0';
        number = number - j*10;
    }
    
        
    dst[i++] = number + '0';
    dst[i] = 0;
}

/* EBUF functions to handle buffer operations*/
/* Extensible buffer for easier construction of string data
   of unknown length */
typedef struct
{
    int    allocated; // how many bytes allocated 
    char * data;     // pointer to data
    int    used;      // how many bytes used (out of all allocated)
    //i dont use it!!! but it is needed
    int    minSize;   // how much memory to pre-allocated upon initialization
} ExtensibleBuffer;

void ebufReplaceChar(ExtensibleBuffer *buf, char c, int pos)
{
    if (pos > buf->used)
        return;
    buf->data[pos] = c;
}

void ebufInsertChar(ExtensibleBuffer *buf, char c, int pos,const void *emulStateP,Call68KFuncType *call68KFuncP)
{
    int newAllocated;
    char *newData;

    // we can only insert up to the last pos in the buffer 
    if (pos > buf->used)
        return;

    if ((buf->used + 1) >= buf->allocated)
    {
        if (0 == buf->allocated)
        {
            newAllocated = 256;
        }
        else
        {
            newAllocated = buf->allocated + 256;
        }
        newData = (char *) new_malloc(newAllocated,emulStateP,call68KFuncP);   
        if (!newData)
            return;
        if (buf->data)
        {
            memmove(newData, buf->data, buf->used);
        }
        buf->allocated = newAllocated;
        if (buf->data)
            new_free(buf->data,emulStateP,call68KFuncP);
        buf->data = newData;
    }
    // make room if inserting 
    if (pos < buf->used)
    {
        memmove(&(buf->data[pos + 1]), &(buf->data[pos]), buf->used - pos);
    }
    buf->data[pos] = c;
    ++buf->used;
}

void ebufAddChar(ExtensibleBuffer *buf, char c,const void *emulStateP,Call68KFuncType *call68KFuncP)
{
    ebufInsertChar(buf, c, buf->used,emulStateP,call68KFuncP);
}

void ebufAddStrN(ExtensibleBuffer *buf, char *str, int strLen,const void *emulStateP,Call68KFuncType *call68KFuncP)
{
    int i;
    for (i = 0; i < strLen; i++)
        ebufInsertChar(buf, str[i], buf->used,emulStateP,call68KFuncP);
}
void ebufAddStr(ExtensibleBuffer * buf, char *str,const void *emulStateP,Call68KFuncType *call68KFuncP)
{
    ebufAddStrN(buf, str, strlen(str),emulStateP,call68KFuncP);
}

//delete one char from buffer
//we will not free memory!!!
void ebufDeleteChar(ExtensibleBuffer *buf, int pos)
{
    if (pos > buf->used || pos < 0)
        return;
    if (pos < buf->used - 1)
        memmove(&(buf->data[pos]), &(buf->data[pos+1]), buf->used - pos - 1);
    buf->used--;
}

//insert string into buf[pos]
void ebufInsertStringOnPos(ExtensibleBuffer *buf, char *string, int pos,const void *emulStateP,Call68KFuncType *call68KFuncP)
{
    int i;
    i = strlen(string) - 1;
    for(; i >= 0; i--)
        ebufInsertChar(buf, string[i], pos,emulStateP,call68KFuncP);
}

//insert "Synonyms: " into buf[pos]
void ebufInsertSynonymsOnPos(ExtensibleBuffer *buf, int pos,const void *emulStateP,Call68KFuncType *call68KFuncP)
{
    ebufInsertChar(buf, ' ', pos,emulStateP,call68KFuncP);
    ebufInsertChar(buf, ':', pos,emulStateP,call68KFuncP);
    ebufInsertChar(buf, 's', pos,emulStateP,call68KFuncP);
    ebufInsertChar(buf, 'm', pos,emulStateP,call68KFuncP);
    ebufInsertChar(buf, 'y', pos,emulStateP,call68KFuncP);
    ebufInsertChar(buf, 'n', pos,emulStateP,call68KFuncP);
    ebufInsertChar(buf, 'o', pos,emulStateP,call68KFuncP);
    ebufInsertChar(buf, 'n', pos,emulStateP,call68KFuncP);
    ebufInsertChar(buf, 'y', pos,emulStateP,call68KFuncP);
    ebufInsertChar(buf, 'S', pos,emulStateP,call68KFuncP);
}


/*Called by Function10*/
//formatting tag code (an unused in database asci code)
//and DisplayPrefs structure
#define FORMAT_TAG        152
#define FORMAT_WORD       153
#define FORMAT_DEFINITION 154
#define FORMAT_EXAMPLE    155
#define FORMAT_POS        156
#define FORMAT_SYNONYM    157  
#define FORMAT_LIST       158  
#define FORMAT_BIG_LIST   159  
#define FORMAT_PRONUNCIATION 160 

// return true if a,b represents a tag
// inline version to make it faster!
static int IsTagInLine(char a, char b)
{
    if(a != (char)FORMAT_TAG)
        return 0;
    switch(b)
    {
        case (char)FORMAT_POS:
        case (char)FORMAT_WORD:
        case (char)FORMAT_DEFINITION: 
        case (char)FORMAT_LIST:
        case (char)FORMAT_BIG_LIST:
        case (char)FORMAT_SYNONYM:
        case (char)FORMAT_EXAMPLE: 
        case (char)FORMAT_PRONUNCIATION: 
            return 1;
        default: 
            return 0;
    }
}              

/*return:   r<0 if pos1 < pos2
            r=0 if pos1 = pos2
            r>0 if pos1 > pos2   */
int  CmpPos(char *pos1, char *pos2)
{
    int i,len1,len2;
    
    i = 1;
    while( !IsTagInLine(pos1[i],pos1[i+1]) && pos1[i+1]!='\0')
        i++;
    len1 = i;
    
    i = 1;
    while( !IsTagInLine(pos2[i],pos2[i+1]) && pos2[i+1]!='\0')
        i++;
    len2 = i;

    i = 0;    
    while(pos1[i] == pos2[i] && i < len1 && i < len2)
        i++;
    //end of pos1 or end of pos2?
    if(i < len1 && i < len2)
    {
        if(pos1[i] < pos2[i])
           return (-1);   
        if(pos1[i] > pos2[i])
           return (1);   
    }
    return(len1 - len2);
}
//delete all text until tag or EOB is reached
static void ebufDeletePos(ExtensibleBuffer *buf, int pos)
{
    while( !IsTagInLine(buf->data[pos],buf->data[pos+1]) && pos < buf->used)
        ebufDeleteChar(buf, pos);
}

/*  Xchg AbcdEef -> EefAbcd   return updated offset2 */
/*            |off1   |off2       |end2              */
/*  before: abCdefghijKlmnopqrstuwXyz                */
/*  after:  abKlmnopqrstuwCdefghijXyz                */
/*                        |off2 - return             */
//old version - in place
int XchgInBuffer(char *txt, int offset1, int offset2, int end2)
{
    int  i;
    char z;
    char *txt1;
    char *txt2;
    int  div2;
    //reverse all
    txt1 = txt + offset1;
    txt2 = txt + end2 - 1;
    
    div2 = (end2-offset1)>>1;
    for(i = 0; i < div2 ; i++)
    {
        z       = txt1[0];
        txt1[0] = txt2[0];
        txt2[0] = z;
        txt1++;
        txt2--;
    }
    //mirror offset2
    i = offset2 - offset1;
    offset2 = end2 - i;
    //reverse 1st
    txt1 = txt + offset1;
    txt2 = txt + offset2 - 1;
    div2 = (offset2-offset1)>>1;
    for(i = 0; i < div2 ; i++)
    {
        z       = txt1[0];
        txt1[0] = txt2[0];
        txt2[0] = z;
        txt1++;
        txt2--;
    }
    //reverse 2nd
    txt1 = txt + offset2;
    txt2 = txt + end2 - 1;
    div2 = (end2-offset2)>>1;
    for(i = 0; i < div2 ; i++)
    {
        z       = txt1[0];
        txt1[0] = txt2[0];
        txt2[0] = z;
        txt1++;
        txt2--;
    }
    return offset2;
}
//word is marked as synonym! and synonyms are marked as words!
//we need to change it... and sort synonyms after definition and examples
//also add "synonyms:" text (but not in thes.).
static void XchgWordsWithSynonyms(ExtensibleBuffer *buf,const void *emulStateP,Call68KFuncType *call68KFuncP)
{
    int  i, j, k;
    int  used;
    char *data;
    char *dataCurr;
    char *dataEnd;

    i = 0;
    if(buf->data[1] == (char)FORMAT_SYNONYM /*|| buf->data[1] == (char)FORMAT_WORD*/)
    {
        buf->data[1] = (char)FORMAT_WORD;
        i = 2;
    }
    else
        return; //we dont have "word" at the begining. This is not Format1 or Format2...

    used = buf->used;
    data = buf->data;
    while(i < used)
    {
        //set i on word tag
        while(i < used && !(data[i]==(char)FORMAT_TAG && data[i+1]==(char)FORMAT_WORD))
            i++;
            
        if(i < used)    
        {
            ebufReplaceChar(buf, FORMAT_SYNONYM, i + 1);
#ifndef THESAURUS   //why? think :)
            //ebufInsertStringOnPos(buf, "Synonyms: ", i + 2,emulStateP,call68KFuncP);
            ebufInsertSynonymsOnPos(buf, i + 2,emulStateP,call68KFuncP);
            used = buf->used;
            data = buf->data;
#endif
        }
        //set j on next tag
        j = i+2;
        dataCurr = data+j;
        dataEnd = data+used;
        while(dataCurr < dataEnd && !IsTagInLine(dataCurr[0],dataCurr[1]))
            dataCurr++;
        j = dataCurr-data;
    
        //some problems with unformated data
        if(!(j+1 < used))
            return; 
        k = j;
        //set k on pos //but not if its reached
        if(data[j+1] != (char)FORMAT_POS)
        {
            k = j+2;
            if(!(k+1 < used))
                return; 
        }    

        dataCurr = data+k;
        while(dataCurr < dataEnd && dataCurr[0]!='\0' && !(dataCurr[0]==(char)FORMAT_TAG && dataCurr[1]==(char)FORMAT_POS))
            dataCurr++;
            
        k = dataCurr - data;    
        //(i,j)(j,k) ---> (j,k)(i,j)
        if(j < used && j!=k)
            XchgInBuffer(data, i, j, k);
        i = k+2;        
    }
}

//format 1
void Format1OnSortedBuffer(ExtensibleBuffer *buf,const void *emulStateP,Call68KFuncType *call68KFuncP)
{
    int  i, j;
    int  first_pos;
    int  number;    
    char str_number[10];

    XchgWordsWithSynonyms(buf,emulStateP,call68KFuncP);

    i = 0;
    while((buf->data[i] != (char)FORMAT_TAG || buf->data[i+1] != (char)FORMAT_POS) && i+1 < buf->used)
        i++;
    
    first_pos = i + 2;
    if(first_pos > buf->used)
        return;   
    i = first_pos;
    number = 1;
    while( i+1 < buf->used )
    {
        if(buf->data[i] == (char)FORMAT_TAG && buf->data[i+1] == (char)FORMAT_POS)
        {
            i += 2;
            
            if(CmpPos(&buf->data[i], &buf->data[first_pos])==0)
            {
                //add numbers to buff (2. 3. 4. etc)
                ebufDeletePos(buf, i);
                number++;                
                strprintshortint(str_number,number);
                j = strlen(str_number);
                str_number[j++] = ':';
                str_number[j++] = ' ';
                str_number[j] = 0;

                ebufReplaceChar(buf, FORMAT_LIST, i - 1);

                j = 0;
                while(str_number[j] != '\0')
                {
                    ebufInsertChar(buf, str_number[j], i + j,emulStateP,call68KFuncP);
                    j++;
                }
                i += j-1;
            }
            else
            {
                //put 1. in first_pos (if its not a single pos type)
                if(buf->data[first_pos] == (char)149)
                {
                    ebufDeleteChar(buf, first_pos);
                    ebufDeleteChar(buf, first_pos);
                    i-=2;
                }

                j = 0;
                while( !IsTagInLine(buf->data[first_pos+j],buf->data[first_pos+j+1]) )
                {
                    if(buf->data[first_pos+j] == ')' || buf->data[first_pos+j]== '(')
                    {
                        ebufDeleteChar(buf, first_pos+j);
                        j--;
                        i--;
                    }
                    j++;                
                }
                j = first_pos + j;

                ebufInsertChar(buf, FORMAT_TAG, j,emulStateP,call68KFuncP);
                ebufInsertChar(buf, FORMAT_BIG_LIST, j + 1,emulStateP,call68KFuncP);
                if(number > 1)
                {
                    ebufInsertChar(buf, '1', j + 2,emulStateP,call68KFuncP);
                    j++;
                    i++;
                }
                ebufInsertChar(buf, ':', j + 2,emulStateP,call68KFuncP);
                ebufInsertChar(buf, ' ', j + 3,emulStateP,call68KFuncP);
                i += 4;
                number = 1;
                first_pos = i;
                i++;
            }

        }
        else
            i++;
    } 
    
    //put 1. in first_pos
    if(buf->data[first_pos] == (char)149)
    {
        ebufDeleteChar(buf, first_pos);
        ebufDeleteChar(buf, first_pos);
    }
    j = 0;
    while( !IsTagInLine(buf->data[first_pos+j],buf->data[first_pos+j+1]) )
    {
        if(buf->data[first_pos+j] == ')' || buf->data[first_pos+j]== '(')
        {
            ebufDeleteChar(buf, first_pos+j);
            j--;
        }
        j++;                
    }
    j = first_pos + j;
    ebufInsertChar(buf, FORMAT_TAG, j,emulStateP,call68KFuncP);
    ebufInsertChar(buf, FORMAT_BIG_LIST, j + 1,emulStateP,call68KFuncP);
    if(number > 1)
        ebufInsertChar(buf, '1', (j++) + 2,emulStateP,call68KFuncP);
    ebufInsertChar(buf, ':', j + 2,emulStateP,call68KFuncP);
    ebufInsertChar(buf, ' ', j + 3,emulStateP,call68KFuncP);

}
/* Print roman in dst */
void strprintroman(char *dst, int roman)
{
    int i = 0;
    dst[i++] = ' ';
    switch(roman)
    {
        case 3: 
                dst[i++] = 'I';
        case 2: 
                dst[i++] = 'I';
        case 1: 
                dst[i++] = 'I';
            break;
        case 4: 
                dst[i++] = 'I';
                dst[i++] = 'V';
            break;
/*        case 5: 
                dst[i++] = 'V';
            break;
        case 6: 
                dst[i++] = 'V';
                dst[i++] = 'I';
            break;
        case 7: 
                dst[i++] = 'V';
                dst[i++] = 'I';
                dst[i++] = 'I';
            break;
        case 8: 
                dst[i++] = 'V';
                dst[i++] = 'I';
                dst[i++] = 'I';
                dst[i++] = 'I';
            break;
        case 9: 
                dst[i++] = 'I';
                dst[i++] = 'X';
            break;*/
        default: break;
    }
    dst[i++] = ' ';
    dst[i++] = 0;
}

/* Format 2 on sorted buffer*/
void Format2onSortedBuffer(ExtensibleBuffer *buf,const void *emulStateP,Call68KFuncType *call68KFuncP)
{       
    int  i, j, p;
    int  first_pos;
    int  number;    
    int  bignumber;    
    char str_number[16];
    char *data;
    char *dataTest;

    XchgWordsWithSynonyms(buf,emulStateP,call68KFuncP);
    
    i = 0;
    while((buf->data[i] != (char)FORMAT_TAG || buf->data[i+1] != (char)FORMAT_POS) && i+1 < buf->used)
        i++;

    first_pos = i + 2;
    if(first_pos > buf->used)
        return;   
    i = first_pos;
    number = 1;
    bignumber = 1;
    
    data = buf->data;
    data += i;
    dataTest = (buf->data) + (buf->used) - 1;
    
    while(data < dataTest)
    {
        if(data[0] == (char)FORMAT_TAG)
        {
            if(data[1] == (char)FORMAT_POS)
            {
                i = buf->used - (dataTest+1-data);
                
                i += 2;
            
                if(CmpPos(&buf->data[i], &buf->data[first_pos])==0)
                {
                    //add numbers to buff (2. 3. 4. etc)
                    ebufDeletePos(buf, i);
                    number++;
                    strprintshortint(str_number,number);
                    p = strlen(str_number);
                    str_number[p++] = ')';
                    str_number[p++] = ' ';
                    str_number[p++] = 0;
                    
                    j = 0;
                    while(str_number[j] != '\0')
                    {
                        ebufInsertChar(buf, str_number[j], i + j,emulStateP,call68KFuncP);
                        j++;
                    }
                    ebufReplaceChar(buf, FORMAT_LIST, i - 1);
                    i += j-1;
                }
                else
                {
                    //put 1) in first_pos (if its not a single pos type)
                    if(buf->data[first_pos] == (char)149)
                    {
                        ebufDeleteChar(buf, first_pos);
                        ebufDeleteChar(buf, first_pos);
                        i-=2;
                    }
    
                    j = first_pos;

                    while( !IsTagInLine(buf->data[j],buf->data[j+1]) )
                    {
                        if(buf->data[j] == ')' || buf->data[j]== '(')
                        {
                            ebufDeleteChar(buf, j);
                            j--;
                            i--;
                        }
                        j++;
                    }
    
                    ebufInsertChar(buf, '\n', j,emulStateP,call68KFuncP);
                    j++;
                    i++;
                                    
                    if(number > 1)
                    {
                        ebufInsertChar(buf, FORMAT_TAG, j,emulStateP,call68KFuncP);
                        ebufInsertChar(buf, FORMAT_LIST , j + 1,emulStateP,call68KFuncP);
                        ebufInsertChar(buf, '1', j + 2,emulStateP,call68KFuncP);
                        ebufInsertChar(buf, ')', j + 3,emulStateP,call68KFuncP);
                        ebufInsertChar(buf, ' ', j + 4,emulStateP,call68KFuncP);
                        i += 5;
                    }
                    
                    str_number[0] = (char) FORMAT_TAG;
                    str_number[1] = (char) FORMAT_BIG_LIST;
                    str_number[2] = 0;
                    strprintroman(&str_number[strlen(str_number)],bignumber);
                    
                    j = 0;
                    while(str_number[j] != 0)
                    {
                        ebufInsertChar(buf, str_number[j], first_pos - 2 + j,emulStateP,call68KFuncP);
                        j++;
                    }
                    i += j-1;
                    
                    i++;
                    number = 1;
                    first_pos = i;
                    i++;
                    bignumber++;
                }
        
                data = buf->data + i;
                dataTest = (buf->data) + (buf->used) - 1;
            }
            else
                data++;            
        }
        else
            data++;
    } 

    //put 1) in first_pos (if its not a single pos type)
    if(buf->data[first_pos] == (char)149)
    {
        ebufDeleteChar(buf, first_pos);
        ebufDeleteChar(buf, first_pos);
    }

    j = first_pos;
    while( !IsTagInLine(buf->data[j],buf->data[j+1]) )
    {
        if(buf->data[j] == ')' || buf->data[j]== '(')
        {
            ebufDeleteChar(buf, j);
            j--;
        }
        j++;                
    }

    //goto end of pos
    j = first_pos + 1;
    while( !IsTagInLine(buf->data[j], buf->data[j+1]) )
        j++;

    ebufInsertChar(buf, '\n', j,emulStateP,call68KFuncP);
    j++;
                                
    if(number > 1)
    {
        ebufInsertChar(buf, FORMAT_TAG, j,emulStateP,call68KFuncP);
        ebufInsertChar(buf, FORMAT_LIST , j + 1,emulStateP,call68KFuncP);
        ebufInsertChar(buf, '1', j + 2,emulStateP,call68KFuncP);
        ebufInsertChar(buf, ')', j + 3,emulStateP,call68KFuncP);
        ebufInsertChar(buf, ' ', j + 4,emulStateP,call68KFuncP);
    }
    
    if(bignumber > 1)
    {    
        str_number[0] = (char) FORMAT_TAG;
        str_number[1] = (char) FORMAT_BIG_LIST;
        str_number[2] = 0;
        strprintroman(&str_number[strlen(str_number)],bignumber);

        j = 0;
        while(str_number[j] != 0)
        {
            ebufInsertChar(buf, str_number[j], first_pos - 2 + j,emulStateP,call68KFuncP);
            j++;
        }
    }
}
/*Fasade on Format2OnSortedBuffer*/
void Function10(void *funData,const void *emulStateP,Call68KFuncType *call68KFuncP)
{
    armFunction10Input *input;
    ExtensibleBuffer buf;
    unsigned long temp;
    input = (armFunction10Input *) funData;
    
    buf.allocated = Read68KUnaligned32(&input->allocated);
    buf.data = (char *) Read68KUnaligned32(&input->data);
    buf.used = Read68KUnaligned32(&input->used);
    buf.minSize = 0;

    
    Format2onSortedBuffer(&buf,emulStateP,call68KFuncP);
        

    Write68KUnaligned32(&input->data,buf.data);
    temp = buf.allocated;
    Write68KUnaligned32(&input->allocated,temp);
    temp = buf.used;
    Write68KUnaligned32(&input->used,temp);
}
/*Fasade on Format1OnSortedBuffer*/
void Function11(void *funData,const void *emulStateP,Call68KFuncType *call68KFuncP)
{
    armFunction10Input *input;
    ExtensibleBuffer buf;
    unsigned long temp;
    input = (armFunction10Input *) funData;
    
    buf.allocated = Read68KUnaligned32(&input->allocated);
    buf.data = (char *) Read68KUnaligned32(&input->data);
    buf.used = Read68KUnaligned32(&input->used);
    buf.minSize = 0;

    
    Format1OnSortedBuffer(&buf,emulStateP,call68KFuncP);
        

    Write68KUnaligned32(&input->data,buf.data);
    temp = buf.allocated;
    Write68KUnaligned32(&input->allocated,temp);
    temp = buf.used;
    Write68KUnaligned32(&input->used,temp);
}
/*while from get_defs_record*/
unsigned long CalculateOffsetGDR(unsigned long *current_entry,unsigned long *offset, unsigned long *curr_len,
                unsigned char **def_lens_fast, unsigned char *def_lens_fast_end,unsigned long synsetNoFast)
{
    do
    {
        (*curr_len) = (unsigned long) ((unsigned char)(def_lens_fast[0])[0]);
        (def_lens_fast[0])++;
        if ((unsigned long)255 == (*curr_len))
        {
            (*curr_len) = (unsigned long)((unsigned char)((def_lens_fast[0])[0])) * 256;
            (def_lens_fast[0])++;
            (*curr_len) += (unsigned long)(unsigned char)((def_lens_fast[0])[0]);
            (def_lens_fast[0])++;
        }

        if ((*current_entry) == synsetNoFast)
            return ((unsigned long) 1);

        (*offset) += (*curr_len);
        
        if(def_lens_fast_end == (def_lens_fast[0]))
            return ((unsigned long) 2);
        (*current_entry)++;
    }
    while (1==1);//true
}

/*runner for CalculateOffsetGDR*/
void FunctionGetDefsRecord(void *funData)
{
    armFunctionGetDefsRecordInput *input = (armFunctionGetDefsRecordInput *) funData;
    unsigned long current_entry = Read68KUnaligned32(&input->current_entry);
    unsigned long offset = Read68KUnaligned32(&input->offset);
    unsigned long curr_len = Read68KUnaligned32(&input->curr_len);
    unsigned char *def_lens_fast;
    unsigned char *def_lens_fast_end;
    unsigned long synsetNoFast = Read68KUnaligned32(&input->synsetNoFast);
    unsigned long returnValue = 0;

    def_lens_fast = (unsigned char *) Read68KUnaligned32(&input->def_lens_fast);
    def_lens_fast_end = (unsigned char *) Read68KUnaligned32(&input->def_lens_fast_end);    

    returnValue = CalculateOffsetGDR(&current_entry, &offset, &curr_len, &def_lens_fast, def_lens_fast_end, synsetNoFast);

    Write68KUnaligned32(&input->current_entry, current_entry);
    Write68KUnaligned32(&input->offset, offset);
    Write68KUnaligned32(&input->curr_len, curr_len);
    Write68KUnaligned32(&input->def_lens_fast, def_lens_fast);
    Write68KUnaligned32(&input->returnValue, returnValue);
}
/*THIS SHOULD BE LAST FUNCTION IN FILE*/
unsigned long NativeFunctionAtTheEnd(const void *emulStateP, void *userData68KP, Call68KFuncType *call68KFuncP)
{
    unsigned long funID;
    armMainInput *inptr;
    inptr = (armMainInput*) userData68KP;

    funID = Read68KUnaligned32(&inptr->functionID);
    switch(funID)
    {
        case ARM_FUN_TESTIFPRESENT: //test function
                Function1((void*)Read68KUnaligned32(&inptr->functionData));
                Write68KUnaligned32(&inptr->functionID, funID+ARM_FUN_RETURN_OFFSET);
            break;
        case ARM_FUN_FORMAT2ONBUFF: //format 2 on sorted buffer
                Function10((void*)Read68KUnaligned32(&inptr->functionData),emulStateP,call68KFuncP);
                Write68KUnaligned32(&inptr->functionID, funID+ARM_FUN_RETURN_OFFSET);
            break;
        case ARM_FUN_FORMAT1ONBUFF: //format 1 on sorted buffer
                Function11((void*)Read68KUnaligned32(&inptr->functionData),emulStateP,call68KFuncP);
                Write68KUnaligned32(&inptr->functionID, funID+ARM_FUN_RETURN_OFFSET);
            break;
        case ARM_FUN_GETDEFSRECORD: //get defs record (while loop)
                FunctionGetDefsRecord((void*)Read68KUnaligned32(&inptr->functionData));
                Write68KUnaligned32(&inptr->functionID, funID+ARM_FUN_RETURN_OFFSET);
            break;    
        default: break;
    }                  
	return (unsigned long)userData68KP;
}
