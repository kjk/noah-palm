/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: szymon knitter (szknitter@wp.pl)
 */

/*
 * Main structure - armlet code gets on input only one structure pointer
 *  executes one function identified by functionID - functionData contains 
 *  data needed to run function.
 * functionID is used olso as a returned value - if function runs functionID
 *  is set to functionID + 100. It is used to find if function realy runs or not.
 */

//functions ids - as unsigned long
#define ARM_FUN_TESTIFPRESENT   1
#define ARM_FUN_FORMAT2ONBUFF   10
#define ARM_FUN_FORMAT1ONBUFF   11
#define ARM_FUN_GETDEFSRECORD   20
//when function executes id is increased by this value
#define ARM_FUN_RETURN_OFFSET   100
 
//main input structure 
typedef struct _armMainInput
{
    unsigned long functionID; //witch function
    void *functionData;       //data (for test - NULL)
}armMainInput;

//format1/2 on buffer
typedef struct _armFunction10Input
{
    char *data;
    unsigned long allocated;
    unsigned long used;
}armFunction10Input;

//while loop data from get_defs_record (common.c)
typedef struct _armFunctionGetDefsRecordInput
{
    unsigned long current_entry;
    unsigned long offset;
    unsigned long curr_len;
    unsigned char *def_lens_fast;
    unsigned char *def_lens_fast_end;
    unsigned long synsetNoFast;
    unsigned long returnValue;
}armFunctionGetDefsRecordInput;
