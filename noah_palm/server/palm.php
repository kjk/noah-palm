<?php

define( 'ERR_NO_PV',             1);
define( 'ERR_NO_CV',             2);
define( 'ERR_INVALID_CV',        3);
define( 'ERR_INVALID_PV',        4);
define( 'ERR_NO_COOKIE',         5);
define( 'ERR_UKNOWN_REQUEST',    6);
define( 'ERR_NO_DI',             7);
define( 'ERR_INVALID_DI',        8); # di (device id) that client sent doesnt correspond to a known format
define( 'ERR_RANDOM_NOT_EMPTY',  9);
define( 'ERR_DB_CONNECT_FAIL',  10);
define( 'ERR_DB_SELECTDB_FAIL', 11);
define( 'ERR_DB_SELECT_FAIL',   12);
define( 'ERR_RECENT_LOOKUPS_NOT_EMPTY', 13);
define( 'ERR_COOKIE_UNKNOWN',   14);  # client sent a cookie that we didnt generate
define( 'ERR_COOKIE_DISABLED',  15);  # client sent a cookie that we disabled for some reason

define( 'TOTAL_REQUESTS_LIMIT', 10);  # how many total requests we allow for non-registered users
define( 'DAILY_REQUESTS_LIMIT',  2);  # how many daily requests we allow for non-registered users

require( "dbsettings.inc" );

require_once("ez_mysql.php");

require_once("common.php");

$dict_db = new inoah_db(DBUSER, '', DBNAME, DBHOST);

$f_force_upgrade = false;

# write MSG response to returning stream
function write_MSG( $msg )
{
    print "MESSAGE\n";
    print $msg;
}

function validate_protocol_version( $pv )
{
    if ($pv != '1')
        report_error(ERR_INVALID_PV);
}

function validate_client_version( $cv )
{
    global $f_force_upgrade;
    if ( $f_force_upgrade && $cv!='1' )
    {
        write_MSG("You're using an older version $cv of the client. Please upgrade to the latest 1.0 version by downloading it from http://www.arslexis.com");
        exit;
    }
    if ( ($cv=='0.5') || ($cv=='1') )
        return;
    report_error(ERR_INVALID_CV);
}

function report_error( $err )
{
    print "ERROR\n";
    print "Error number $err occured on the server. Please contact support@arslexis.com for more information.";
    exit;
}

# write DEF response to returning stream
# if $requests_left < 0 => don't write it
function write_DEF( $word, $pron, $def, $requests_left )
{
    global $client_version;

    print "DEF\n";
    print "$word\n";

    if ( $client_version!='0.5' )
    {
        if ( $pron )
        {
            $pron = strtolower($pron);
            print "PRON $pron\n";
        }
    }
    if ($requests_left>=0)
    {
        print "REQUESTS_LEFT $requests_left\n";
    }
    print $def;
}

function write_WORDLIST( $list )
{
    print "WORDLIST\n";
    print "$list\n";
}

function report_registration_failed()
{
    print "REGISTRATION_FAILED\n";
    exit;
}

function report_registration_ok()
{
    print "REGISTRATION_OK\n";
    exit;
}


function report_no_lookups_left()
{
    $total_limit = TOTAL_REQUESTS_LIMIT;
    $daily_limit = DAILY_REQUESTS_LIMIT;
    write_MSG("You've reached the lookup limit for the trial version. Trial version allows allows unlimited random word lookups but only $daily_limit daily lookups. Please register iNoah at http://www.arslexis.com.");
    exit;
}

define( 'COOKIE_LEN', 12);

function generate_cookie()
{
    global $cookie_letters_len, $dict_db;
    $cookie_letters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    $cookie_letters_len = strlen($cookie_letters);
    $cookie = "";
    for ($i=0; $i<COOKIE_LEN; $i++)
    {
        $letter_pos = rand(0,$cookie_letters_len-1);
        $letter = $cookie_letters[$letter_pos];
        $cookie .= $letter;
    }
    return $cookie;
}

function generate_unique_cookie()
{
    global $dict_db;
    do {
        $cookie = generate_cookie();
        $found_cookie = $dict_db->get_var("SELECT cookie FROM cookies WHERE cookie='$cookie';");
    } while ( $found_cookie );
    return $cookie;
}

