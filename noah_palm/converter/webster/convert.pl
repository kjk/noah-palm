############################################################################
require 5.001;
############################################################################

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
        $str =~ s/&eacute;/e/g;
        $str =~ s/&Eacute;/E/g;

        $str =~ s/&aelig;/ae/g;
        $str =~ s/&oelig;/oe/g;
        $str =~ s/&OElig;/OE/g;
        $str =~ s/&AElig;/AE/g;
        $str =~ s/&ccedil;/c/g;
        $str =~ s/&ecirc;/e/g;
        $str =~ s/&acirc;/a/g;

        $str =~ s/&ouml;/[\"o]/g;
        $str =~ s/&euml;/[\"e]/g;
        $str =~ s/&auml;/[\"a]/g;
        $str =~ s/&Auml;/[\"A]/g;
        $str =~ s/&iuml;/[\"i]/g;
        $str =~ s/&uuml;/[\"u]/g;

        $str =~ s/&egrave;/e/g;
        $str =~ s/&agrave;/[\'a]/g;

        $str =~ s/&amacr;/[=a]/g;
        $str =~ s/&emacr;/[=e]/g;
        $str =~ s/&aemacr;/[=AE]/g;
        $str =~ s/&imacr;/[=i]/g;
        $str =~ s/&umacr;/[=u]/g;
        $str =~ s/&omacr;/[=o]/g;

        $str =~ s/&ntilde;/~n/g;
        $str =~ s/&ltilde;/~l/g;

        $str =~ s/&alpha;/[alpha]/g;
        $str =~ s/&deg;/deg/g;

        $str =~ s/&frac12/ 1\/2/g;
        $str =~ s/&frac14/ 1\/4/g;
        $str =~ s/&frac34/ 3\/4/g;
        $str =~ s/&frac13/ 1\/3/g;
        $str =~ s/&frac15/ 1\/5/g;
        $str =~ s/&frac17/ 1\/7/g;
        $str =~ s/&frac19/ 1\/9/g;

        $str =~ s/&frac58/ 5\/8/g;
        $str =~ s/&frac38/ 3\/8/g;
        $str =~ s/&frac36/ 1\/2/g;
        $str =~ s/&frac56/ 5\/6/g;

        $str =~ s/&frac1x10/ 1x10/g;

        ######################
        # Undefined ampersands
        $str =~ s/&Prime;/Prime/g;
        $str =~ s/&prime;/prime/g;

        $str =~ s/&flat;/[flat]/g;
        $str =~ s/&sharp;/[sharp]/g;

        $str =~ s/&SIGMA;/[SIGMA]/g;
        $str =~ s/&DELTA;/[DELTA]/g;
        $str =~ s/&UPSILON;/[UPSILON]/g;

        $str =~ s/&and;/and/g;
        $str =~ s/&add;/add/g;
        $str =~ s/&pi;/pi/g;
        $str =~ s/&PI;/pi/g;
        $str =~ s/&eta;/eta/g;

        $str =~ s/&sect;/[sect]/g;
        $str =~ s/&lsquo;/[lsquo]/g;
        $str =~ s/&pound;/[pound]/g;
        $str =~ s/&scorpio;/[scorpio]/g;
        $str =~ s/&male;/[male]/g;
        $str =~ s/&divide;/\:/g;

        $str =~ s/&upslur;/[upslur]/g;
        $str =~ s/&Crev;/[Crev]/g;


        $str =~ s/&GAMMA;/[GAMMA]/g;
        $str =~ s/&gamma;/[gamma]/g;
        $str =~ s/&LAMBDA;/[LAMBDA]/g;
        $str =~ s/&OMEGA;/[OMEGA]/g;
        $str =~ s/&delta;/[delta]/g;
        $str =~ s/&TAU;/[TAU]/g;
        $str =~ s/&tau;/[tau]/g;
        $str =~ s/&beta;/[beta]/g;
        $str =~ s/&epsilon;/[epsilon]/g;
        $str =~ s/&icirc;/[icirc]/g;
        $str =~ s/&ocirc;/[ocirc]/g;
        $str =~ s/&pause;/[pause]/g;

        $str =~ s/&Jupiter;/[Jupiter]/g;
        $str =~ s/&aacute;/[aacute]/g;
        $str =~ s/&zeta;/[zeta]/g;
        $str =~ s/&aacute;/[aacute]/g;
        $str =~ s/&adot;/[adot]/g;
        $str =~ s/&2dot;/[2dot]/g;
        $str =~ s/&oomac;/[oomac]/g;
        $str =~ s/&tsdot;/[tsdot]/g;
        $str =~ s/&Leo;/[Leo]/g;

        $str =~ s/&dagger;/[dagger]/g;
        $str =~ s/&Dagger;/[Dagger]/g;
        $str =~ s/&dale;/[dale]/g;
        $str =~ s/&filig;/[filig]/g;
        $str =~ s/&theta;/[theta]/g;
        $str =~ s/&kappa;/[kappa]/g;
        $str =~ s/&para;/[para]/g;
        $str =~ s/&natural;/[natural]/g;


        $str =~ s/&abreve;/[abreve]/g;
        $str =~ s/&ebreve;/[ebreve]/g;
        $str =~ s/&ibreve;/[ibreve]/g;
        $str =~ s/&breve;/[breve]/g;
        $str =~ s/&ugreve;/[ugreve]/g;
        $str =~ s/&rho;/[rho]/g;


        $str =~ s/&mdash;/[^m]/g;
        $str =~ s/&ssmile;/[ssmile]/g;
        $str =~ s/&min;/[min]/g;
        $str =~ s/&radic;/[radic]/g;
        $str =~ s/&fist;/[fist]/g;
        $str =~ s/&asper;/[asper]/g;
        $str =~ s/&\?;/\?/g; #??        what is that?

        $str =~ s/&frac1x6000;/[frac1x6000]/g;
        $str =~ s/&iota;/[iota]/g;
        $str =~ s/&dale;/[dale]/g;
        $str =~ s/&umlaut;/[umlaut]/g;
        $str =~ s/&frac1x50000;/[frac1x50000]/g;
        $str =~ s/&eth;/[eth]/g;
        $str =~ s/&frac1x29966;/[frac1x29966]/g;
        $str =~ s/&middot;/[middot]/g;
        $str =~ s/&thlig;/[thlig]/g;
        $str =~ s/&frac1x24;/[frac1x24]/g;
        $str =~ s/&thorn;/[thorn]/g;
        $str =~ s/&gt;/[gt]/g;
        $str =~ s/&iota;/[iota]/g;
        $str =~ s/&lambda;/[lambda]/g;
        $str =~ s/&libra;/[libra]/g;
        $str =~ s/&ffllig;/[ffllig]/g;
        $str =~ s/&mercury;/[mercury]/g;
        $str =~ s/&omicron;/[omicron]/g;
        $str =~ s/&oocr;/[oocr]/g;
        $str =~ s/&oacute;/[oacute]/g;
        $str =~ s/&Sun;/[Sun]/g;
        $str =~ s/&pisces;/[pisces]/g;
        $str =~ s/&ugrave;/[ugrave]/g;
        $str =~ s/&3dot;/[3dot]/g;
        $str =~ s/&sagittarius;/[sagittarius]/g;
        $str =~ s/&sigma;/[sigma]/g;
        $str =~ s/&downslur;/[downslur]/g;
        $str =~ s/&frac1x20;/[frac1x20]/g;
        $str =~ s/&times;/[times]/g;
        $str =~ s/&taurus;/[taurus]/g;
        $str =~ s/&chi;/[chi]/g;
        $str =~ s/&isl;/[isl]/g;
        $str =~ s/&ubreve;/[ubreve]/g;
        $str =~ s/&frac23;/[frac23]/g;
        $str =~ s/&upsilon;/[upsilon]/g;

        $str =~ s/&mu;/[mu]/g;
        $str =~ s/&nu;/[nu]/g;
        $str =~ s/&frac1x8000;/[frac1x8000]/g;
        $str =~ s/&dele;/[dele]/g;
        $str =~ s/&ETH;/[ETH]/g;
        $str =~ s/&lt;/[lt]/g;
        $str =~ s/&fllig;/[fllig]/g;
        $str =~ s/&sigmat;/[sigmat]/g;
        $str =~ s/&phi;/[phi]/g;
        $str =~ s/&Virgo;/[Virgo]/g;

        $str =~ s/&Ccedil;/[Ccedil]/g;
        $str =~ s/&ucirc;/[ucirc]/g;
        $str =~ s/&atilde;/[atilde]/g;

        return $str;
}   ##StripFromAmpersands
############################################################################
# Delete all html tags form string
#
sub StripFromHtmlTags       #2004-02-12 17:08
############################################################################
{
        $str = shift;

        $str =~ s/\<i\>//g;
        $str =~ s/\<\/i\>//g;

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

        $str =~ s/\<\sd\>//g;
        $str =~ s/\<\/sd\>//g;

        $str =~ s/\<\org\>//g;
        $str =~ s/\<\/org\>//g;

        $str =~ s/\<\hw\>//g;
        $str =~ s/\<\/hw\>//g;

        $str =~ s/\<\sn\>//g;
        $str =~ s/\<\/sn\>//g;

        $str =~ s/\<\blockquote\>//g;
        $str =~ s/\<\/blockquote\>//g;

        $str =~ s/\<\grk\>//g;
        $str =~ s/\<\/grk\>//g;

        $str =~ s/\<u\>//g;
        $str =~ s/\<\/u\>//g;
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
        if ($str =~ m:<(.*?)\/>:)
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
        $str =~ s/&/<FONT color=red size=+3>&/g;
        $str =~ s/;/;<\/FONT>/g;

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
            return "n.";
        }

        if($pos =~m:a.:)
        {
            return "adj.";
        }

        if($pos =~m:adVerb:)
        {
            return "adv.";
        }

        if($pos =~m:v. t.:)
        {
            return "v.t.";
        }

        if($pos =~m:v. i.:)
        {
            return "v.i.";
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
            return "v.";
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
            return "n";
        }
        if($pos =~m:N.:)
        {
            return "N.";
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
# Files should be opened and ready!
sub MainLoop        #2004-02-13 21:51
############################################################################
{
while ( $words_count<=$max_words && $whole_file =~ m:<p>(.*?)</p>:sig ) {

    $paragraph = $1;

    if ( $paragraph =~ m:^<hw>(.*?)</hw>:si )
	{
		if ($paragraph =~ m:<!.+!>:)
		{
			print "!Another page \n";
			next;
		}
        $word = $1;
        next if $word eq $prev_word;
		$prev_word = $word;

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
        print OUT2 "<i><FONT color=\"red\">\"$example\"</FONT></i><BR>";
    }

    ++$paragraph_count;
}
}   ##MainLoop
############################################################################
# FILES
############################################################################
$OUT_FILE_NAME  = "out.txt";
$OUT_FILE_NAME2  = "out.html";

$IN_FILE_NAME[0]  = "pgw050ab.txt";
$IN_FILE_NAME[1]  = "pgw050c.txt";
$IN_FILE_NAME[2]  = "pgw050de.txt";
$IN_FILE_NAME[3]  = "pgw050fh.txt";
$IN_FILE_NAME[4]  = "pgw050il.txt";
$IN_FILE_NAME[5]  = "pgw050mo.txt";
$IN_FILE_NAME[6]  = "pgw050pq.txt";
$IN_FILE_NAME[7]  = "pgw050r.txt";
$IN_FILE_NAME[8]  = "pgw050s.txt";
$IN_FILE_NAME[9]  = "pgw050tw.txt";
$IN_FILE_NAME[10] = "pgw050xz.txt";
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


$prev_word = "";
$words_count = 0;
$max_words = 102000;
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
