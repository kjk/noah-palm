#include <stdio.h>
#include <stdlib.h>
//#include <conio.h>
#include <ctype.h>
#include <string.h>
#include <alloc.h>
//#include <dos.h>

#define OFF_AppInfoArea          52
#define OFF_SortInfoArea         56
#define OFF_NumberOfRecords      76
#define OFF_RecordList           78

#define MAX_RecordSize			 65000

typedef unsigned char BB8;
typedef unsigned int  BB16;
typedef unsigned long BB32;

char *baseIN  = "thesbase.pdb";
char *baseOUT = "base.pdb";
char *pronIN  = "c06d";

char *pronBIS  = "PIndxBis.mag";
char *pronRL   = "PRL.mag";
char *pronIND  = "PIndxFil.mag";
char *pronDATA = "PDataFil.mag";

typedef struct
{
	char word[25];
	BB8  pron[32];
} PRONBIS;

typedef struct
{
	char name[32];
	BB16 recNumber;
	BB16 recAdded;
	BB32 additionalOffset;
} NEWFILESTRUCT;

NEWFILESTRUCT newFileStruct;

void Set8(BB8 *val,FILE *in)
{
	fwrite(val,1,1,in);
}

void Set16(BB16 *val,FILE *in)
{
	BB8 x,y;
	x = (((*val)&0x0FF00)>>8);
	y = ((*val)-x*256);
	Set8(&x,in);
	Set8(&y,in);
}

void Set32(BB32 *val,FILE *in)
{
	BB16 x,y;
	x = ((*val)>>16)&0xFFFF;
	y = (*val) - x*65536;
	Set16(&x,in);
	Set16(&y,in);
}

void Get8(BB8 *val,FILE *in)
{
	fread(val,1,1,in);
}

void Get16(BB16 *val,FILE *in)
{
	BB8 x,y;
	Get8(&x,in);
	Get8(&y,in);
	*val = x*256 + y;
}

void Get32(BB32 *val,FILE *in)
{
	BB16 x,y;
	Get16(&x,in);
	Get16(&y,in);
	*val = (BB32)x*65536 + y;
}

typedef struct
{
	char            *packData;
	int             packStringLen[256];
	char            *packString[256];

	unsigned char   *currData;
	long            currOffset;
	long            lastCharInRecordOffset;
	int             currPackStr;
	int             currPackStrLen;
	int             currPackStrUsed;
} PackContext;

PackContext pc;

void pcInit(FILE *in, int recWithComprData)
{
	unsigned char   *dataM;
	unsigned char   *data;
	unsigned char   *dataInt;
	int             maxComprStrLen;
	int             i, j, k;
	long            dataLen;



	fseek(in,OFF_RecordList + 8*recWithComprData,SEEK_SET);
	BB32 bb32;
	BB32 cc32;

	Get32(&bb32,in);

	Get32(&cc32,in);//skip
	Get32(&cc32,in);

	fseek(in,bb32,SEEK_SET);



	dataM = (unsigned char *) malloc(cc32 - bb32);
	data = dataM;

	if(data == NULL)
	{
		printf("malloc failed!!!\n");
		exit(6);
	}


	dataLen = cc32 - bb32;
	bb32 = cc32 - bb32;
	cc32 = 0;

	while(cc32 <= bb32)
		data[cc32++] = fgetc(in);

	dataInt = (unsigned char *) data;

	maxComprStrLen = (int)dataInt[0]*256 + dataInt[1];

	data += 2 + maxComprStrLen * 2;
	dataLen -= 2 + maxComprStrLen * 2;


	pc.packData = (char *) malloc(dataLen);
	if(pc.packData == NULL)
	{
		printf("malloc failed!");
		exit(2);
	}

	memmove(pc.packData, data, dataLen);

	data = (unsigned char *) pc.packData;

	j = 0;
	for (i = 1; i <= maxComprStrLen; i++)
	{
		for (k = 0; k < (dataInt[2*i]*256 + dataInt[2*i+1]); k++)
		{
			pc.packStringLen[j] = i;
			pc.packString[j] = (char *) data;
			data += i;
			++j;
		}
	}

	free(dataM);
	return;
}