function serve_get_cookie($di)
{
    global $dict_db;
    $cookie = generate_unique_cookie();
    print "COOKIE\n";
    print $cookie;

    $cookie = $dict_db->escape($cookie);
    $query = "INSERT INTO cookies (cookie,dev_info,reg_code,when_created, disabled_p) VALUES ('$cookie', '$di', NULL, CURRENT_TIMESTAMP(),'f');";
    $dict_db->query($query);
    exit;
}

function user_registered_p($cookie)
{
    global $dict_db;
    $cookie = $dict_db->escape($cookie);
    $query = "SELECT reg_code FROM cookies WHERE cookie='$cookie';";
    $reg_code = $dict_db->get_var($query);
    if ($reg_code)
        return true;
    else
        return false;
}

# return true if a given registration code is valid (i.e. is in our reg_code
# table and is not flagged as disabled)
function f_reg_code_valid($reg_code)
{
    global $dict_db;

    $reg_code = $dict_db->escape($reg_code);
    $query = "SELECT reg_code FROM reg_codes WHERE reg_code='$reg_code' AND disabled_p='f';";
    $reg_code = $dict_db->get_var($query);
    if ( $reg_code )
        return true;
    else
        return false;
}

# check if a registration code is valid. If it isn't, send a messag to the client
# and exit. if it is valid, do nothing - caller assumes it works
function check_reg_code($reg_code)
{
    if ( ! f_reg_code_valid($reg_code) )
        report_registration_failed();
}

# check if reg_code is valid. Return apropriate response to the client
# (a MSG to be displayed)
function serve_register($reg_code)
{
    if ( f_reg_code_valid($reg_code) )
        report_registration_ok();
    else
        report_registration_failed();
    exit;
}

function get_words_count()
{
    global $dict_db;

    $query = "SELECT COUNT(*) FROM words;";
    $words_count = $dict_db->get_var( $query );
    return $words_count;
}

function serve_get_random_word()
{
    global $dict_db;

    $word_no = rand( 1, get_words_count()-1);
    $query = "SELECT def,word,pron FROM words WHERE word_no='$word_no';";
    $word_row = $dict_db->get_row( $query );

    $def  = $word_row->def;
    $word = $word_row->word;
    $pron = $word_row->pron;

    write_DEF($word, $pron, $def, -1);
    exit;
}

# TODO: handle $reg_code
function log_get_word_request($cookie,$word,$reg_code)
{
    global $dict_db;
    $cookie = $dict_db->escape($cookie);
    $word = $dict_db->escape($word);
    $query = "INSERT INTO request_log (cookie,word,query_time) VALUES ('$cookie','$word',CURRENT_TIMESTAMP());";
    $dict_db->query( $query );
}

function get_total_requests_for_cookie($cookie)
{
    global $dict_db;
    $cookie = $dict_db->escape($cookie);
    $query = "SELECT COUNT(*) FROM request_log WHERE cookie='$cookie';";
    $requests_count = $dict_db->get_var($query);
    return $requests_count;
}

function get_today_requests_for_cookie($cookie)
{
    global $dict_db;
    $cookie = $dict_db->escape($cookie);
    $query = "SELECT COUNT(*) FROM request_log WHERE cookie='$cookie' AND DATE_FORMAT(query_time,'%Y-%m-%d')=DATE_FORMAT(CURRENT_TIMESTAMP(),'%Y-%m-%d');";
    $requests_count = $dict_db->get_var($query);
    return $requests_count;
}

# if $reg_code is an empty string => unregistered lookup
function serve_get_word($cookie,$word,$reg_code)
{
/*
    if ( $word=='wl' )
    {
        write_WORDLIST( "word one\nand another\nand even more\nword three\ntest\nsample\n" );
        exit;
    }

    if ( $word=='tm' )
    {
        write_MSG( "This is a test message. Do you hear me now?" );
        exit;
    }
*/

    $requests_left = -1;
    if (!$reg_code)
    {
        $total_requests = get_total_requests_for_cookie($cookie);
        $requests_left = TOTAL_REQUESTS_LIMIT-$total_requests+DAILY_REQUESTS_LIMIT;
        if ($requests_left<=0)
        {
            $today_requests = get_today_requests_for_cookie($cookie);
            $requests_left = DAILY_REQUESTS_LIMIT-$today_requests;
            if ($requests_left<=0)
                report_no_lookups_left();
        }
        else
            $requests_left = TOTAL_REQUESTS_LIMIT-$total_requests+DAILY_REQUESTS_LIMIT;
    }

    log_get_word_request($cookie, $word, $reg_code);

    $word_row = get_word_row_try_stemming($word);
    if ( $word_row )
        write_DEF($word_row->word, $word_row->pron,$word_row->def,$requests_left);
    else
        write_MSG("Definition of word '$word' has not been found");
    exit;
}

