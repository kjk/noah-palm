#include "five_way_nav.h"

void InitFiveWay(AppContext* appContext)
{
    UInt32      value;
    Err         err;

    err = FtrGet( navFtrCreator, navFtrVersion, &value );
    if ( err == errNone )
        AppSetFlag(appContext, appHasFiveWayNav);

    err = FtrGet( sysFileCSystem, sysFtrNumUIHardwareHas5Way, &value );
    if ( err == errNone )
    {
        AppSetFlag(appContext, appHasFiveWayNav);
        err = FtrGet( hsFtrCreator, hsFtrIDNavigationSupported, &value );
        if ( err == errNone )
            AppSetFlag(appContext, appHasHsNav);
    }
}

