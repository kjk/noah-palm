#include <DefinitionStyle.hpp>
#include "iNoahStyles.hpp"

#ifdef _WIN32
#include "sm_inoah.h"
#endif

/*
#define styleNameDefinition "definition"
#define styleNameDefinitionList "definitionList"
#define styleNameExample "example"
#define styleNameExampleList "exampleList"
#define styleNamePOfSpeech "partOfSpeech"
#define styleNamePOfSpeechList "partOfSpeechList"
#define styleNameSynonyms "synonyms"
#define styleNameSynonymsList "synonymsList"
#define styleNameWord "word"
 */

static const StaticStyleDescriptor staticStyleDescriptors[] = {
	{styleNameDefault, "font-size: medium; color: rgb(0, 0, 0); background-color: rgb(100%, 255, 255);"},
	{styleNameHyperlink, "text-decoration: underline; color: rgb(0, 0, 100%);"},



	{styleNameHeader, "font-size: large; font-weight: bold;"},


/*	
	{styleNameBold, "font-weight: bold;"},
	{styleNameBlack, "color: rgb(0, 0, 0);"},
	{styleNameBlue, "color: rgb(0, 0, 100%);"},
	{styleNameBoldBlue, "font-weight: bold; color: rgb(0, 0, 100%);"},
	{styleNameBoldGreen, "font-weight: bold; color: rgb(0, 100%, 0);"},
	{styleNameBoldRed, "font-weight: bold; color: rgb(100%, 0, 0);"},
	{styleNameGray, "color: rgb(67%, 67%, 67%);"},
	{styleNameGreen, "color: rgb(0, 100%, 0);"},
	{styleNameLarge, "font-size: large;"},
	{styleNameLargeBlue, "font-size: large; color: rgb(0, 0, 100%);"},
	{styleNamePageTitle, "font-size: x-large; font-weight: bold;"},
	{styleNameRed, "color: rgb(100%, 0, 0);"},
	{styleNameSmallHeader, "font-weight: bold;"},
	{styleNameStockPriceDown, "font-weight: bold; color: rgb(100%, 0, 0);"},
	{styleNameStockPriceUp, "font-weight: bold; color: rgb(0, 100%, 0);"},
	{styleNameYellow, "color: rgb(100%, 100%, 0);"},
 */
};

static DefinitionStyle* staticStyles[ARRAY_SIZE(staticStyleDescriptors)] = {NULL};


uint_t StyleGetStaticStyleCount()
{
    return ARRAY_SIZE(staticStyles);
}

const char* StyleGetStaticStyleName(uint_t index)
{
    assert(index < ARRAY_SIZE(staticStyleDescriptors));
    return staticStyleDescriptors[index].name;
}

const DefinitionStyle* StyleGetStaticStyle(uint_t index)
{
    assert(index < ARRAY_SIZE(staticStyleDescriptors));
	assert(NULL != staticStyles[index]);
    return staticStyles[index];
}

const DefinitionStyle* StyleGetStaticStyle(const char* name, uint_t length)
{
	return StyleGetStaticStyleHelper(staticStyleDescriptors, ARRAY_SIZE(staticStyleDescriptors), name, length);
}

#ifdef _WIN32
static void ScaleStyleFont(int fontSize, DefinitionStyle& style)
{
	if (style.fontSizeNotDefined == style.fontSize)
		return;

	if (fontSizeSmall == fontSize)
	{
		style.fontSize *= 80;
		style.fontSize /= 100;
	}
	else if (fontSizeLarge == fontSize)
	{
		style.fontSize *= 120;
		style.fontSize /= 100;
	}
}
#endif


void StylePrepareStaticStyles()
{

	int fontSize = GetPrefFontSize();

	assert(NULL == staticStyles[0]);
	for (uint_t i = 0; i < ARRAY_SIZE(staticStyleDescriptors); ++i)
	{
		const char* def = staticStyleDescriptors[i].definition;
		staticStyles[i] = StyleParse(def, strlen(def));

#ifdef _WIN32
		// pre-create default fonts so that they are referenced at least once when used for the 1st time
		ScaleStyleFont(fontSize, *staticStyles[i]);
		staticStyles[i]->font();
#endif

	}

#ifdef DEBUG
	test_StaticStyleTable();
#endif
}

void StyleDisposeStaticStyles()
{
	for (uint_t i = 0; i < ARRAY_SIZE(staticStyleDescriptors); ++i)
	{
		delete staticStyles[i];
		staticStyles[i] = NULL;
	}
}

#ifdef _WIN32
void ScaleStylesFontSizes(int fontSize)
{
	if (NULL == staticStyles[0])
		return;

	for (uint_t i = 0; i < ARRAY_SIZE(staticStyleDescriptors); ++i)
	{
		const char* def = staticStyleDescriptors[i].definition;
		staticStyles[i]->invalidateCachedFont();
		StyleParse(*staticStyles[i], def, strlen(def));
		ScaleStyleFont(fontSize, *staticStyles[i]);
		staticStyles[i]->font();
	}
}
#endif

#ifdef DEBUG
void test_StaticStyleTable()
{
    // Validate that fields_ are sorted.
    for (uint_t i = 0; i < ARRAY_SIZE(staticStyleDescriptors); ++i)
    {
        if (i > 0 && staticStyleDescriptors[i] < staticStyleDescriptors[i-1])
        {
            const char* prevName = staticStyleDescriptors[i-1].name;
            const char* nextName = staticStyleDescriptors[i].name;
            assert(false);
        }
    }
    
    //test |= operator on NULL style
    DefinitionStyle* ptr = NULL;
    DefinitionStyle s = *StyleGetStaticStyle(styleIndexDefault);
    s |= *ptr;
}
#endif
