    /**
     * Number of displayed lines. Replaces DRAW_DI_LINES from common.h to accomodate DynamicInputArea.
     * Should be updated every time display is resized.
     */
    int                 dispLinesCount;
    
    /**
     * Current screen width. Cached so that we don't have to query it every time we need to convert
     * hardcoded coordinates into relative ones.
     */
    int                 screenWidth;

    /**
     * Current screen height.
     * @see screenWidth
     */
    int                 screenHeight;
    
    /**
     * Flags related with system notifications and present features.
     * @see AppFlags in file common.h
     */
    long               flags;

    /**
     * Settings used by DynamicInputArea implementation.
     */
    DIA_Settings diaSettings;
    
    /**
     * Word Matching Pattern cache database.
     */
    DmOpenRef g_wmpDB;
