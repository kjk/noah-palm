/******************************************************************************
 *
 * File: armlet-simple.c
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
 * 1 - Test function - retrun 101
 *
 *
 ****************************************************/

#include "PceNativeCall.h"
#ifdef WIN32
	#include "SimNative.h"
	#include <stdio.h>
#endif

#include "../../src/armlet_structures.h"
#include "utils68k.h"

unsigned long NativeFunctionAtTheEnd(const void *emulStateP, void *userData68KP, Call68KFuncType *call68KFuncP);

#ifndef WIN32
unsigned long __ARMlet_Startup__(const void *emulStateP, void *userData68KP, Call68KFuncType *call68KFuncP)
#else
unsigned long NativeFunction(const void *emulStateP, void *userData68KP, Call68KFuncType *call68KFuncP)
#endif
{
	return NativeFunctionAtTheEnd(emulStateP, userData68KP, call68KFuncP);
}


#ifndef WIN32

#include <stdio.h>
#include <string.h>

/*
void memmove(char *dst,char *src,int n)
{

}

int strlen(char *str)
{

}
*/
#endif
/*FROM THIS POINT YOU CAN ADD YOUR OWN FUNCTIONS*/
void Function1(void *funData)
{            
}

/*Mem allocations*/
#define sysTrapMemPtrNew	0xA013 // we need this in order to call into MemPtrNew
#define sysTrapMemPtrFree	0xA012 // we need this in order to call into MemPtrFree
const void *emulStatePGLOBAL;
Call68KFuncType *call68KFuncPGLOBAL;

void new_free(void * ptr)
{
	void* ptrSwaped;  // array of Byte
	ptrSwaped = (void *)EndianSwap32(ptr);   
    ((call68KFuncPGLOBAL)(emulStatePGLOBAL, PceNativeTrapNo(sysTrapMemPtrFree), &ptrSwaped, 4 | kPceNativeWantA0));
}