unsigned char pcGetChar()
{
	unsigned char code;

	if (pc.currPackStrUsed == pc.currPackStrLen)
	{
		/* FIXME: here we can run into a problem: in the compressor
		   (word-packer-build-word-pdb-data) I mark the end of the db
		   with 0, which should be a code that corresponds to 0. bummer
		 */
		code = pc.currData[pc.currOffset++];
		// TODO: this is a total hack, it fixes the problem with word "gro"
		// (word no 51256 in wn_ex_full but is this really correct?
// line below was the old code and line after that is a fixed (?) thing
//        if (0 == (int) code && (pc->currOffset == pc->lastCharInRecordOffset + 1))
		if (pc.currOffset >= pc.lastCharInRecordOffset + 1)
		{
			pc.currPackStrLen = 0;
			pc.currPackStrUsed = 0;
			return 0;
		}
		pc.currPackStrLen = pc.packStringLen[code];
		pc.currPackStr = (int) code;
		pc.currPackStrUsed = 0;
	}
	code = (pc.packString[pc.currPackStr])[pc.currPackStrUsed];
	++pc.currPackStrUsed;
	return code;
}

void pcUngetChar()
{
	unsigned char code;

	if (pc.currPackStrUsed > 0)
	{
		--pc.currPackStrUsed;
	}
	else
	{
		--pc.currOffset;
		code = pc.currData[pc.currOffset];
		pc.currPackStrLen = pc.packStringLen[code];
		pc.currPackStr = (int) code;
		pc.currPackStrUsed = pc.currPackStrLen - 1;
	}
}

void wcUnpackWord(char *prevWord, char *unpacked)
{
	unsigned char c;

	c = pcGetChar();
	if (c < 32)
	{
		prevWord += c;
		unpacked += c;
		c = pcGetChar();
	}

	while (c > 31)
	{
		*unpacked++ = c;
		c = pcGetChar();
	}
	pcUngetChar();
	*unpacked = 0;
}

int wcGetWord(FILE *in, char * wordIn, BB32 wordNo)
{
	BB16            record;
	BB16            offset;
	BB32            currWord;
	BB32            firstWord;
	char * 			data;

	BB32            bb32,cc32;
	BB16			bb16,cc16;
	BB32            recordSize;

	int				found = 0;
	int				recWithWordCache  = 1;
	int				firstRecWithWords = 4;

	static char 	prevWord[160];

	static int      wasPCINIT = 0;


	if(wasPCINIT == 0)
	{
		pcInit(in, 2);
		wasPCINIT = 1;
	}

	BB32 ch_bb32;
	BB16 ch_bb16;
	BB16 ch_cc16;

	BB16 ch_nextbb16;
	BB16 ch_nextcc16;

	//my
	fseek(in,OFF_RecordList + recWithWordCache * 8,SEEK_SET);
	Get32(&bb32,in);
	Get32(&cc32,in);
	Get32(&cc32,in);

	BB32 cacheEnd = cc32;

	fseek(in,bb32,SEEK_SET);
	found = 0;

	Get32(&ch_bb32,in);
	Get16(&ch_bb16,in);
	Get16(&ch_cc16,in);

	while(found == 0)
	{
		if(ftell(in) < cacheEnd-4)
		{
			Get32(&bb32,in);
			if(bb32 < wordNo)
			{
				ch_bb32 = bb32;
				Get16(&ch_bb16,in);
				Get16(&ch_cc16,in);
			}
			else
				found = 1;
		}
		else
		{
			found = 1;
		}
	}

//	Get16(&bb16,in);
	record = ch_bb16 + firstRecWithWords;
//	Get16(&bb16,in);
	offset = ch_cc16;

	firstWord = ch_bb32;

	//next record next offset

	if(ftell(in) < cacheEnd-4)
	{
		Get16(&bb16,in);
		Get16(&cc16,in);
	}
	else
		ch_nextbb16 = record + 1;

	ch_nextbb16 = bb16 + firstRecWithWords;
	ch_nextcc16 = cc16;

	//
	fseek(in,OFF_RecordList + (BB32)record * 8,SEEK_SET);
	Get32(&bb32,in);

	Get32(&cc32,in);//skip
	Get32(&cc32,in);


	bb32 += (BB32) offset;

	if(record == ch_nextbb16)
		cc32 = bb32 + (BB32) (ch_nextcc16 - offset);


	recordSize = cc32 - bb32;


	fseek(in,bb32,SEEK_SET);

	data = (char *) malloc(recordSize);

	if(data == NULL)
	{
		printf("malloc failed!!!\n");
		printf("wordNo:%lu Coreleft:%lu\n",wordNo,coreleft());
		printf("record:%u offset:%u nextrecord:%u nextoffset:%u\n",record,offset,ch_nextbb16,ch_nextcc16);
		printf("malloc size:%lu\n",recordSize);

		exit(6);
	}



	cc32 = 0;
	while(cc32 < recordSize)
		data[cc32++] = fgetc(in);

	currWord = firstWord;
	prevWord[0] = '\0';

	pc.currData = data;
	pc.currOffset = 0;
	pc.currPackStr = -1;
	pc.currPackStrLen = 0;
	pc.currPackStrUsed = 0;

	pc.lastCharInRecordOffset = recordSize;

	//
	wcUnpackWord(prevWord, prevWord);
	if(currWord != wordNo)
		do
		{
			wcUnpackWord(prevWord, prevWord);
			++currWord;
		}
		while (currWord < wordNo);

	sprintf(wordIn,"%s",prevWord);

	free(data);
	return 0;
}
/************************************/

