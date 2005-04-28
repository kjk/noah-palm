#ifndef INOAH_STYLES_HPP__
#define INOAH_STYLES_HPP__

// This file defines iNoah-specific static style identifiers

//#define styleNameBold           "bold"
//#define styleNameLarge          "large"
//#define styleNameBlue           "blue"
//#define styleNameGray           "gray"
//#define styleNameRed            "red"
//#define styleNameGreen          "green"
//#define styleNameYellow         "yellow"
//#define styleNameBlack          "black"
//#define styleNameBoldRed        "boldred"
//#define styleNameBoldGreen      "boldgreen"
//#define styleNameBoldBlue       "boldblue"
//#define styleNameLargeBlue      "largeblue"
//#define styleNameSmallGray      "smallgray"

//#define styleNameSmallHeader    "smallheader"
//#define styleNamePageTitle      "pagetitle"

#define styleNameDefinition "definition"
#define styleNameDefinitionList "definitionList"
#define styleNameExample "example"
#define styleNameExampleList "exampleList"
#define styleNamePOfSpeech "partOfSpeech"
#define styleNamePOfSpeechList "partOfSpeechList"
#define styleNameSynonyms "synonyms"
#define styleNameSynonymsList "synonymsList"
#define styleNameWord "word"


enum ElementStyle
{
    styleDefault,
    styleWord,
    styleDefinitionList,
    styleDefinition,
    styleExampleList,
    styleExample,
    styleSynonymsList,
    styleSynonyms,
    stylePOfSpeechList,
    stylePOfSpeech,
	styleCount_
};

enum LayoutType
{
	layoutCompact,
	layoutClassic,
	layoutCount_
};

void ScaleStylesFontSizes(int fontSize);

#endif