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

# each di tag consists of tag name and tag value
# known tag names are:
#  HS - hex-bin encoded hotsync name
#  SN - hex-bin encoded device serial number (if exists)
#  PN - hex-bin encoded phone number (if exists)
#  OC - hex-bin encoded OEM company ID
#  OD - hex-bin encoded OEM device ID
function is_valid_di_tag($tag)
{

    if ( strlen($tag)<2 )
        return false;

    $tag_name  = substr($tag,0,2);

    if ( $tag_name == "HS" )
        return true;

    if ( $tag_name == "SN" )
        return true;

    if ( $tag_name == "HS" )
        return true;

    if ( $tag_name == "PN" )
        return true;

    if ( $tag_name == "OC" )
        return true;

    if ( $tag_name == "OD" )
        return true;

    $tag_value_encoded = substr($tag,2);
    $tag_value = decode_di_tag_value($tag_value_encoded);
    if ( $tag_value )
        return true;

    return false;
}

# given OEM Company ID (oc) and OEM Device id (od)
# return a name of Palm device.
# Based on data from http://homepage.mac.com/alvinmok/palm/codenames.html
# and http://www.mobilegeographics.com/dev/devices.php
function get_device_name_by_oc_od($oc, $od)
{
    if ( $oc=='hspr' && $od=='H101' )
        return "Treo 600";

    if ( $oc=='sony' && $od=='mdna' )
        return "PEG-T615C";

    if ( $oc=='sony' && $od=='prmr' )
        return "PEG-UX50";

    if ( $oc=='sony' && $od=='atom' )
        return "PEG-TH55";

    if ( $oc=='smsn' && $od=='blch' )
        return "SPH-i500";

    if ( $oc=='smsn' && $od=='Phx2' )
        return "SPH-I330";

    if ( $oc=='palm' && $od=='MT64' )
        return "Tungsten C";

    return "Unknown ($oc/$od)";
}

function decode_di($device_id)
{
    $tags = explode(":", $device_id);
    $ret_arr = array();
    $out_str = "";
    foreach( $tags as $tag )
    {
        if ( !is_valid_di_tag($tag) )
            return "INVALID because not is_valid_di_tag($tag)";

        $tag_name  = substr($tag,0,2);
        $tag_value_encoded = substr($tag,2);
        $tag_value = decode_di_tag_value($tag_value_encoded);
        if ( ! $tag_value )
            return "INVALID because not decode_di_tag_value($tag_value_encoded,$tag_value)";
        $out_str .= "$tag_name: $tag_value, ";
        $ret_arr[$tag_name] = $tag_value;
    }
    /* return $out_str; */
    if ( isset($ret_arr['OC']) && isset($ret_arr['OD']) )
    {
        $device_name = get_device_name_by_oc_od( $ret_arr['OC'], $ret_arr['OD'] );
        $ret_arr['device_name'] = $device_name;
    }
    return $ret_arr;
}

?>