int	GetWord(FILE *in,char *word,long wordNo)
{
	return wcGetWord(in, word, wordNo);
}

char RetNo(char a, char b)
{
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


	return 0;
	//we dont use strees
	if(a == '0' && b ==  0 ) return 40;
	if(a == '1' && b ==  0 ) return 41;
	if(a == '2' && b ==  0 ) return 42;
}

int  MakeNumbersFromPronunciation(char *we, char *wy)
{
	int i,j,w,wyTmp;

	i = 0;
	w = 0;
	while(we[i] != 0)
	{
		j = i;
		while(we[j] != 0 && we[j] != ' ')
			j++;

		switch(j-i)
		{
			case 1:
				wyTmp   = RetNo(we[i],0);
				if(wyTmp == 0)
					goto ErrorDePron;
				wy[w++] = wyTmp;
				break;
			case 2:
				if(we[i+1]!='0' && we[i+1]!='1' && we[i+1]!='2')
				{
					wyTmp= RetNo(we[i],we[i+1]);
					if(wyTmp == 0)
						goto ErrorDePron;
					wy[w++] = wyTmp;
				}
				else
				{
					wyTmp = RetNo(we[i],0);
					if(wyTmp == 0)
						goto ErrorDePron;
					wy[w++] = wyTmp;
					//wy[w++] = RetNo(we[i+1],0);
				}
				break;
			case 3:
				wyTmp = RetNo(we[i],we[i+1]);
				if(wyTmp == 0)
					goto ErrorDePron;
				wy[w++] = wyTmp;

				//wy[w++] = RetNo(we[i+2],0);
				break;
			default:
				goto ErrorDePron;
				break;
		}

		if(we[j]!=0)
		{
			while(we[j]==' ')
				j++;
			i = j;
		}
		else
			i = j;
	}
	wy[w]=0;
	return 0;
ErrorDePron:

	printf("\n\nError while trying to convert pronunciation to numbers!\n");
	printf("Edit file: %s\n",pronIN);
	printf("Find pronunciation: %s\n",we);
	printf("Pabobly: %c%c%c\n",we[i],we[i+1],we[i+2]);
	return 1;
}

