#include "common.h"
#include "word_compress.h"
#include <ctype.h>

DmOpenRef g_wmpDB;

Boolean TappedInRect(RectangleType * rect)
{
	EventType ev;

	if (EvtEventAvail())
	{
		EvtGetEvent(&ev, 1);
		if (ev.eType == penDownEvent && RctPtInRectangle(ev.screenX, ev.screenY ,rect))
			return true;
	}
	return false;
}

Err OpenMatchingPatternDB()
{
    Err err;
    LocalID dbID;
    char * wmpDBName = "Noah_wmp_cache";

    Assert(StrLen(wmpDBName) < dmDBNameLength);
    
    if (g_wmpDB == NULL)
    {
        dbID = DmFindDatabase(0, wmpDBName);
        if (!dbID)
        {
            err = DmCreateDatabase(0, wmpDBName, 'NoAH', 'data', false);
            if (err) return err;
            dbID = DmFindDatabase(0, wmpDBName);
            if (!dbID) return dmErrCantOpen;
        }
        g_wmpDB = DmOpenDatabase(0, dbID, dmModeReadWrite);
        if (NULL == g_wmpDB)
            return dmErrCantOpen;
    }
    return errNone;
}

Err CloseMatchingPatternDB()
{
	if (g_wmpDB != NULL)
	{
		DmCloseDatabase(g_wmpDB);
		g_wmpDB = NULL;
	}
    return errNone;
}

Err ClearMatchingPatternDB()
{
    Err err;

    while (DmNumRecords(g_wmpDB))
    {
        err = DmRemoveRecord(g_wmpDB, 0);
        if (err) return err;
    }
    return errNone;
}

Err ReadPattern(char * pattern)
{
    MemHandle rh;
    char * rp;

    rh = DmQueryRecord(g_wmpDB, 0);
    if (!rh) {
        pattern[0] = 0;
        return dmErrCantFind;
    }
    rp = MemHandleLock(rh);
    StrCopy(pattern, rp);
    return MemHandleUnlock(rh);
}

// Writes a pattern into the DB at record no 0
Err WritePattern(char * pattern)
{
    Err err;
    MemHandle rh;
    UInt16 len, pos, num;
    char * rp;

    len = StrLen(pattern);
    num = DmNumRecords(g_wmpDB);
    if (num > 0)
        rh = DmGetRecord(g_wmpDB, 0);
    else {
        pos = dmMaxRecordIndex;
        rh = DmNewRecord(g_wmpDB, &pos, len + 1);
    }
    if (!rh) return DmGetLastErr();
    rp = MemHandleLock(rh);
    err = DmWrite(rp, 0, pattern, len + 1);
    MemHandleUnlock(rh);
    DmReleaseRecord(g_wmpDB, 0, true);
    return err;
}

Err ReadMatchingPatternRecord(long pos, long * elem)
{
    MemHandle rh;
    long *rp;

    rh = DmQueryRecord(g_wmpDB, pos / WMP_REC_PACK_SIZE + 1);
    if (!rh) return DmGetLastErr();
    rp = MemHandleLock(rh);
    *elem = rp[pos % WMP_REC_PACK_SIZE];
    return MemHandleUnlock(rh);
}

Err WriteMatchingPatternRecord(long elem)
{
    Err err;
    MemHandle rh;
    long *rp;
    UInt16 pos, offset;
    Int32 num;

    num = DmNumRecords(g_wmpDB);
    if (num > 1)
    {
        pos = num - 1;
        rh = DmGetRecord(g_wmpDB, pos);
        offset = MemHandleSize(rh);
        if (offset < WMP_REC_PACK_SIZE * WMP_REC_SIZE)
            DmResizeRecord(g_wmpDB, pos, offset + WMP_REC_SIZE);
        else
        {
            offset = 0;
            pos = dmMaxRecordIndex;
            rh = DmNewRecord(g_wmpDB, &pos, WMP_REC_SIZE);
        }
    }
    else
    {
        offset = 0;
        pos = dmMaxRecordIndex;
        rh = DmNewRecord(g_wmpDB, &pos, WMP_REC_SIZE);
    }
    if (!rh) return DmGetLastErr();
        
    rp = MemHandleLock(rh);
    err = DmWrite(rp, offset, &elem, WMP_REC_SIZE);
    MemHandleUnlock(rh);
    DmReleaseRecord(g_wmpDB, pos, true);
    return err;
}

