/*
  Copyright (C) 2000-2003 Krzysztof Kowalczyk
  Author: szymon knitter (szknitter@wp.pl)
 */

#include <PalmOS.h> 
#include <PceNativeCall.h>
#include "armlet_structures.h"
#include "armlet_runner.h"
#include "noah_pro_rcp.h"

/**
 *  Function from example...
 *  This function runs armcode. It gets *armMainInput as *userDataP!!!
 *  It starts dll on windows simulator or bin-code on palm
 *  Arm code if executes return increased functionID value, so you
 *  can detect if arm is working or not.
 */
static UInt32 armPceNativeResourceCall(DmResType resType, DmResID resID, char* DLLEntryPointP, void *userDataP)
{
	UInt32    processorType;
	MemHandle armH;
	MemPtr    armP;
	UInt32    result;

	// get the processor type
	FtrGet(sysFileCSystem, sysFtrNumProcessorID, &processorType);
	
	if (sysFtrNumProcessorIsARM(processorType))
		{
			// running on ARM; call the ARM resource
			armH = DmGetResource(resType, resID);
			armP = MemHandleLock(armH);
	
			result = PceNativeCall((unsigned long (*)(const void *, void *, unsigned long (*)(const void *, unsigned long, const void *, unsigned long))) armP, userDataP);
	
			MemHandleUnlock(armH);
			DmReleaseResource(armH);
		}
	else if (processorType == sysFtrNumProcessorx86)
		{
			// running on Simulator; call the DLL
			result = PceNativeCall( (NativeFuncType*)DLLEntryPointP, userDataP);
		}
	else
		{
			// some other processor; fail gracefully
            result = -1;
        }
	return result;
}

/**
 *  Test if armlet is working - return true
 */
Boolean armTestArmLet()
{
    armMainInput inpt;
    inpt.functionID = ARM_FUN_TESTIFPRESENT;                         
    inpt.functionData = NULL;                    //first function just tests if arm is working!
	armPceNativeResourceCall(
        			'ARMC',                      // ARMC is a good
					armID, 
					"ARMlet.dll\0NativeFunction", // default dll path is the location of PalmSim.exe
					&inpt);
    return (inpt.functionID == (ARM_FUN_TESTIFPRESENT+ARM_FUN_RETURN_OFFSET))?true:false;
}
/**
 *  Format1/2 on sorted buffer - armlet version
 */
Boolean armFormatonSortedBuffer(ExtensibleBuffer *buf, int format_nr)
{
    armMainInput inpt;
    armFunction10Input funInp;

    funInp.data = buf->data;
    funInp.allocated = buf->allocated;
    funInp.used = buf->used;

    if(format_nr==1)
        inpt.functionID = ARM_FUN_FORMAT1ONBUFF; //format1 on sorted buffer
    else
    if(format_nr==2)
        inpt.functionID = ARM_FUN_FORMAT2ONBUFF; //format2 on sorted buffer
    else
        return false;
            
    inpt.functionData = &funInp;                    
	armPceNativeResourceCall(
        			'ARMC',                      // ARMC is a good
					armID, 
					"ARMlet.dll\0NativeFunction", // default dll path is the location of PalmSim.exe
					&inpt);
 
    buf->allocated = funInp.allocated;
    buf->used = funInp.used;
    buf->data = funInp.data;

    if(format_nr==1)
        return(inpt.functionID == (ARM_FUN_FORMAT1ONBUFF+ARM_FUN_RETURN_OFFSET))?true:false;
    else
        return(inpt.functionID == (ARM_FUN_FORMAT2ONBUFF+ARM_FUN_RETURN_OFFSET))?true:false;
}
/**
 *  get_defs_record - while loop in arm
 */
Boolean armGetDefsRecordWhile(long *current_entry, long *offset, long *curr_len,
                unsigned char **def_lens_fast, unsigned char *def_lens_fast_end, long synsetNoFast)
{
    armMainInput inpt;
    armFunctionGetDefsRecordInput funInp;

    funInp.current_entry = (unsigned long) *current_entry;
    funInp.offset = (unsigned long) *offset;    
    funInp.curr_len = (unsigned long) *curr_len;
    funInp.def_lens_fast = def_lens_fast[0];
    funInp.def_lens_fast_end = def_lens_fast_end;
    funInp.synsetNoFast = (unsigned long) synsetNoFast;    
    funInp.returnValue = 1;
    
    inpt.functionID = ARM_FUN_GETDEFSRECORD; 
    inpt.functionData = &funInp;                    

	armPceNativeResourceCall(
        			'ARMC',                      // ARMC is a good
					armID, 
					"ARMlet.dll\0NativeFunction", // default dll path is the location of PalmSim.exe
					&inpt);
 
    *current_entry = (long) funInp.current_entry;
    *offset = (long) funInp.offset;    
    *curr_len = (long) funInp.curr_len;
    def_lens_fast[0] = funInp.def_lens_fast;

    return(funInp.returnValue == (unsigned long)1)?true:false;
}