void MakePronBis()
{
	FILE *in, *o1, *o2, *o3;

	char line[200];
	char unpack[100];
	char tmp[200];
	long index = 0;
	int  i,j,k,wordLen;
	PRONBIS pronBisObj;

	in = fopen(pronIN,"rt");
	o2 = fopen(pronBIS,"wb");

	printf("MAKE Pronunciation Sorted Struct file: ");
	fgets(line,200,in);

	while(!feof(in) && strlen(line)>0)
	{
		if(line[0]!='#' && line[1]!='#')
		{
			index++;
			i = 0;

			while(!((line[i]==' ' && line[i+1]==' ') || line[i]=='\n'))
				i++;

			if(line[i]=='\n')
			{
				i = 0;
				while(line[i]!=' ')
					i++;
				wordLen = i;
				i--;
			}
			else
				wordLen = i;
			i += 2;
			//i points at pronunciation begining
			j = i;
			while(line[j]!='\n' && line[j]!='\0')
				j++;
			//j points at pronunciation ending
			for(k=0;k<j-i;k++)
				tmp[k] = line[i+k];
			tmp[k] = 0;


			//pronunciation to numbers
			if(MakeNumbersFromPronunciation(tmp, unpack)==1)
			{
				printf("Find word:%s\n",line);
				printf("Try to fix problem using editor. Good luck!\n");
				exit(1);
			}

			for(k = 0;k < wordLen;k++)
				pronBisObj.word[k] = line[k];
			for(     ;k < sizeof(pronBisObj.word);k++)
				pronBisObj.word[k] = 0;

			for(k = 0;k < strlen(unpack);k++)
				pronBisObj.pron[k] = unpack[k];
			for(     ;k < sizeof(pronBisObj.pron);k++)
				pronBisObj.pron[k] = 0;

			fwrite(&pronBisObj,sizeof(pronBisObj),1,o2);


		}
		fgets(line,200,in);
	}
	printf("done\n");

	fclose(o2);
	fclose(in);
}

/**
 *	return x>0 if w1 >  w2
 *  return x<0 if w1 <  w2
 *  return x=0 if w1 == w2
 */
int CompareWords(char *w1, char *w2)
{
	int i,j,k;


	i = 0;
	while(w1[i] && w2[i] && w1[i]==w2[i])
		i++;

	if(w1[i]!=0 || w2[i]!=0) //not end of one!
	{
		if(w1[i] && w2[i]) //both != 0
		{
			return (int)w1[i]-(int)w2[i];
		}
		else
		if(w1[i]!=0)
		{
			return (int)w1[i];
		}
		else
		{
			return -(int)w2[i];
		}
	}
	else //zero reached in both
	{
		return 0;
	}
}

int GetPron(FILE *pr,char *pron,char *word)
{
	int  i;
	long r,a;
	long L,R,A;
	int  found = 0;

	PRONBIS record;

	if(word[0]==0)
	{
		pron[0] = 0;
		return 0;
	}

	//word to upper
	for(i=0;i<strlen(word);i++)
		word[i] = toupper(word[i]);

	//set 'l' and 'r'
	L = 0L;
	fseek(pr,0L,SEEK_END);
	r = ftell(pr);
	R = r / sizeof(PRONBIS);

	while(found == 0)
	{
		A = (R+L) /2;
		a = A * sizeof(PRONBIS);

		fseek(pr,a,SEEK_SET);
		fread(&record,sizeof(PRONBIS),1,pr);

		i = CompareWords(word,record.word);

		if(i>0)
			L = A + 1;
		else
		if(i<0)
			R = A - 1;


		if(L > R)
			found = 2;

		if(i == 0)
		{
			found = 1;
			strcpy(pron,record.pron);
		}

	}
	if(found == 2)
		pron[0] = 0;

	return found%2;
}

int IsPronComplex(FILE *pr, char* word)
{
	char wordT[60];
	char pronT[60];
	int  i,j;

	j = 0;
	while(word[j] != 0)
	{
		i = 0;
		while(word[j]!=0 && word[j]!=' ' && word[j]!='-' && word[j]!='(')
		{
			wordT[i] = word[j];
			i++;
			j++;
		}
		wordT[i] = 0;

		if(word[j] == '(')
			while(word[j] != ')')
				j++;



		if(GetPron(pr,pronT,wordT)!=1)//cant find
		{
//			printf("test:%s\n",word);
//			printf("testpart:%s\n",wordT);
			return 1;
		}

		while(word[j]!=0 && (word[j]==' ' || word[j]=='-'))
			j++;
	}

//	if(kbhit())
//		exit(1);

	return 0;
}

