/**
 * @file words_list_form.c
 * Implementation of form that shows list of words obtained from the server.
 */
 
#include "words_list_form.h"
#include "inet_word_lookup.h"
#include "five_way_nav.h"

static Boolean WordsListFormOpen(AppContext* appContext, FormType* form, EventType* event)
{
    Assert(appContext->wordsList);
    Assert(appContext->wordsInListCount);
    UInt16 index=FrmGetObjectIndex(form, listProposals);
    Assert(frmInvalidObjectId!=index);
    ListType* list=static_cast<ListType*>(FrmGetObjectPtr(form, index));
    Assert(list);
    LstSetListChoices(list, appContext->wordsList, appContext->wordsInListCount);

    index=FrmGetObjectIndex(form, fieldWordInput);
    Assert(frmInvalidObjectId!=index);
    FrmSetFocus(form, index);
        
    FrmUpdateForm(formPrefs, frmRedrawUpdateCode);
    return true;
}

static void WordsListFormCleanup(AppContext* appContext)
{
    Assert(appContext->wordsList);
    Assert(appContext->wordsInListCount);
    while (appContext->wordsInListCount)
    {
        Char* word=appContext->wordsList[--appContext->wordsInListCount];
        if (word)
            new_free(word);
    }
    new_free(appContext->wordsList);
    appContext->wordsList=NULL;
}

static Int16 ProposalSearchFunction(const void* searchData, const void* arrayData, Int32 other)
{
    const Char* word=static_cast<const Char*>(searchData);
    const Char* const* item=static_cast<const Char* const*>(arrayData);
    return StrCompare(word, *item);
}

static UInt16 WordsListFormFindClosestProposal(AppContext* appContext, const ListType* list, const Char* word)
{
    UInt16 itemCount=appContext->wordsInListCount;
    const Char* const * items=appContext->wordsList;
    Assert(items);
    Assert(itemCount);
    UInt16 proposal=0;
    if (itemCount)
    {
        Int32 position=0;
        Boolean found=SysBinarySearch(items, itemCount, sizeof(const Char*), ProposalSearchFunction, word, 0, &position, true);
        Assert(position>=0);
        Assert(position<=itemCount);
        if (position==itemCount)
            position--;
        if (position>0)
        {
            const Char* match=LstGetSelectionText(list, position);
            if (StrCompare(match, word)>0)
                position--;
        }
        proposal=position;
    }
    return proposal;
}

static Boolean WordsListFormFieldChanged(AppContext* appContext, FormType* form, EventType* event)
{
    UInt16 index=FrmGetObjectIndex(form, fieldWordInput);
    Assert(index!=frmInvalidObjectId);
    FieldType* field=static_cast<FieldType*>(FrmGetObjectPtr(form, index));
    Assert(field);
    index=FrmGetObjectIndex(form, listProposals);
    Assert(index!=frmInvalidObjectId);
    ListType* list=static_cast<ListType*>(FrmGetObjectPtr(form, index));
    Assert(list);
    const Char* word=FldGetTextPtr(field);
    if (word && *word)
    {
        UInt16 proposal=WordsListFormFindClosestProposal(appContext, list, word);
        LstSetSelection(list, proposal);
    }
    return true;
}

static void ScrollBoundedList(const FormType* form, UInt16 listId, Int16 delta)
{
    Assert(form);
    UInt16 index=FrmGetObjectIndex(form, listId);
    Assert(frmInvalidObjectId!=index);
    ListType* list=static_cast<ListType*>(FrmGetObjectPtr(form, index));
    Assert(list);
    Int16 position=LstGetSelection(list);
    if (noListSelection==position)
        position=LstGetTopItem(list);
    position+=delta;
    if (position<0) 
        position=0;
    else 
    {
        UInt16 itemsCount=LstGetNumberOfItems(list);
        if (position>itemsCount)
            position=itemsCount-1;
    }
    LstSetSelection(list, position);
}

inline static void WordsListFormScrollList(const FormType* form, Int16 delta)
{
    ScrollBoundedList(form, listProposals, delta);
}

static Boolean WordsListFormControlSelected(AppContext* appContext, FormType* form, const EventType* event)
{
    Boolean handled=false;
    Err error=errNone;
    UInt16 controlId = event->data.ctlSelect.controlID;
    switch (controlId)
    {
        case ctlArrowLeft:
            WordsListFormScrollList(form,  -(appContext->dispLinesCount-1));
            handled=true;
            break;                
        
        case ctlArrowRight:
            WordsListFormScrollList(form, (appContext->dispLinesCount-1));
            handled=true;
            break;      
            
        case buttonCancel:
            FrmReturnToForm(0);
            WordsListFormCleanup(appContext);
            handled=true;
            break;
            
        default:
            Assert(false);
            
    }
    return handled;
}

