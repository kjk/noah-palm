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

#include "PceNativeCall.h"
#ifdef WIN32
	#include "SimNative.h"
#endif

unsigned long NativeFunctionAtTheEnd(const void *emulStateP, void *userData68KP, Call68KFuncType *call68KFuncP);

unsigned long NativeFunction(const void *emulStateP, void *userData68KP, Call68KFuncType *call68KFuncP)
{
	return NativeFunctionAtTheEnd(emulStateP, userData68KP, call68KFuncP);
}

/*FROM THIS POINT YOU CAN ADD YOUR OWN FUNCTIONS*/






/*THIS SHOULD BE LAST FUNCTION IN FILE*/
unsigned long NativeFunctionAtTheEnd(const void *emulStateP, void *userData68KP, Call68KFuncType *call68KFuncP)
{




	return (unsigned long)userData68KP;
}