int PutPron(FILE *pronData,char *pron)
{
	/**
	 * 	We store sth like that: 1B            | XB    | 1B        | XB
	 *                          size of pron1 | pron1 | size of.2 | pron2
	 */

	BB8 zero = 0;
	BB8 i = 0;
	BB8 sizeOfPron;
	BB8 compresed[50];

	BB8 a,b,c,d;
	BB8 A,B,C;
	BB8 m,n;

	i = 0;
	while(pron[i] != 0)
		i++;
	sizeOfPron = i;

	if(sizeOfPron == 0)
	{
		zero = (BB8) 0;
		Set8(&zero,pronData);
		return 0;
	}

	m = 0;
	n = 0;
	i = sizeOfPron;
	while(i >= 4)
	{
		i -= 4;
		a = pron[m++];
		b = pron[m++];
		c = pron[m++];
		d = pron[m++];

		//A,B,C <- a,b,c,d
		A = (a & 0x3F) + ((d & 0x30)<<2);
		B = (b & 0x3F) + ((d & 0x0C)<<4);
		C = (c & 0x3F) + ((d & 0x03)<<6);

		compresed[n++] = A;
		compresed[n++] = B;
		compresed[n++] = C;
	}

	while(i > 0)
	{
		i--;
		compresed[n++] = ((pron[m++])&0x3F);
	}

	//store size
	zero = (BB8) n;
	Set8(&zero,pronData);

	//store compresed pronunciation
	for(i=0;i<n;i++)
	{
		zero = compresed[i];
		Set8(&zero,pronData);
	}
}

int PutIndex(FILE *pronIndex,BB32 wordNo,BB16 recordNo,BB16 pronOffset)
{
	Set32(&wordNo,pronIndex);
	Set16(&recordNo,pronIndex);
	Set16(&pronOffset,pronIndex);

	return 0;
}

int PutRecordList(FILE *offsetsRL, BB32 recordOffset)
{
	BB32 zero = 0;

	Set32(&recordOffset,offsetsRL);
	Set32(&zero        ,offsetsRL);
	return 0;
}

int	MakeFiles()
{
	FILE *in, *o1, *pr;
	FILE *offsetsRL;
	FILE *pronIndex;
	FILE *pronData;
	int  i,j,k;
	BB32 offset;
	BB32 x32;
	BB16 x16;
	BB32 wordsCount;
	BB32 actWord;

	BB32 withoutPron = 0;

	BB32 complexPron = 0;

	in = fopen(baseIN,"rb");
	pr = fopen(pronBIS,"rb");
	o1 = fopen(baseOUT,"wb");
	offsetsRL = fopen(pronRL,"wb");
	pronIndex = fopen(pronIND,"wb");
	pronData  = fopen(pronDATA,"wb");


	if(in == NULL || pr == NULL || o1 == NULL)
	{
		printf("Can't open file!!!\n");

		return 1;
	}


	fseek(in, OFF_NumberOfRecords,SEEK_SET);
	Get16(&x16,in);
	printf("Number of Records:%u\n",x16);

	newFileStruct.recNumber = x16;


	for(i = 0;i<x16;i++)
	{
		fseek(in, OFF_RecordList + i*8,SEEK_SET);
		Get32(&x32,in);

		//if(i < 10)
		printf("Record:%d Offset:%ld\n",i,x32);
	}


	fseek(in, OFF_RecordList, SEEK_SET);
	Get32(&x32,in);
	fseek(in, x32, SEEK_SET);
	Get32(&x32,in);
	wordsCount = x32;

	printf("Words count:%ld\n",x32);


	//main loop:
	BB32 actRecord = -1;  //newFileStruct.recNumber + 1;//1 for index
	char word[60];
	char pron[60];
	BB32 offBefore = 0;
	BB32 offAfter = 0;
	BB32 nextRecord = 0;
	BB32 prevRecord = 0;


	for(actWord = 0; actWord < wordsCount; actWord++)
	{

		/*
		if(kbhit())
			if(getch()==27)
				exit(1);
		 */

		GetWord(in,word,actWord);

		if(GetPron(pr,pron,word)==0)
		{
			withoutPron++;
			//printf("Cant find pronunciation for word: %s\n",word);
			if(IsPronComplex(pr,word)==0)
				complexPron++;
		}


		if(actWord%1000 == 0)
		{
			printf("Word:%lu Coreleft:%lu\n",actWord,coreleft());
		}

		offBefore = ftell(pronData);
		PutPron(pronData,pron);
		offAfter = ftell(pronData);


		if(offAfter > nextRecord)
		{
			prevRecord = offBefore;
			nextRecord += MAX_RecordSize;

			PutRecordList(offsetsRL,prevRecord);
			printf("Record added. RecNo:%ld FirstWordNo:%ld\n",actRecord+1,actWord);

			actRecord++;

			PutIndex(pronIndex,(BB32)actWord,(BB16)actRecord,(BB16)offBefore-prevRecord);
		}
		else
			if(actWord%3000 == 2999)
				PutIndex(pronIndex,(BB32)actWord,(BB16)actRecord,(BB16)offBefore-prevRecord);
	}

	fclose(offsetsRL);
	fclose(pronIndex);
	fclose(pronData);
	fclose(in);
	fclose(pr);
	fclose(o1);

	printf("All records added.\n");

	printf("Words count              : %lu\n", wordsCount);
	printf("Without pron.            : %lu\n", withoutPron);
	printf("With pron. but complex   : %lu\n", complexPron);
	printf("Absolutely without pron. : %lu\n", withoutPron - complexPron);


	return 0;
}