long NumMatchingPatternRecords()
{
    UInt16 num;
    long count;
    MemHandle rh;
    
    num = DmNumRecords(g_wmpDB);
    if (num > 1)
    {
        rh = DmQueryRecord(g_wmpDB, num - 1);
        // full record packs = all records - first record (pattern's there) - last record (doesn't have to be full)
        // so 2 records are definetely not full
        count = (num - 2) * WMP_REC_PACK_SIZE;
        count += MemHandleSize(rh) / WMP_REC_SIZE;
        return count;
    }
    return 0;
}

// returns 1 if word matches given pattern, 0 otherwise
int WordMatchesPattern(char * word, char * pattern)
{
    char c;

    switch (c = *pattern++)
    {
        case '?':
            if (*word == '\0')
                return 0;
            return WordMatchesPattern(++word, pattern);

        case '*':
            while (*pattern == '*')
                pattern++;
            if (*pattern == '\0')
                return 1;
            while (*++word != '\0')
                if (WordMatchesPattern(word, pattern))
                    return 1;
            return 0;

        default:
            // I used tolower from ctype.h, because char_to_lower from word_compress.h didn't work (WHY?)
            if (tolower(c) == tolower(*word))
            {
                if (c == '\0')
                    return 1;
                return WordMatchesPattern(++word, pattern);
            }
            return 0;
    }
}

void FillMatchingPatternDB(char * pattern)
{
    char tmp[5];
    RectangleType rc = {{30, 40}, {100, 60}};
    RectangleType rcStop = {{60, 80}, {40, 12}};
    long i, num;
    char * str;

    num = dictGetWordsCount();
    WinEraseRectangle(&rc, 7);
    WinDrawRectangleFrame(roundFrame, &rc);
    DrawCenteredString(SEARCH_TXT, rc.topLeft.y + 5);
    WinDrawRectangleFrame(roundFrame, &rcStop);
    DrawCenteredString("Stop", rcStop.topLeft.y);
    if (pattern[0] != '*' && pattern[0] != '?')
    {
        // special case: we can optimize the searching

        UInt16 len, tmpLen;
        char * tmpPattern;  // prefix of the pattern (beginning of the pattern until '*' or '?')
        long pos;

        len = StrLen(pattern);
        tmpPattern = (char *) new_malloc(len + 1);
        for (i = 0; i < len; i++)
        {
            if (pattern[i] != '*' && pattern[i] != '?')
                tmpPattern[i] = pattern[i];
            else
                break;
        }
        tmpPattern[i] = 0;
        tmpLen = StrLen(tmpPattern);
        pos = dictGetFirstMatching(tmpPattern);
        str = dictGetWord(pos);
        while (1)
        {
            // word prefix has changed and is not fitting to pattern, we quit
            if (StrNCaselessCompare(str, tmpPattern, tmpLen)) break;
            if (WordMatchesPattern(str, pattern))
                WriteMatchingPatternRecord(pos);
            StrPrintF(tmp, "%d%%", (int)((long)(pos * 100) / (long)num));
            DrawCenteredString(tmp, rc.topLeft.y + 20);
            if (TappedInRect(&rcStop)) break;
            str = dictGetWord(++pos);
        }
        new_free(tmpPattern);
    }
    else
    {
        // we need to iterate through the whole database
        
        for (i = 0; i < num; i++)
        {
            str = dictGetWord(i);
            if (WordMatchesPattern(str, pattern))
                WriteMatchingPatternRecord(i);
            StrPrintF(tmp, "%d%%", (int)((long)(i * 100) / (long)num));
            DrawCenteredString(tmp, rc.topLeft.y + 20);
            if (TappedInRect(&rcStop)) break;
        }
    }
}

void PatternListDrawFunc(Int16 itemNum, RectangleType * bounds, char **data)
{
    char        *str;
    Int16       stringWidthP = 160; /* max width of the string in the list selection window */
    Int16       stringLenP;
    Boolean     truncatedP = false;
    long        realItemNo;

    Assert(itemNum >= 0);

    if (ReadMatchingPatternRecord(itemNum, &realItemNo) == errNone)
    {
        str = dictGetWord(realItemNo);
        stringLenP = StrLen(str);

        FntCharsInWidth(str, &stringWidthP, &stringLenP, &truncatedP);
        WinDrawChars(str, stringLenP, bounds->topLeft.x, bounds->topLeft.y);
    }
}
