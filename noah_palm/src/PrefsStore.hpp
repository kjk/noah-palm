/*
  Copyright (C) Krzysztof Kowalczyk
  Owner: Krzysztof Kowalczyk (krzysztofk@pobox.com)

  Helper class for easy storing preferences.
*/

#include "ErrBase.h"
#include "PalmOS.h"

#define psErrDuplicateId psErrorClass+1
#define psErrNoMem       psErrorClass+2

enum PrefItemType {
    pitInt = 0,
    pitStr,
    pitBool
};

typedef struct _prefItem
{
    int                 uniqueId;
    enum PrefItemType   type;
    union {
        int         intVal;
        char *      strVal;
        Boolean     boolVal;
    } value;
} PrefItem;

class PrefsStoreReader
{
private:
    char *  _dbName;
    UInt32  _dbCreator;
    UInt32  _dbType;

public:
    PrefsStoreReader(char *dbName, UInt32 dbCreator, UInt32 dbType);
    Err ErrGetInt(int uniqueId, int *value);
    Err ErrGetStr(int uniqueId, char **vlaue);
    Err ErrGetBool(int uniqueId, Boolean *value);
    ~PrefsStoreReader();
};

#define MAX_PREFS_ITEMS   40

class PrefsStoreWriter
{
private:
    char *      _dbName;
    UInt32      _dbCreator;
    UInt32      _dbType;
    PrefItem    _items[MAX_PREFS_ITEMS];
    int         _itemsCount;

    Err ErrSetItem(PrefItem *item);

public:
    PrefsStoreWriter(char *dbName, UInt32 dbCreator, UInt32 dbType);
    Err ErrSetInt(int uniqueId, int value);
    Err ErrSetStr(int uniqueId, char *value);
    Err ErrSetBool(int uniqueId, Boolean value);
    Err ErrSavePreferences();
    ~PrefsStoreWriter();
};

