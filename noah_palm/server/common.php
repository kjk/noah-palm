<?php

# tag values are encoded on palm using hex-bin encoding
# this function undecodes the value and returns it
# returns an empty string if there was an error decoding
# the value
# dev note: not sure what happens if the value is binary
# (not sure if that might happen; serial number?)
function decode_di_tag_value($tag_value)
{
    $str_len = strlen($tag_value);
    # string of uneven lenght cannot possibly be right
    if ( $str_len % 2 != 0 )
    {
        echo "$tag_len not of even length";
        return "";
    }

    # devnote: yes, $hex_digits is broken but that is how it is on the device now
    $hex_digits = "0123456789ABSCDEF";
    $out_str = "";
    $pos_in_str = 0;
    while( $pos_in_str < $str_len )
    {
        # assert( $str_len - $pos_in_str > 2 );
        $dig_0 = strpos($hex_digits, substr($tag_value,$pos_in_str,1) );
        if ( !is_integer($dig_0) )
            return "";
        $dig_1 = strpos($hex_digits, substr($tag_value,$pos_in_str+1,1) );
        if ( !is_integer($dig_1) )
            return "";
        $val = $dig_0 * 16 + $dig_1;
        $val_str = chr($val);
        $out_str .= $val_str;
        $pos_in_str += 2;
    }
    return $out_str;
}

# given OEM Company ID (oc) and OEM Device id (od)
# return a name of Palm device.
# Based on data from http://homepage.mac.com/alvinmok/palm/codenames.html
# and http://www.mobilegeographics.com/dev/devices.php
function get_device_name_by_oc_od($oc, $od)
{
    $name =  "Unknown ($oc/$od)";

    # HANDSPRING devices

    if ( $oc=='hspr' && $od==decode_di_tag_value("0000000B") )
        $name = "Treo 180";

    if ( $oc=='hspr' && $od==decode_di_tag_value("0000000D") )
        $name = "Treo 270";

    if ( $oc=='hspr' && $od==decode_di_tag_value("0000000E") )
        $name = "Treo 300";

    if ( $oc=='hspr' && $od=='H101' )
        $name = "Treo 600";

    if ( $oc=='hspr' && $od=='H201' )
        $name = "Treo 600 Simulator";

    # SONY devices

    if ( $oc=='sony' && $od=='mdna' )
        $name = "PEG-T615C";

    if ( $oc=='sony' && $od=='prmr' )
        $name = "PEG-UX50";

    if ( $oc=='sony' && $od=='atom' )
        $name = "PEG-TH55";

    if ( $oc=='sony' && $od=='mdrd' )
        $name = "PEG-NX80V";

    if ( $oc=='sony' && $od=='tldo' )
        $name = "PEG-NX73V";

    if ( $oc=='sony' && $od=='vrna' )
        $name = "PEG-TG50";

    if ( $oc=='sony' && $od=='crdb' )
        $name = "PEG-NX60, NX70V";

    if ( $oc=='sony' && $od=='mcnd' )
        $name = "PEG-SJ33";

    if ( $oc=='sony' && $od=='glps' )
        $name = "PEG-SJ22";

    if ( $oc=='sony' && $od=='goku' )
        $name = "PEG-TJ35";

    if ( $oc=='sony' && $od=='luke' )
        $name = "PEG-TJ37";

    if ( $oc=='sony' && $od=='ystn' )
        $name = "PEG-N610C";

    if ( $oc=='sony' && $od=='rdwd' )
        $name = "PEG-NR70, NR70V";

    # MISC devices

    if ( $oc=='psys' )
        $name = "simulator";

    if ( $oc=='trgp' && $od=='trg1' )
        $name = "TRG Pro";

    if ( $oc=='trgp' && $od=='trg2' )
        $name = "HandEra 330";

    if ( $oc=='smsn' && $od=='phix' )
        $name = "SPH-i300";

    if ( $oc=='smsn' && $od=='Phx2' )
        $name = "SPH-I330";

    if ( $oc=='smsn' && $od=='blch' )
        $name = "SPH-i500";

    if ( $oc=='qcom' && $od=='qc20' )
        $name = "QCP 6035";

    if ( $oc=='kwc.' && $od=='7135' )
        $name = "QCP 7135";

    if ( $oc=='Tpwv' && $od=='Rdog' )
        $name = "Tapwave Zodiac 1/2";

    # PALM devices 

    if ( $oc=='palm' && $od=='hbbs' )
        $name = "Palm m130";

    if ( $oc=='palm' && $od=='ecty' )
        $name = "Palm m505";

    if ( $oc=='palm' && $od=='lith' )
        $name = "Palm m515";

    if ( $oc=='Palm' && $od=='Zpth' )
        $name = "Zire 71";

    if ( $oc=='Palm' && $od=='Zi72' )
        $name = "Zire 71";

    if ( $oc=='palm' && $od=='MT64' )
        $name = "Tungsten C";

    if ( $oc=='palm' && $od=='atc1' )
        $name = "Tungsten W";

    if ( $oc=='Palm' && $od=='Cct1' )
        $name = "Tungsten E";

    if ( $oc=='Palm' && $od=='Frg1' )
        $name = "Tungsten T";

    if ( $oc=='Palm' && $od=='Frg2' )
        $name = "Tungsten T2";

    if ( $oc=='Palm' && $od=='Arz1' )
        $name = "Tungsten T3";

    return $name;
}

?>
