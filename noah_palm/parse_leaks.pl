#!/usr/bin/perl

$file = "c:\\leaks.txt";

open( F, $file ) || die( "cannot open file $file " );

$curr_usage = 0;
$max_usage = 0;
while (<F>) {
    #print $_;
    @line = split;
    #print @line;
    if ($line[0] eq "@") {
    $addr = $line[1];
    $size = $line[2];
#    print "$addr : $size\n";
    $allocs{ $addr } = $size;
    $full_line{ $addr } = $_;
    $curr_usage += hex( $line[2] );
    $max_usage = $curr_usage > $max_usage ? $curr_usage : $max_usage;
    } else {
    $addr = $line[1];
    $size = $allocs{ $addr };
    $frees{ $addr } = $size;
    $curr_usage -= hex($size);
    }
}
close(F);

foreach $key (keys %allocs) {
    if ( !exists $frees{ $key } ) {
    $size = $allocs{ $key };
    $line = $full_line{ $key };
    print "mem leak: $line";
    }
}

print "Max usage: $max_usage\n";