inline static void WordsListFormSelectProposal(AppContext* appContext, UInt16 proposal)
{
    Assert(proposal>=0 && proposal<appContext->wordsInListCount);
    const Char* selectedWord=appContext->wordsList[proposal];
    FrmReturnToForm(0);
    StartWordLookup(appContext, selectedWord);
    WordsListFormCleanup(appContext);
}

inline static Boolean WordsListFormItemSelected(AppContext* appContext, FormType* form, EventType* event)
{
    Assert(event->data.lstSelect.listID=listProposals);
    WordsListFormSelectProposal(appContext, event->data.lstSelect.selection);
    return true;
}

static Boolean WordsListFormKeyDown(AppContext* appContext, FormType* form, EventType* event)
{
    Boolean handled=false;
    switch (event->data.keyDown.chr)
    {
        case returnChr:
        case linefeedChr:
            UInt16 selected=LstGetSelectionByListID(form, listProposals);
            WordsListFormSelectProposal(appContext, selected);
            handled=true;
            break;

        case pageUpChr:
            if ( ! (HaveFiveWay(appContext) && EvtKeydownIsVirtual(event) && IsFiveWayEvent(appContext, event) ) )
            {
                WordsListFormScrollList(form, -(appContext->dispLinesCount-1));
                handled=true;
            }
            break;
        
        case pageDownChr:
            if ( ! (HaveFiveWay(appContext) && EvtKeydownIsVirtual(event) && IsFiveWayEvent(appContext, event) ) )
            {
                WordsListFormScrollList(form, (appContext->dispLinesCount-1));
                handled=true;
            }
            break;

        default:
            if (HaveFiveWay(appContext) && EvtKeydownIsVirtual(event) && IsFiveWayEvent(appContext, event))
            {
                if (FiveWayCenterPressed(appContext, event))
                {
                    UInt16 selected=LstGetSelectionByListID(form, listProposals);
                    WordsListFormSelectProposal(appContext, selected);
                    handled=true;
                }
            
                if (FiveWayDirectionPressed(appContext, event, Left ))
                {
                    WordsListFormScrollList(form, -(appContext->dispLinesCount-1));
                    handled=true;
                }
                if (FiveWayDirectionPressed(appContext, event, Right ))
                {
                    WordsListFormScrollList(form, (appContext->dispLinesCount-1));
                    handled=true;
                }
                if (FiveWayDirectionPressed(appContext, event, Up ))
                {
                    WordsListFormScrollList(form, -1);
                    handled=true;
                }
                if (FiveWayDirectionPressed(appContext, event, Down ))
                {
                    WordsListFormScrollList(form, 1);
                    handled=true;
                }
            }
            else
            {
                FieldType* field=NULL;
                UInt16 index=FrmGetObjectIndex(form, fieldWordInput);
                Assert(index!=frmInvalidObjectId);
                field=static_cast<FieldType*>(FrmGetObjectPtr(form, index));
                Assert(field);
                FldSendChangeNotification(field);
            }                
    }
    return handled;
}    

 
static Boolean WordsListFormHandleEvent(EventType* event)
{
    Boolean handled=false;
    FormType* form=FrmGetFormPtr(formWordsList);
    AppContext* appContext=GetAppContext();
    switch (event->eType)
    {
        case frmOpenEvent:
            handled=WordsListFormOpen(appContext, form, event);
            break;
        
        case ctlSelectEvent:
            handled=WordsListFormControlSelected(appContext, form, event);
            break;
            
        case lstSelectEvent:
            handled=WordsListFormItemSelected(appContext, form, event);
            break;
        
        case fldChangedEvent:
            handled=WordsListFormFieldChanged(appContext, form, event);
            break;
            
        case keyDownEvent:
            handled=WordsListFormKeyDown(appContext, form, event);
            break;
            
    }    
    return handled;
}
 
Err WordsListFormLoad(AppContext* appContext)
{
    Err error=errNone;
    FormType* form=FrmInitForm(formWordsList);
    if (!form)
    {
        error=memErrNotEnoughSpace;
        goto OnError;
    }
    error=DefaultFormInit(appContext, form);
    if (!error)
    {
        FrmSetActiveForm(form);
        FrmSetEventHandler(form, WordsListFormHandleEvent);
    }
    else 
        FrmDeleteForm(form);
OnError:
    return error;    
}
