############################################################################
require 5.001;
############################################################################
############################################################################
# Deletes all umpersands from string
#
sub StripFromAmpersands     #2004-02-12 17:06
############################################################################
{
        $str = shift;

        #############################################
        #   Delete all guide to pronunciation bracers
        if ($str =~ m:\((.*?)Guide to Pronunciation(.*?)\):)
        {
            $str =~ s/\((.*?)Guide to Pronunciation(.*?)\)/ /g;

        }

        #######################
        #   find all ampersands
        $str =~ s/&eacute;/e/g;
        $str =~ s/&Eacute;/E/g;
        $str =~ s/&aelig;/ae/g;
        $str =~ s/&oelig;/oe/g;
        $str =~ s/&AElig;/AE/g;
        $str =~ s/&ccedil;/c/g;
        $str =~ s/&ecirc;/e/g;

        $str =~ s/&ouml;/[\"o]/g;
        $str =~ s/&euml;/[\"e]/g;
        $str =~ s/&auml;/[\"a]/g;
        $str =~ s/&uuml;/[\"u]/g;

        $str =~ s/&egrave;/e/g;
        $str =~ s/&agrave;/[\'a]/g;

        $str =~ s/&amacr;/[=a]/g;
        $str =~ s/&aemacr;/[=AE]/g;
        $str =~ s/&imacr;/[=i]/g;
        $str =~ s/&umacr;/[=u]/g;

        $str =~ s/&alpha;/[alpha]/g;

        $str =~ s/&ntilde;/~n/g;

        $str =~ s/&deg;/deg/g;
        $str =~ s/&Prime;/Prime/g;
        $str =~ s/&prime;/prime/g;

        $str =~ s/&frac12/ 1\/2/g;
        $str =~ s/&frac14/ 1\/4/g;
        $str =~ s/&frac34/ 3\/4/g;

        $str =~ s/&flat;/[flat]/g;
        $str =~ s/&sharp;/[sharp]/g;

        $str =~ s/&\?;/\?/g; #??        what is that?

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

        $str =~ s/\<pos\>//g;
        $str =~ s/\<\/pos\>//g;

        $str =~ s/\<sub\>//g;
        $str =~ s/\<\/sub\>//g;

        $str =~ s/\<\sd\>//g;
        $str =~ s/\<\/sd\>//g;

        $str =~ s/\<\org\>//g;
        $str =~ s/\<\/org\>//g;

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
#        $def =~ s/\*//g;
#        $def =~ s/\"//g;
#        $def =~ s/\'//g;
#        $def =~ s/\|//g;
#        $def =~ s/\`//g;

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
        $word =~ s/\*//g;
		$word =~ s/\"//g;
#		$word =~ s/\'//g;
		$word =~ s/\|//g;
		$word =~ s/\`//g;
		$word =~ s/&eacute;/e/g;
		$word =~ s/&aelig;/ae/g;
		$word =~ s/&oelig;/oe/g;
		$word =~ s/&AElig;/AE/g;
		$word =~ s/&ouml;/o/g;
		$word =~ s/&euml;/e/g;
		$word =~ s/&ccedil;/c/g;
		$word =~ s/&ecirc;/e/g;
		$word =~ s/&egrave;/e/g;
        return $word;
}   ##StripWord
############################################################################
# Format POS
#
sub StripPos        #2004-02-12 14:02
############################################################################
{
        $pos = shift;
        $pos =  StripFromHtmlTags($pos);
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

        $undefinedPos[$undefinedPosCount] = "($pos) ";
        $undefinedPosCount++;

        $retPosTmp = "Unknown POS:  $pos";
        return $retPosTmp;

        return $pos;
}   ##StripPos
############################################################################
# Format example
#
sub StripExample        #2004-02-12 14:03
############################################################################
{
        $example = shift;
#        $example =~ s/\*//g;
#        $example =~ s/\"//g;
#        $example =~ s/\'//g;
#        $example =~ s/\|//g;
#        $example =~ s/\`//g;
        $example = StripFromAmpersands($example);
        $example = StripFromHtmlTags($example);
        $example =~ s/\n/ /g;

        $example = FindUndefinedAmpsAndTags($example);

        return $example;
}   ##StripExample
############################################################################
# FILES
############################################################################
$IN_FILE_NAME = "pgw050ab.txt";
$OUT_FILE_NAME  = "out.txt";
$OUT_FILE_NAME2  = "out.html";
############################################################################
# Counters - to detect anomalies
############################################################################
$undefinedAmpsCount = 0;
$undefinedTagsCount = 0;
$undefinedPosCount = 0;



############################################################################
($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size, $atime,$mtime,$ctime,$blksize,$blocks) = stat($IN_FILE_NAME);
die "cannot stat the file: $IN_FILE_NAME" unless defined $dev;
print "\n\n\n\n\n\n\n\n\nfile size=$size \n";

open( IN, $IN_FILE_NAME ) || die "Cannot open $IN_FILE_NAME as an input";
open( OUT, ">$OUT_FILE_NAME" ) || die "Cannot open $OUT_FILE_NAME as an output";
open( OUT2, ">$OUT_FILE_NAME2" ) || die "Cannot open $OUT_FILE_NAME2 as an output";

print "Startting to read the file...\n";

$read_size = read IN, $whole_file, $size;

print "real size=$size, read_size=$read_size\n";

print "Successfully read the file...\n";

$count = 0;
$reps = 20;
$from = 38;
$to = 48;

die "from=$from is bigger or equal than to=$to" unless $to > $from;

$prev_word = "";
$words_count = 0;
$max_words = 10000; #102000;
$unhandled_count = 0;
$words_len = 0;

print OUT2 "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\"><HTML><HEAD><TITLE>TEST</TITLE></HEAD><BODY bgColor=#ffffff>";

############################################################################
#   Main loop
############################################################################
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
close IN;
close OUT;
close OUT2;