int	MakeEndFile()
{
	printf("Start getting files together: ");

	FILE *in1, *in2, *in3, *in4;
	FILE *out;
	long act,pos;
	int  z;
	char name[32];
	BB32 bb32;
	BB16 bb16,tmp;
	BB32 firstStart,firstEnd;
	long sizeofin1, sizeofin2, sizeofin3;
	long addedoffset = 12;
	char pronunciationID[] = {"pronun######"}; //only 6 first chars

	in1 = fopen(baseIN  ,"rb");
	in2 = fopen(pronRL  ,"rb");
	in3 = fopen(pronIND ,"rb");
	in4 = fopen(pronDATA,"rb");
	out = fopen(baseOUT ,"wb");

	if(in1 == NULL || in2 == NULL || in3 == NULL || in4 == NULL)
	{
		printf("\nCan't open file!!!\n");
		return 1;
	}

	//set sizeofs
	fseek(in1,0L,SEEK_END);
	sizeofin1 = ftell(in1);
	fseek(in1,0L,SEEK_SET);
	fseek(in2,0L,SEEK_END);
	sizeofin2 = ftell(in2);
	fseek(in2,0L,SEEK_SET);
	fseek(in3,0L,SEEK_END);
	sizeofin3 = ftell(in3);
	fseek(in3,0L,SEEK_SET);

	//name
	fread(name,32,1,in1);
	sprintf(&name[strlen(name)]," pron");
	fwrite(name,32,1,out);

	//all to num of records
	act = 32;
	while(act++ < OFF_NumberOfRecords)
		fputc(fgetc(in1),out);


	//new number of records
	Get16(&bb16,in1);
	pos  = bb16;
	tmp  = bb16;
	bb16 += (sizeofin2/8)+1;
	Set16(&bb16,out);

	//update old records list
	while(pos-- > 0)
	{
		Get32(&bb32,in1);
		if(tmp-1 == pos)
			firstStart = bb32;
		if(tmp-2 == pos)
			firstEnd = bb32;

		if(tmp-1 != pos)
		{	//first will be longer!
			bb32 += addedoffset;
		}

		bb32 += sizeofin2 + 8;

		Set32(&bb32,out);
		Get32(&bb32,in1);
		Set32(&bb32,out);
	}

	//add record list element for index
	bb32 = sizeofin1 + sizeofin2 + 8 + addedoffset + 1;
	Set32(&bb32,out);
	bb32 = 0;
	Set32(&bb32,out);

	//add new record list
	act = 0;
	while(act < sizeofin2)
	{
		Get32(&bb32,in2);
		bb32 += sizeofin1 + sizeofin2 + 8 + sizeofin3 + addedoffset + 2;
		Set32(&bb32,out);
		Get32(&bb32,in2);
		Set32(&bb32,out);
		act += 8;
	}

	//write old first record
	act = firstEnd - firstStart;

	while(act-- > 0)
		fputc(fgetc(in1),out);

	for(int i=0;i<6;i++)
		fputc(pronunciationID[i],out);

	bb16 = tmp;           //index record no
	Set16(&bb16,out);
	bb16 = (tmp+1);       //first pronunciation record no
	Set16(&bb16,out);
	bb16 = (sizeofin2/8); //number of pronunciation records
	Set16(&bb16,out);


	//write old records
	while(!feof(in1))
		fputc(fgetc(in1),out);

	//write index
	while(!feof(in3))
		fputc(fgetc(in3),out);

	//write new records
	while(!feof(in4))
		fputc(fgetc(in4),out);


	fclose(in1);
	fclose(in2);
	fclose(in3);
	fclose(in4);
	fclose(out);

	printf("done\n");
	return 0;
}

