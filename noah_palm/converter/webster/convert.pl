############################################################################
require 5.001;
############################################################################

$WEBSTER_FILES_PATH = "C:\\kjk\\src\\mine\\dicts_data\\webster\\";

sub wb_full_path
{
	$file = shift;
	$path = $WEBSTER_FILES_PATH . $file;
	return $path;
}

############################################################################
# Find some bugs in dictionary
#
sub StripFromSpecials       #2004-02-15 14:14
############################################################################
{
        $str = shift;
        #############################################
        #   Delete all guide to pronunciation bracers
        if ($str =~ m:\((.*?)Guide to Pronunciation(.*?)\):)
        {
            $str =~ s/\((.*?)Guide to Pronunciation(.*?)\)/ /g;

        }

        #############################################
        #   Handle <pos><i>imp. & p. p.</i></pos>
        $str =~ s/<pos><i>imp. & p. p.<\/i><\/pos>/imp. p. p./g;

        #############################################
        #   Handle (Bot. & Zo&ouml;l.)
        $str =~ s/\(Bot. & Zo&ouml;l.\)/\(Bot. Zo&ouml;l.\)/g;

        #############################################
        #   Handle <pos><i>a. & p. p.</i></pos>
        $str =~ s/<pos><i>a. & p. p.<\/i><\/pos>/a. p. p./g;

        #############################################
        #   Handle <i>Beau. & Fl.</i> <sd><i>(c)</i></sd>
        $str =~ s/<i>Beau. & Fl.<\/i> <sd><i>\(c\)<\/i><\/sd>/Beau. Fl. <sd>\(c\)<\/sd>/g;

        return $str;
}   ##StripFromSpecials
############################################################################
# Remove * " | ' ` from str
#
sub StripFromStars      #2004-02-15 14:42
############################################################################
{
        $str = shift;
        #############################################
        #   ' " * | etc.

#       $str =~ s/\'//g;
        $str =~ s/\*//g;
        $str =~ s/\"//g;
        $str =~ s/\|//g;
        $str =~ s/\`//g;

        return $str;
}   ##StripFromStars
############################################################################
# Deletes all umpersands from string
#
sub StripFromAmpersands     #2004-02-12 17:06
############################################################################
{
        $str = shift;
        ###################
        # Accepted by Boss



        ####################
        # Defined ampersands

        # 'e 'a   ' = /
        $str =~ s/&eacute;/e/g;
        $str =~ s/&Eacute;/E/g;
        $str =~ s/&aacute;/a/g;
        $str =~ s/&oacute;/o/g;
        # `a `u   ` = \
        $str =~ s/&egrave;/e/g;
        $str =~ s/&agrave;/a/g;
        $str =~ s/&ugrave;/u/g;
        # AE OE
        $str =~ s/&aelig;/ae/g;
        $str =~ s/&oelig;/oe/g;
        $str =~ s/&OElig;/OE/g;
        $str =~ s/&AElig;/AE/g;
        # c, C,
        $str =~ s/&ccedil;/c/g;
        $str =~ s/&Ccedil;/C/g;
        # ^a  ^u
        $str =~ s/&acirc;/a/g;
        $str =~ s/&ecirc;/e/g;
        $str =~ s/&icirc;/i/g;
        $str =~ s/&ocirc;/o/g;
        $str =~ s/&ucirc;/u/g;
        # :a :u  ( "a  "u )
        $str =~ s/&Auml;/A/g;
        $str =~ s/&auml;/a/g;
        $str =~ s/&euml;/e/g;
        $str =~ s/&iuml;/i/g;
        $str =~ s/&ouml;/o/g;
        $str =~ s/&uuml;/u/g;
        $str =~ s/&umlaut;/\"/g;
        # ~n ~l
        $str =~ s/&ntilde;/n/g;
        $str =~ s/&ltilde;/l/g;
        $str =~ s/&atilde;/a/g;
        #fractures defined in html
        $str =~ s/&frac12;/ 1\/2/g;
        $str =~ s/&frac14;/ 1\/4/g;
        $str =~ s/&frac34;/ 3\/4/g;
        #fractures undefined in html
        $str =~ s/&frac13;/ 1\/3/g;
        $str =~ s/&frac15;/ 1\/5/g;
        $str =~ s/&frac17;/ 1\/7/g;
        $str =~ s/&frac19;/ 1\/9/g;
        $str =~ s/&frac58;/ 5\/8/g;
        $str =~ s/&frac38;/ 3\/8/g;
        $str =~ s/&frac36;/ 1\/2/g;
        $str =~ s/&frac56;/ 5\/6/g;
        $str =~ s/&frac23;/ 2\/3/g;
        $str =~ s/&frac1x10;/ 1x10/g;
        $str =~ s/&frac1x6000;/ 1x6000/g;
        $str =~ s/&frac1x50000;/ 1x50000/g;
        $str =~ s/&frac1x29966;/ 1x29966/g;
        $str =~ s/&frac1x24;/ 1x24/g;
        $str =~ s/&frac1x20;/ 1x20/g;
        $str =~ s/&frac1x8000;/ 1x8000/g;
        $str =~ s/&frac1000x1434;/ 1000x1434/g;
        #grek alphabet:
        $str =~ s/&alpha;/[alpha]/g;
        $str =~ s/&beta;/[beta]/g;
        $str =~ s/&GAMMA;/[GAMMA]/g;
        $str =~ s/&gamma;/[gamma]/g;
        $str =~ s/&DELTA;/[DELTA]/g;
        $str =~ s/&delta;/[delta]/g;
        $str =~ s/&ETH;/[ETH]/g;
        $str =~ s/&eth;/[eth]/g;
        $str =~ s/&eta;/eta/g;
        $str =~ s/&theta;/[theta]/g;
        $str =~ s/&iota;/[iota]/g;
        $str =~ s/&kappa;/[kappa]/g;
        $str =~ s/&LAMBDA;/[LAMBDA]/g;
        $str =~ s/&lambda;/[lambda]/g;
        $str =~ s/&chi;/[chi]/g;
        $str =~ s/&omicron;/[omicron]/g;
        $str =~ s/&pi;/pi/g;
        $str =~ s/&PI;/pi/g;
        $str =~ s/&rho;/[rho]/g;
        $str =~ s/&SIGMA;/[SIGMA]/g;
        $str =~ s/&sigma;/[sigma]/g;
        $str =~ s/&TAU;/[TAU]/g;
        $str =~ s/&tau;/[tau]/g;
        $str =~ s/&epsilon;/[epsilon]/g;
        $str =~ s/&OMEGA;/[OMEGA]/g;
        $str =~ s/&UPSILON;/[UPSILON]/g;
        $str =~ s/&upsilon;/[upsilon]/g;
        $str =~ s/&zeta;/[zeta]/g;
        $str =~ s/&phi;/[phi]/g;
        $str =~ s/&mu;/[mu]/g;
        $str =~ s/&nu;/[nu]/g;
        $str =~ s/&thorn;/[thorn]/g;
        #sth like ' but looks like 6 but very small
        $str =~ s/&lsquo;/\'/g;
        #-
        $str =~ s/&mdash;/-/g;
        #" '
        $str =~ s/&Prime;/\" /g;
        $str =~ s/&prime;/\' /g;
        #logical and - ^
        $str =~ s/&and;/^/g;
        #£ - pound
        $str =~ s/&pound;/pound/g;
        #symbol for div
        $str =~ s/&divide;/\:/g;
        #symbol of sqrt
        $str =~ s/&radic;/sqrt/g;
        $str =~ s/&times;/x/g;
        #looks like a sword
        $str =~ s/&dagger;/+/g;
        $str =~ s/&Dagger;/+/g;
        #paragraph symbol
        $str =~ s/&para;/\n\n/g;
        #tags < >
        $str =~ s/&gt;/>/g;
        $str =~ s/&lt;/</g;
        #middle dot * . ???
        $str =~ s/&middot;/*/g;
        #degrees ? how to show that?
        $str =~ s/&deg;/deg/g;
        #section?
        $str =~ s/&sect;/[sect]/g;



        ######################
        # Undefined ampersands
        $str =~ s/&abreve;/[abreve]/g;
        $str =~ s/&ebreve;/[ebreve]/g;
        $str =~ s/&ibreve;/[ibreve]/g;
        $str =~ s/&ubreve;/[ubreve]/g;
        $str =~ s/&breve;/[breve]/g;

        $str =~ s/&amacr;/[=a]/g;
        $str =~ s/&emacr;/[=e]/g;
        $str =~ s/&aemacr;/[=AE]/g;
        $str =~ s/&imacr;/[=i]/g;
        $str =~ s/&umacr;/[=u]/g;
        $str =~ s/&omacr;/[=o]/g;

        $str =~ s/&ffllig;/[ffllig]/g;
        $str =~ s/&fllig;/[fllig]/g;
        $str =~ s/&thlig;/[thlig]/g;
        $str =~ s/&filig;/[filig]/g;

        $str =~ s/&sigmat;/[sigmat]/g;

        $str =~ s/&upslur;/[upslur]/g;
        $str =~ s/&Crev;/[Crev]/g;
        $str =~ s/&oomac;/[oomac]/g;
        $str =~ s/&oocr;/[oocr]/g;

        $str =~ s/&flat;/[flat]/g;
        $str =~ s/&sharp;/[sharp]/g;
        $str =~ s/&add;/add/g;
        $str =~ s/&pause;/[pause]/g;

        $str =~ s/&8star;/[8star]/g;
        $str =~ s/&dale;/[dale]/g;
        $str =~ s/&dele;/[dele]/g;
        $str =~ s/&natural;/[natural]/g;
        $str =~ s/&min;/[min]/g;
        $str =~ s/&fist;/[fist]/g;
        $str =~ s/&asper;/[asper]/g;
        $str =~ s/&\?;/\?/g; #??        what is that?
        $str =~ s/&male;/[male]/g;
        $str =~ s/&ssmile;/[ssmile]/g;
        $str =~ s/&downslur;/[downslur]/g;
        $str =~ s/&pisces;/[pisces]/g;

        $str =~ s/&scorpio;/[scorpio]/g;
        $str =~ s/&Jupiter;/[Jupiter]/g;
        $str =~ s/&Leo;/[Leo]/g;
        $str =~ s/&libra;/[libra]/g;
        $str =~ s/&mercury;/[mercury]/g;
        $str =~ s/&Sun;/[Sun]/g;
        $str =~ s/&sagittarius;/[sagittarius]/g;
        $str =~ s/&taurus;/[taurus]/g;
        $str =~ s/&Virgo;/[Virgo]/g;


        $str =~ s/&adot;/[adot]/g;
        $str =~ s/&2dot;/[2dot]/g;
        $str =~ s/&3dot;/[3dot]/g;
        $str =~ s/&tsdot;/[tsdot]/g;

        $str =~ s/&isl;/[isl]/g;

        return $str;
}   ##StripFromAmpersands
############################################################################
# Delete all html tags form string
#
sub StripFromHtmlTags       #2004-02-12 17:08
############################################################################
{
        $str = shift;

        #italic - ignored
        $str =~ s/\<i\>//g;
        $str =~ s/\<\/i\>//g;

        #bold - ignored
        $str =~ s/\<b\>//g;
        $str =~ s/\<\/b\>//g;

        $str =~ s/\<sd\>//g;
        $str =~ s/\<\/sd\>//g;

        $str =~ s/\<cd\>//g;
        $str =~ s/\<\/cd\>//g;

        $str =~ s/\<sup\>//g;
        $str =~ s/\<\/sup\>//g;

        $str =~ s/\<col\>//g;
        $str =~ s/\<\/col\>//g;

        $str =~ s/\<umac\>//g;
        $str =~ s/\<\/umac\>//g;

        $str =~ s/\<qpers\>//g;
        $str =~ s/\<\/qpers\>//g;

        $str =~ s/\<wf\>//g;
        $str =~ s/\<\/wf\>//g;

        $str =~ s/\<singw\>//g;
        $str =~ s/\<\/singw\>//g;

        $str =~ s/\<sups\>//g;
        $str =~ s/\<\/sups\>//g;

        $str =~ s/\<subs\>//g;
        $str =~ s/\<\/subs\>//g;

        $str =~ s/\<stageof\>//g;
        $str =~ s/\<\/stageof\>//g;

        $str =~ s/\<est\>//g;
        $str =~ s/\<\/est\>//g;

        $str =~ s/\<funct\>//g;
        $str =~ s/\<\/funct\>//g;

        $str =~ s/\<fract\>//g;
        $str =~ s/\<\/fract\>//g;

        $str =~ s/\<plw\>//g;
        $str =~ s/\<\/plw\>//g;

        $str =~ s/\<chname\>//g;
        $str =~ s/\<\/chname\>//g;

        $str =~ s/\<universbold\>//g;
        $str =~ s/\<\/universbold\>//g;

        $str =~ s/\<sansserif\>//g;
        $str =~ s/\<\/sansserif\>//g;

        $str =~ s/\<pos\>//g;
        $str =~ s/\<\/pos\>//g;

        $str =~ s/\<sub\>//g;
        $str =~ s/\<\/sub\>//g;

        $str =~ s/\<sd\>//g;
        $str =~ s/\<\/sd\>//g;

        $str =~ s/\<org\>//g;
        $str =~ s/\<\/org\>//g;

        #word - used in definition
        $str =~ s/\<hw\>//g;
        $str =~ s/\<\/hw\>//g;

        #synonyms
        $str =~ s/\<sn\>//g;
        $str =~ s/\<\/sn\>//g;

        #quote - (")
        $str =~ s/\<blockquote\>/\"/g;
        $str =~ s/\<\/blockquote\>/\"/g;

        #
        $str =~ s/\<grk\>//g;
        $str =~ s/\<\/grk\>//g;

        #
        $str =~ s/\<u\>//g;
        $str =~ s/\<\/u\>//g;

        #enter
        $str =~ s/<BR>/\n/g;

        return $str;
}   ##StripFromHtmlTags
############################################################################
# Check if string contains some undefined ampersands or html tags
#
sub FindUndefinedAmpsAndTags        #2004-02-12 17:26
############################################################################
{
        $str = shift;

        ###########
        # HTML tags
        ###########
        if ($str =~ m:<(.*?)>(.*?)<\/(.*?)>:)
		{
            $undefinedTags[$undefinedTagsCount] = "<$1>";
            ++$undefinedTagsCount;
        }
        # single tags
        if ($str =~ m:<(.*?)>:)
		{
            $undefinedTags[$undefinedTagsCount] = "<$1>";
            ++$undefinedTagsCount;
        }
        ####################
        # AMPERSANDS - &(*);
        ####################
        if ($str =~ m:&(.*?)\;:)
		{
            $undefinedAmps[$undefinedAmpsCount] = "(&$1;) ";
            ++$undefinedAmpsCount;
            print "$word\n";
        }
        ################################
        # Mark them to find them in html
        # ##############################
        #$str =~ s/&/<FONT color=red size=+3>&/g;
        #$str =~ s/;/;<\/FONT>/g;

        return $str;
}   ##FindUndefinedAmpsAndTags
############################################################################
# Format definition
#
sub StripDefinition        #2004-02-12 13:39
############################################################################
{
        $def = shift;

        $def = StripFromSpecials($def);
        $def = StripFromStars($def);
        $def = StripFromAmpersands($def);
        $def = StripFromHtmlTags($def);

        $def =~ s/\n/ /g;

        $def = FindUndefinedAmpsAndTags($def);

        return $def;
}   ##StripDefinition
############################################################################
# Format word
#
sub StripWord       #2004-02-12 13:52
############################################################################
{
        $word = shift;

#        $word =~ s/&eacute;/e/g;
#        $word =~ s/&aelig;/ae/g;
#        $word =~ s/&oelig;/oe/g;
#        $word =~ s/&AElig;/AE/g;
#        $word =~ s/&ouml;/o/g;
#        $word =~ s/&euml;/e/g;
#        $word =~ s/&ccedil;/c/g;
#        $word =~ s/&ecirc;/e/g;
#        $word =~ s/&egrave;/e/g;
#

        $word = StripFromStars($word);
        $word = StripFromAmpersands($word);

        $word =~ s/\n/ /g;

        $word = FindUndefinedAmpsAndTags($word);

        return $word;
}   ##StripWord
############################################################################
# Format example
#
sub StripExample        #2004-02-12 14:03
############################################################################
{
        $example = shift;

        $example = StripFromSpecials($example);
        $example = StripFromStars($example);
        $example = StripFromAmpersands($example);
        $example = StripFromHtmlTags($example);
        $example =~ s/\n/ /g;

        $example = FindUndefinedAmpsAndTags($example);

        return $example;
}   ##StripExample
############################################################################
# Format POS
#
sub StripPos        #2004-02-12 14:02
############################################################################
{
        $pos = shift;
        $pos = StripFromHtmlTags($pos);
        $pos =~ s/\n/ /g;

        #####################################
        #set default n. -> Noun, etc...
        # Only:
        # n., adj., adv., v.i., v.t.
        if($pos =~m:n.(.*?):)
        {
            return "noun";
        }
        if($pos =~m:adv.:)
        {
            return "adv.";
        }

        if($pos =~m:v. t.:)
        {
            return "verb";
        }

        if($pos =~m:v. i.:)
        {
            return "verb";
        }

        if($pos =~m:a.:)
        {
            return "adj.";
        }

        ###################################
        # TODO: Define them - what is that?
        ###################################
        if($pos =~m:p. p.:)
        {
            return "p.p.";
        }
        if($pos =~m:prep.:)
        {
            return "prep.";
        }
        if($pos =~m:pret.:)
        {
            return "pret.";
        }
        if($pos =~m:imp.:)
        {
            return "imp.";
        }
        if($pos =~m:v.:)
        {
            return "verb";
        }
        if($pos =~m:A prefix.:)
        {
            return "A prefix.";
        }
        if($pos =~m:prefix.:)
        {
            return "prefix.";
        }
        if($pos =~m:pref.:)
        {
            return "pref.";
        }
        if($pos =~m:n:)
        {
            return "noun";
        }
        if($pos =~m:N.:)
        {
            return "noun";
        }
        if($pos =~m:a:)
        {
            return "a";
        }
        if($pos =~m:pl.:)
        {
            return "pl.";
        }
        if($pos =~m:p.:)
        {
            return "p.";
        }
        if($pos =~m:q.:)
        {
            return "q.";
        }
        if($pos =~m:suffix.:)
        {
            return "suffix.";
        }
        if($pos =~m:pl. pres.:)
        {
            return "pl. pres.";
        }
        if($pos =~m:superl.:)
        {
            return "superl.";
        }
        if($pos =~m:&\?\;\.:)
        {
            return "&?;.";
        }

        $undefinedPos[$undefinedPosCount] = "($pos) ";
        $undefinedPosCount++;

        $retPosTmp = "Unknown POS:  $pos";
        return $retPosTmp;

        return $pos;
}   ##StripPos
############################################################################
############################################################################
$ActGloss = "";
############################################################################
# In verb file
sub SaveActualGlossVerb     #2004-02-22 15:20
############################################################################
{
    $odl = tell OUT_V;
    printf OUT_V "%08d",$odl;
    print OUT_V " ";
    #file code?
    print OUT_V "00";
    print OUT_V " v 01 ";
    #word - underline instead space
    $wordTmpV = $word;
    $wordTmpV =~ s/ /_/g;

    if( $prevWordTmpV eq $wordTmpV)
    {
        $prevWordTmpV = $wordTmpV;
        $numWordV = 0;
    }
    else
    {
        ++$numWordV;
    }

    print OUT_V "$wordTmpV ";
    print OUT_V "$numWordV ";

    print OUT_V "000 ";

    #only verbs
    print OUT_V "00 ";

    #gloss
    print OUT_V "| ";
    print OUT_V "$ActGloss  \n";

    $ActGloss = "";
}   ##SaveActualGlossVerb
############################################################################
# In noun file
sub SaveActualGlossNoun     #2004-02-22 15:21
############################################################################
{
    $odl = tell OUT_N;
    printf OUT_N "%08d",$odl;
    print OUT_N " ";
    #file code?
    print OUT_N "00";
    print OUT_N " n 01 ";
    #word - underline instead space
    $wordTmpN = $word;
    $wordTmpN =~ s/ /_/g;

    if( ! ($prevWordTmpN eq $wordTmpN))
    {
        $prevWordTmpN = $wordTmpN;
        $numWordN = 0;
    }
    else
    {
        ++$numWordN;
    }

    print OUT_N "$wordTmpN ";
    print OUT_N "$numWordN ";

    print OUT_N "000 ";

    #gloss
    print OUT_N "| ";
    print OUT_N "$ActGloss  \n";

    $ActGloss = "";
}   ##SaveActualGlossNoun
############################################################################
# In adv file
sub SaveActualGlossAdverb       #2004-02-22 15:22
############################################################################
{
    $odl = tell OUT_ADV;
    printf OUT_ADV "%08d",$odl;
    print OUT_ADV " ";
    #file code?
    print OUT_ADV "00";
    print OUT_ADV " r 01 ";
    #word - underline instead space
    $wordTmpADV = $word;
    $wordTmpADV =~ s/ /_/g;

    if( $prevWordTmpADV eq $wordTmpADV)
    {
        $prevWordTmpADV = $wordTmpADV;
        $numWordADV = 0;
    }
    else
    {
        ++$numWordADV;
    }

    print OUT_ADV "$wordTmpADV ";
    print OUT_ADV "$numWordADV ";

    print OUT_ADV "000 ";

    #gloss
    print OUT_ADV "| ";
    print OUT_ADV "$ActGloss  \n";

    $ActGloss = "";
}   ##SaveActualGlossAdverb
############################################################################
sub SaveActualGlossAdjective        #2004-02-22 15:22
############################################################################
{
    $odl = tell OUT_ADJ;
    printf OUT_ADJ "%08d",$odl;
    print OUT_ADJ " ";
    #file code?
    print OUT_ADJ "00";
    print OUT_ADJ " a 01 ";
    #word - underline instead space
    $wordTmpADJ = $word;
    $wordTmpADJ =~ s/ /_/g;

    if( $prevWordTmpADJ eq $wordTmpADJ)
    {
        $prevWordTmpADJ = $wordTmpADJ;
        $numWordADJ = 0;
    }
    else
    {
        ++$numWordADJ;
    }

    print OUT_ADJ "$wordTmpADJ ";
    print OUT_ADJ "$numWordADJ ";

    print OUT_ADJ "000 ";

    #gloss
    print OUT_ADJ "| ";
    print OUT_ADJ "$ActGloss  \n";

    $ActGloss = "";
}   ##SaveActualGlossAdjective
############################################################################
# Save actual gloss
# in wn20 format!
sub SaveActualGloss     #2004-02-22 15:02
############################################################################
{
    #first word in file
    if ( ! $ActGloss )
    {
        return;
    }
    #identify pos - identify output file
    if ( $pos =~m:verb:)
    {
        SaveActualGlossVerb;
    }
    if ( $pos =~m:noun:)
    {
        SaveActualGlossNoun;
    }
    if ( $pos =~m:adv.:)
    {
        SaveActualGlossAdverb;
    }
    if ( $pos =~m:adj.:)
    {
        SaveActualGlossAdjective;
    }

    if ( ! $ActGloss )
    {
        return;
    }
    #undefined pos!!!!
#    print "$ActGloss\n";
#    print "undefined pos: $pos \n\n\n";
    $ActGloss = "";
}   ##SaveActualGloss
############################################################################
############################################################################
# Files should be opened and ready!
sub MainLoop        #2004-02-13 21:51
############################################################################
{
$ActGloss = "";
while ( $words_count<=$max_words && $whole_file =~ m:<p>(.*?)</p>:sig ) {

    $paragraph = $1;

    if ( $paragraph =~ m:^<hw>(.*?)</hw>:si )
	{
        SaveActualGloss;

		if ($paragraph =~ m:<!.+!>:)
		{
            print "!Another page \n";
			next;
		}
        $word = $1;
#        next if $word eq $prev_word;
#        $prev_word = $word;

        $word = StripWord($word);

        $unhandled_len = 0;
		if ($word =~ m:&(.+?)\;:)
		{
			$unhandled[$unhandled_count] = "&$1;";
            ++$unhandled_count;
			$unhandled_len = length $1 + 1;
		}
#       print "($word)\n";
       print OUT2 "<BR><b><FONT color=\"blue\">$word</FONT></b><BR>";

        ++$words_count;
		$words_len += length $word;
		$words_len -= $unhandled_len;

        if ( $paragraph =~ m:^*<pos>(.*?)</pos>:si )
        {
            $pos = $1;
            $pos = StripPos($pos);
        }
        else
        {
            $pos = "----undefined----";
        }

#        print "($pos)\n";
        print OUT2 "<i><FONT color=\"green\">$pos</FONT></i><BR>";
        if ( $paragraph =~ m:^*<def>(.*?)</def>:si )
        {
            if ($paragraph =~ m:<!.+!>:)
            {
                print "!Another page \n";
                next;
            }
            $definition = $1;
            $definition = StripDefinition($definition);
#            print "$definition\n";
            $tmpGloss = $ActGloss;
            $ActGloss = "$tmpGloss$definition";

            print OUT2 "$definition<BR>";
        }
    }

    if ( $paragraph =~ m:^<sn>(.*?)</sn>:si )
	{
        if ( $paragraph =~ m:^*<def>(.*?)</def>:si )
        {
            if ($paragraph =~ m:<!.+!>:)
            {
                print "!Another page \n";
                next;
            }
#            print "($pos)\n";
            print OUT2 "<i><FONT color=\"green\">$pos</FONT></i><BR>";
            $definition = $1;
            $definition = StripDefinition($definition);
#            print "$definition\n";
            $tmpGloss = $ActGloss;
            $ActGloss = "$tmpGloss; $definition";
            print OUT2 "$definition<BR>";
        }
    }

    if ( $paragraph =~ m:^<blockquote>(.*?)</blockquote>:si )
	{
		if ($paragraph =~ m:<!.+!>:)
		{
			print "!Another page \n";
			next;
		}
        $example = $1;
        $example = StripExample($example);
#        print "\"$example\"\n";
        $tmpGloss = $ActGloss;
        $ActGloss = "$tmpGloss; \"$example\"";
        print OUT2 "<i><FONT color=\"red\">\"$example\"</FONT></i><BR>";
    }

    ++$paragraph_count;
}
    SaveActualGloss;
}   ##MainLoop
############################################################################
# FILES
############################################################################
$OUT_FILE_NAME  = "out.txt";
$OUT_FILE_NAME2 = "out.html";

$OUT_FILE_VERB = "data.verb";
$OUT_FILE_NOUN = "data.noun";
$OUT_FILE_ADJ  = "data.adj";
$OUT_FILE_ADV  = "data.adv";

$IN_FILE_NAME[0]  = wb_full_path("pgw050ab.txt");
$IN_FILE_NAME[1]  = wb_full_path("pgw050c.txt");
$IN_FILE_NAME[2]  = wb_full_path("pgw050de.txt");
$IN_FILE_NAME[3]  = wb_full_path("pgw050fh.txt");
$IN_FILE_NAME[4]  = wb_full_path("pgw050il.txt");
$IN_FILE_NAME[5]  = wb_full_path("pgw050mo.txt");
$IN_FILE_NAME[6]  = wb_full_path("pgw050pq.txt");
$IN_FILE_NAME[7]  = wb_full_path("pgw050r.txt");
$IN_FILE_NAME[8]  = wb_full_path("pgw050s.txt");
$IN_FILE_NAME[9]  = wb_full_path("pgw050tw.txt");
$IN_FILE_NAME[10] = wb_full_path("pgw050xz.txt");

############################################################################
# Counters - to detect anomalies
############################################################################
$undefinedAmpsCount = 0;
$undefinedTagsCount = 0;
$undefinedPosCount = 0;
############################################################################
open( OUT, ">$OUT_FILE_NAME" ) || die "Cannot open $OUT_FILE_NAME as an output";
open( OUT2, ">$OUT_FILE_NAME2" ) || die "Cannot open $OUT_FILE_NAME2 as an output";
print OUT2 "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\"><HTML><HEAD><TITLE>TEST</TITLE></HEAD><BODY bgColor=#ffffff>";

open( OUT_V, ">$OUT_FILE_VERB" ) || die "Cannot open $OUT_FILE_VERB as an output";
open( OUT_N, ">$OUT_FILE_NOUN" ) || die "Cannot open $OUT_FILE_NOUN as an output";
open( OUT_ADJ, ">$OUT_FILE_ADJ" ) || die "Cannot open $OUT_FILE_ADJ as an output";
open( OUT_ADV, ">$OUT_FILE_ADV" ) || die "Cannot open $OUT_FILE_ADV as an output";

$prev_word = "";
$words_count = 0;
$max_words = 900000;
$unhandled_count = 0;
$words_len = 0;

############################################################################
#   Main loop
############################################################################
$filesCount = 0;
while ( $filesCount < 11 )
{

($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size, $atime,$mtime,$ctime,$blksize,$blocks) = stat($IN_FILE_NAME[$filesCount]);
die "cannot stat the file: $IN_FILE_NAME[$filesCount]" unless defined $dev;
print "\n\nfile size=$size \n";
open( IN, $IN_FILE_NAME[$filesCount] ) || die "Cannot open $IN_FILE_NAME[$filesCount] as an input";
print "Startting to read the file...\n";

$read_size = read IN, $whole_file, $size;
print "real size=$size, read_size=$read_size\n";
print "Successfully read the file...\n";

$count = 0;

    MainLoop();

    close IN;

    ++$filesCount;
}
############################################################################
# End of main loop
############################################################################
print OUT2 "</BODY></HTML>";

print "\n\n\n\n\n\n\n";

print "\nunhandled specials:\n";
print @unhandled;
print "\n";

print "\nundefined pos:\n";
print @undefinedPos;
print "\n";
print "\nundefined amps:\n";
print @undefinedAmps;
print "\n";
print "\nundefined tags:\n";
print @undefinedTags;
print "\n";


print "words: $words_count \n";
print "total words len: $words_len \n";
$total_len = $words_count + $words_len;
print "total words len including 0: $total_len \n";
$average = $total_len / $words_count;
print "average per word: $average \n";
print "End of program.\n";
close OUT;
close OUT2;
close OUT_V;
close OUT_N;
close OUT_ADJ;
close OUT_ADV;
