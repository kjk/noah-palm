/**
 * @file dynamic_input_area.h
 * Generic interface to DynamicInputArea features present on some new PalmOS 5 devices.
 * @author Andrzej Ciarkowski (a.ciarkowski@interia.pl)
 * @date 2003-11-26
 */
#ifndef __DYNAMIC_INPUT_AREA_H__
#define __DYNAMIC_INPUT_AREA_H__

#include <PalmOS.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Structure that holds settings related with Dynamic Input Area implementation.
 */
typedef struct DIA_Settings_ {

    /**
     * Configuration flags determined by <code>DIA_Init()</code>.
     * @see DIA_Flag
     */
    UInt16 flags;
    
    /**
     * Reference number of Sony SilkLib. 
     */
    UInt16 sonySilkLibRefNum;
    
} DIA_Settings;

/**
 * Flags used with DIA_Settings::flags field.
 */
typedef enum {
    diaHasPenInputMgr,
    diaRegisteredDisplayResizeNotify,
    diaRegisteredDisplayChangeNotify,
    diaHasSonySilkLib,
    diaLoadedSonySilkLib
} DIA_Flag;

/**
 * Initializes DynamicInputArea feature.
 * Detects features available in the system and registers for appropriate notifications.
 * @param diaSettings structure that will be used to store detected configuration settings.
 * @return standard PalmOS error code. In case of error all resources are freed and there's no need to call DIA_Free().
 */
extern Err DIA_Init(DIA_Settings* diaSettings);

/**
 * Frees resources allocated with DIA_Init() and unregisters notifications.
 * @param diaSettings structure filled with a call to DIA_Init().
 * @return standard PalmOS error code. Code other than errNone means that there's a bug in one of functions defined in this API.
 */
extern Err DIA_Free(DIA_Settings* diaSettings);

/**
 * Sends winDisplayChangedEvent. 
 * Should be called in system notification handler for the following notification types:
 *  - sysNotifyDisplayResizedEvent
 *  - sysNotifyDisplayChangeEvent
 */
extern void DIA_HandleResizeEvent();

/**
 * Enables specified form for use with DIA according to current settigns.
 * @param diaSettings current settings as detected by DIA_Init().
 * @param form form that should be enabled.
 * @param minH minimum height that form supports.
 * @param prefH form's preferred height.
 * @param maxH form's maximum height.
 * @param minW minimum width form supports.
 * @param prefW form's preferred width.
 * @param maxW form's maximum width.
 * @return standard PalmOS error code. Code other than errNone means that there's a bug in one of functions defined in this API.
 */
extern Err DIA_FrmEnableDIA(const DIA_Settings* diaSettings, FormType* form, Coord minH, Coord prefH, Coord maxH, Coord minW, Coord prefW, Coord maxW);

#ifdef __cplusplus
}
#endif

/**
 * Tests whether flag is set in <code>DIA_Settings</code> structure.
 * @param diaSettings pointer to (optionally <code>const</code>) <code>DIA_Settings</code> structure.
 * @param flag flag to test.
 * @return nonzero if flag is set.
 * @see DIA_Flag
 */
#define DIA_TestFlag(diaSettings, flag) ((diaSettings)->flags & (1 << flag))

/**
 * Sets flag in <code>DIA_Settings</code> structure.
 * @param diaSettings pointer to <code>DIA_Settings</code> structure.
 * @param flag flag to set.
 * @see DIA_Flag
 */
#define DIA_SetFlag(diaSettings, flag) ((diaSettings)->flags |= (1 << flag))

/**
 * Resets flag in <code>DIA_Settings</code> structure.
 * @param diaSettings pointer to <code>DIA_Settings</code> structure.
 * @param flag flag to reset.
 * @see DIA_Flag
 */
#define DIA_ResetFlag(diaSettings, flag) ((diaSettings)->flags &= ~(1 << flag))

/**
 * Tests if <code>diaHasPenInputMgr</code> flag is set in given <code>DIA_Settings</code> structure.
 * @param diaSettings pointer to <code>DIA_Settings</code> structure.
 */
#define DIA_HasPenInputMgr(diaSettings) DIA_TestFlag(diaSettings, diaHasPenInputMgr)

/**
 * Tests if <code>diaHasSonySilkLib</code> flag is set in given <code>DIA_Settings</code> structure.
 * @param diaSettings pointer to <code>DIA_Settings</code> structure.
 */
#define DIA_HasSonySilkLib(diaSettings) DIA_TestFlag(diaSettings, diaHasSonySilkLib)

/**
 * Tests if dynamic input area features are available.
 * @param diaSettings pointer to <code>DIA_Settings</code> structure.
 */
#define DIA_Supported(diaSettings) (DIA_HasSonySilkLib(diaSettings) || DIA_HasPenInputMgr(diaSettings))

#endif