/**
 *
 *
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

//	WinDrawChars("TEST RUN",8,10,10);
	// get the processor type
	FtrGet(sysFileCSystem, sysFtrNumProcessorID, &processorType);
	
	if (sysFtrNumProcessorIsARM(processorType))
		{
          	WinDrawChars("ARM  RUN",8,10,20);
			// running on ARM; call the actual ARM resource
			armH = DmGetResource(resType, resID);
			armP = MemHandleLock(armH);
	
			result = PceNativeCall((unsigned long (*)(const void *, void *, unsigned long (*)(const void *, unsigned long, const void *, unsigned long))) armP, userDataP);
	
			MemHandleUnlock(armH);
			DmReleaseResource(armH);
		}
	else if (processorType == sysFtrNumProcessorx86)
		{
//        	WinDrawChars("DLL  RUN",8,10,20);
			// running on Simulator; call the DLL
			result = PceNativeCall( (NativeFuncType*)DLLEntryPointP, userDataP);
		}
	else
		{
			// some other processor; fail gracefully
//          WinDrawChars("NONE RUN",8,10,20);
            result = -1;
        }
	
// 	WinDrawChars("TEST END",8,10,30);
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
 *  Format2 on sorted buffer
 */
Boolean armFormat2onSortedBuffer(ExtensibleBuffer *buf)
{
    armMainInput inpt;
    armFunction10Input funInp;

    funInp.data = buf->data;
    funInp.allocated = buf->allocated;
    funInp.used = buf->used;

    inpt.functionID = ARM_FUN_FORMAT2ONBUFF; //format2 on sorted buffer
    inpt.functionData = &funInp;                    
	armPceNativeResourceCall(
        			'ARMC',                      // ARMC is a good
					armID, 
					"ARMlet.dll\0NativeFunction", // default dll path is the location of PalmSim.exe
					&inpt);
 
    buf->allocated = funInp.allocated;
    buf->used = funInp.used;
    buf->data = funInp.data;
/*
    if(inpt.functionID != (ARM_FUN_FORMAT2ONBUFF+ARM_FUN_RETURN_OFFSET))
    {
       	WinDrawChars("FAIL ARM",8,10,30);
    }
*/
    return(inpt.functionID == (ARM_FUN_FORMAT2ONBUFF+ARM_FUN_RETURN_OFFSET))?true:false;
}