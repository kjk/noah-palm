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

    # devnote: yes, $hex_digits is broken but that's how it is on the device now
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

# check if $device_id is in proper format

function decode_di($device_id)
{
    $tags = explode(":", $device_id);

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
    }
    return $out_str;
}

?>