int  TestIfNoahPro()
{
	FILE *in;

	char test[9];
	int  i;
	int  ret = 0;

	in = fopen(baseIN,"rb");
	fseek(in,(12 + 0x30),SEEK_SET);
	fread(&test,8,1,in);

	//in test we have identificator of dictionary;
	for(i=0;i<8;i++)
		test[i] = toupper(test[i]);
	test[i] = 0;
	//wn20NOAH  yes
	//wnetNOAH  yes/no???
	//rgetTHES  no
	//simpNoAH	no
	//enplNoAH  no

	if(strcmp(&test[4],"NOAH")==0)
	{
		printf("Found NoaH dictionary: %s\n",&test[4]);
		test[4] = 0;
		if(strcmp(test,"WN20")==0)
		{
			printf("Found wn20 dictionary: %s\n",test);
			ret = 2;
		}
		else
		{
			printf("Found dictionary: %s\n",test);
			ret = 1;
		}
	}
	else
	if(strcmp(&test[4],"THES")==0)
	{
		printf("Thes dictionary type: %s not supported.\n",test);
		ret = 0;
	}
	else
	{
		printf("Unknown dictionary type: %s\n",test);
		ret = 0;
	}
	fclose(in);
	return ret;
}

void RemoveFiles()
{
	remove(pronBIS);
	remove(pronRL);
	remove(pronIND);
	remove(pronDATA);
}

int TryOpenFiles()
{
	FILE *in;

	in = fopen(baseIN,"rb");
	if(in == NULL)
	{
		printf("Cant open base:%s",baseIN);
		return 1;
	}
	fclose(in);

	in = fopen(pronIN,"rb");
	if(in == NULL)
	{
		printf("Cant open pronunciation file:%s",pronIN);
		return 1;
	}
	fclose(in);

	in = fopen(baseOUT,"wb");
	if(in == NULL)
	{
		printf("Cant create base:%s",baseOUT);
		return 1;
	}
	fclose(in);
	return 0;
}

void DrawInfo()
{
	printf("Program add pronunciation data to given dictionary.\n");
	printf("To use it type:\n");
	printf("addpron1.exe <pronData> <dictionary file> <output file>\n");
	printf("where:\n");
	printf("<pronData> - pronunciation data file - \"c06d\"\n");
	printf("<dictionary file> - input dictionary (wn20). \"wn_full.pdb\"\n");
	printf("<output file> - name of file to store new dictionary\n");

	printf("\nIMPORTANT: use dirs/filenames under DOS rules! <8.3, without spaces>\n");
}

int main(int argc, char *argv[])
{
	printf("START\n");

	if(argc == 4)
	{
		pronIN  = argv[1];
		baseIN  = argv[2];
		baseOUT = argv[3];
	}
	else
	{
		DrawInfo();
		return 0;
	}

	if(TryOpenFiles()==1)
		return 0;

	if(TestIfNoahPro()<=1)
		return 0;

	MakePronBis();
	MakeFiles();
	MakeEndFile();
	RemoveFiles();
	printf("END\n");
	return 0;
}