define( 'MAX_RECENT_LOOKUPS', 20 );

# return a list of recently looked up words by user using cookie $cookie
# TODO: use TOP in sql query to limit the number of lookups. should be faster
function serve_recent_lookups($cookie)
{
    global $dict_db;

    $words = "";
    $query = "SELECT DISTINCT word FROM request_log WHERE cookie='$cookie' ORDER BY query_time;";
    $rows = $dict_db->get_results($query);
    $words_count = 0;
    foreach ( $rows as $row )
    {
        $word = $row->word;
        $words .= "$word\n";
        $words_count += 1;
        if ( $words_count > MAX_RECENT_LOOKUPS )
            break;        
    }
    write_WORDLIST($words);
    exit;
}

# check if $device_id is in proper format
function validate_di($device_id)
{
    $tags = explode(":", $device_id);

    foreach( $tags as $tag )
    {
        if ( !is_valid_di_tag($tag) )
        {
            report_error(ERR_INVALID_DI);
            exit;
        }
    }
}

# check if $cookie is a valid cookie (was assigned by us and is not disabled)
# TODO: log invalid cookies ?
function validate_cookie($cookie)
{
    global $dict_db;

    $query = "SELECT cookie, disabled_p FROM cookies WHERE cookie='$cookie';";
    $row = $dict_db->get_row($query);
    if ( $row )
    {
        if ( $row->disabled_p == "t" )
        {
            report_error(ERR_COOKIE_DISABLED);
        }
    }
    else
    {
        report_error(ERR_COOKIE_UNKNOWN);
    }
    # everything went fine so the cookie is ok
}

if ( !isset($HTTP_GET_VARS['pv']) )
    report_error(ERR_NO_PV);
$protocol_version = $HTTP_GET_VARS['pv'];
if ( empty($protocol_version) )
    report_error(ERR_NO_PV);
validate_protocol_version($protocol_version);

if ( !isset($HTTP_GET_VARS['cv']) )
    report_error(ERR_NO_CV);
$client_version = $HTTP_GET_VARS['cv'];
if ( empty($client_version) )
    report_error(ERR_NO_CV);
validate_client_version($client_version);

# the only request that doesnt require cookie
if ( isset( $HTTP_GET_VARS['get_cookie'] ) )
{
    if ( !isset($HTTP_GET_VARS['di']) )
        report_error(ERR_NO_DI);
    $device_id = $HTTP_GET_VARS['di'];
    if ( empty($device_id) )
        report_error(ERR_NO_DI);
    validate_di($device_id);
    serve_get_cookie($device_id);
}

# all other requests require cookie to be present
if ( !isset($HTTP_GET_VARS['c']) )
    report_error(ERR_NO_COOKIE);
$cookie = $HTTP_GET_VARS['c'];
if ( empty($cookie) )
    report_error(ERR_NO_COOKIE);
validate_cookie($cookie);

if ( isset($HTTP_GET_VARS['register']) )
{
    $reg_code = $HTTP_GET_VARS['register'];
    serve_register($reg_code);
}

if ( isset($HTTP_GET_VARS['get_random_word'] ) )
{
    $get_random_word = $HTTP_GET_VARS['get_random_word'];
    if ($get_random_word != "")
        report_error(ERR_RANDOM_NOT_EMPTY);
    serve_get_random_word();
}

if ( isset($HTTP_GET_VARS['recent_lookups'] ) )
{
    $recent_lookups = $HTTP_GET_VARS['recent_lookups'];
    if ($recent_lookups != "")
        report_error(ERR_RECENT_LOOKUPS_NOT_EMPTY);
    serve_recent_lookups($cookie);
}

if ( isset($HTTP_GET_VARS['get_word']) )
{
    $reg_code = "";
    # we only need to check for registration if we do get_word lookups
    if ( isset($HTTP_GET_VARS['rc']) )
    {
        $reg_code = $HTTP_GET_VARS['rc'];
        check_reg_code($reg_code);
    }

    $get_word = $HTTP_GET_VARS['get_word'];
    serve_get_word($cookie,$get_word,$reg_code);
}

report_error(ERR_UKNOWN_REQUEST);
?>