void * new_malloc(unsigned long setSize)
{
	unsigned long  size;     // UInt32, must be stack based
	unsigned char* bufferP;  // array of Byte
	// set the size to setSize bytes, byte swapped so it's in big-endian format
	// This will be the argument to MemPtrNew().
	size = EndianSwap32(setSize);   
	// call MemPtrNew, using the 4 bytes of size directly from the stack
	// The 4 specifies the size of the arguments, and we need to OR the size
	// with kPceNativeWantA0 because the return value is a pointer type.
	bufferP = (unsigned char *)((call68KFuncPGLOBAL)(emulStatePGLOBAL, PceNativeTrapNo(sysTrapMemPtrNew), &size, 4 | kPceNativeWantA0));

    return (void *)bufferP;  // return a pointer to the buffer
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

void ebufInsertChar(ExtensibleBuffer *buf, char c, int pos)
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
        newData = (char *) new_malloc(newAllocated);   
        if (!newData)
            return;
        if (buf->data)
        {
            memmove(newData, buf->data, buf->used);
        }
        buf->allocated = newAllocated;
        if (buf->data)
            new_free(buf->data);
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

void ebufAddChar(ExtensibleBuffer *buf, char c)
{
    ebufInsertChar(buf, c, buf->used);
}

void ebufAddStrN(ExtensibleBuffer *buf, char *str, int strLen)
{
    int i;
    for (i = 0; i < strLen; i++)
        ebufInsertChar(buf, str[i], buf->used);
}
void ebufAddStr(ExtensibleBuffer * buf, char *str)
{
    ebufAddStrN(buf, str, strlen(str));
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
void ebufInsertStringOnPos(ExtensibleBuffer *buf, char *string, int pos)
{
    int i;
    i = strlen(string) - 1;
    for(; i >= 0; i--)
        ebufInsertChar(buf, string[i], pos);
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
    //reverse all
    txt1 = txt + offset1;
    txt2 = txt + end2 - 1;
    for(i = 0; i < (end2-offset1)/2 ; i++)
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
    for(i = 0; i < (offset2-offset1)/2 ; i++)
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
    for(i = 0; i < (end2-offset2)/2 ; i++)
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
static void XchgWordsWithSynonyms(ExtensibleBuffer *buf)
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
            ebufInsertStringOnPos(buf, "Synonyms: ", i + 2);
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

/* Format 2 on sorted buffer*/
void Format2onSortedBuffer(ExtensibleBuffer *buf)
{       
    int  i, j;
    int  first_pos;
    int  number;    
    int  bignumber;    
    char str_number[10];
    char *roman[10];  // = {""," I"," II"," III"," IV"," V"," VI"," VII"," VIII"," IX"};
    char *data;
    char *dataTest;

    //initialize data (resident mode)
    roman[0] = " ";
    roman[1] = " I";
    roman[2] = " II";
    roman[3] = " III";
    roman[4] = " IV";
    roman[5] = " V";
    roman[6] = " VI";
    roman[7] = " VII";
    roman[8] = " VIII";
    roman[9] = " IX";
  
    XchgWordsWithSynonyms(buf);

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
                    sprintf(str_number,"%d) \0",number);
                    
                    j = 0;
                    while(str_number[j] != '\0')
                    {
                        ebufInsertChar(buf, str_number[j], i + j);
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
    
                    ebufInsertChar(buf, '\n', j);
                    j++;
                    i++;
                                    
                    if(number > 1)
                    {
                        ebufInsertChar(buf, FORMAT_TAG, j);
                        ebufInsertChar(buf, FORMAT_LIST , j + 1);
                        ebufInsertChar(buf, '1', j + 2);
                        ebufInsertChar(buf, ')', j + 3);
                        ebufInsertChar(buf, ' ', j + 4);
                        i += 5;
                    }
                    
                    sprintf(str_number,"%c%c%s \0", FORMAT_TAG, FORMAT_BIG_LIST, roman[bignumber%10]);
                    j = 0;
                    while(str_number[j] != '\0')
                    {
                        ebufInsertChar(buf, str_number[j], first_pos - 2 + j);
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

    ebufInsertChar(buf, '\n', j);
    j++;
                                
    if(number > 1)
    {
        ebufInsertChar(buf, FORMAT_TAG, j);
        ebufInsertChar(buf, FORMAT_LIST , j + 1);
        ebufInsertChar(buf, '1', j + 2);
        ebufInsertChar(buf, ')', j + 3);
        ebufInsertChar(buf, ' ', j + 4);
    }
    
    if(bignumber > 1)
    {    
        sprintf(str_number,"%c%c%s \0", FORMAT_TAG, FORMAT_BIG_LIST, roman[bignumber%10]);                     
        j = 0;
        while(str_number[j] != '\0')
        {
            ebufInsertChar(buf, str_number[j], first_pos - 2 + j);
            j++;
        }
    }
}

void Function10(void *funData)
{
    armFunction10Input *input;
    ExtensibleBuffer buf;
    unsigned long temp;
    input = (armFunction10Input *) funData;
    
    buf.allocated = Read68KUnaligned32(&input->allocated);
    buf.data = (char *) Read68KUnaligned32(&input->data);
    buf.used = Read68KUnaligned32(&input->used);
    buf.minSize = 0;

    
    Format2onSortedBuffer(&buf);
        

    Write68KUnaligned32(&input->data,buf.data);
    temp = buf.allocated;
    Write68KUnaligned32(&input->allocated,temp);
    temp = buf.used;
    Write68KUnaligned32(&input->used,temp);
}
/*THIS SHOULD BE LAST FUNCTION IN FILE*/
unsigned long NativeFunctionAtTheEnd(const void *emulStateP, void *userData68KP, Call68KFuncType *call68KFuncP)
{
    unsigned long funID;
    armMainInput *inptr;
    call68KFuncPGLOBAL = call68KFuncP;
    emulStatePGLOBAL = emulStateP;
    inptr = (armMainInput*) userData68KP;

    funID = Read68KUnaligned32(&inptr->functionID);
    switch(funID)
    {
        case 1: //test function
                Function1((void*)Read68KUnaligned32(&inptr->functionData));
                Write68KUnaligned32(&inptr->functionID, funID+100);
            break;
        case 10: //format 2 on sorted buffer
                Function10((void*)Read68KUnaligned32(&inptr->functionData));
                Write68KUnaligned32(&inptr->functionID, funID+100);
            break;
        default: break;
    }                  
	return (unsigned long)userData68KP;
}
