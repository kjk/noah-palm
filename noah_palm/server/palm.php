<?php

# Some testing urls:
# http://dict-pc.arslexis.com:3000/palm.php?pv=1&cv=0.5&c=any_cookie_is_good&get_random_word

define( 'ERR_NO_PV',             1);
define( 'ERR_NO_CV',             2);
define( 'ERR_INVALID_CV',        3);
define( 'ERR_INVALID_PV',        4);
define( 'ERR_NO_COOKIE',         5);
define( 'ERR_UKNOWN_REQUEST',    6);
define( 'ERR_NO_DI',             7);
define( 'ERR_RANDOM_NOT_EMPTY',  8);
define( 'ERR_DB_CONNECT_FAIL',   9);
define( 'ERR_DB_SELECTDB_FAIL', 10);
define( 'ERR_DB_SELECT_FAIL',   11);
define( 'ERR_RECENT_LOOKUPS_NOT_EMPTY', 12);
define( 'ERR_COOKIE_UNKNOWN',   13);  # client sent a cookie that we didnt generate
define( 'ERR_COOKIE_DISABLED',  14);  # client sent a cookie that we disabled for some reason

require( "dbsettings.inc" );

require_once("ez_mysql.php");

$dict_db = new inoah_db(DBUSER, '', DBNAME, DBHOST);

function validate_protocol_version( $pv )
{
    if ($pv != '1')
        report_error(ERR_INVALID_PV);
}

function validate_client_version( $cv )
{
    if ($cv != '0.5')
        report_error(ERR_INVALID_CV);
}

function report_error( $err )
{
    print "ERROR\n";
    print "Error number $err occured on the server. Please contact support@arslexis.com for more information.";
    exit;
}

# write MSG response to returning stream
function write_MSG( $msg )
{
    print "MESSAGE\n";
    print $msg;
}

# write DEF response to returning stream
function write_DEF( $word, $pron, $def )
{
    print "DEF\n";
    print "$word\n";
    if ( $pron )
        print "PRON $pron\n";
    print $def;
}

function write_WORDLIST( $list )
{
    print "WORDLIST\n";
    print "$list\n";
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

    $query = "INSERT INTO cookies (cookie,dev_info,reg_code,disabled_p) VALUES ('$cookie', '$di', NULL, 'f');";
    $dict_db->query($query);
    exit;
}

function user_registered_p($cookie)
{
    global $dict_db;
    $query = ("SELECT reg_code FROM cookies WHERE cookie='$cookie';");
    $reg_code = $dict_db->get_var($query);
    if ($reg_code)
        return true;
    else
        return false;
}

function serve_register($reg_code)
{
    global $dict_db;

    $query = "SELECT reg_code FROM reg_codes WHERE reg_code='$reg_code';";
    $reg_code = $dict_db->get_var($query);
    if ( $reg_code )
        write_MSG("Registration has been succesful. We hope you'll enjoy using Noah.");
    else
        write_MSG("Registration failed");
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
    $query = "SELECT def,word FROM words WHERE word_no='$word_no';";
    $res = $dict_db->get_row( $query );
    $def = $res->def;
    $word = $res->word;

    write_DEF($word, $word, $def);
    exit;
}

function get_word_def($word)
{
    global $dict_db;
    $query = "SELECT def FROM words WHERE word='$word';";
    $def = $dict_db->get_var($query); 
    return $def;
}

function log_get_word_request($cookie,$word)
{
    global $dict_db;
    $query = "INSERT INTO request_log (cookie,word,query_time) VALUES ('$cookie','$word',CURRENT_TIMESTAMP());";
    $dict_db->query( $query );
}

function serve_get_word($cookie,$word)
{
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

    log_get_word_request($cookie, $word);

    $def = get_word_def($word);

    if ( $def != "" )
    {
        write_DEF($word,$word,$def);
        exit;
    }

    # didnt find the exact word, so try a few heuristics

    # if word ends with "s", try to find word without it e.g. "strings" => "string"
    if ( "s" == substr($word,strlen($word)-1) )
    {
        $word = substr($word,0,strlen($word)-1);
        $def = get_word_def($word);
        if ( $def != "" )
        {
            write_DEF($word, NULL,$def);
            exit;
        }
    }

    # if word ends with "ed", try to find word without it e.g. "glued" => "glue"
    if ( "ed" == substr($word,strlen($word)-2) )
    {
        $word = substr($word,0,strlen($word)-2);
        $def = get_word_def($word);
        if ( $def != "" )
        {
            write_DEF($word,$word,$def);
            exit;
        }
    }

    # if word ends with "ing", try to find word without it e.g. "working" => "work"
    if ( "ing" == substr($word,strlen($word)-3) )
    {
        $word = substr($word,0,strlen($word)-3);
        $def = get_word_def($word);
        if ( $def != "" )
        {
            write_DEF($word,$word,$def);
            exit;
        }
    }

    # didnt find the word
    write_MSG("Definition of word $word not found");
    exit;
}

define( 'MAX_RECENT_LOOKUPS', 20 );

# return a list of recently looked up words by user using cookie $cookie
# TODO: use TOP in sql query to limit the number of lookups. should be faster
function serve_recent_lookups($cookie)
{
    global $dict_db;

    $words = "";
    $query = "SELECT word FROM request_log WHERE cookie='$cookie' ORDER BY query_time;";
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
    # TODO

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

$protocol_version = $HTTP_GET_VARS['pv'];
if ( is_null($protocol_version) )
    report_error(ERR_NO_PV);
validate_protocol_version($protocol_version);

$client_version = $HTTP_GET_VARS['cv'];
if ( is_null($client_version) )
    report_error(ERR_NO_CV);
validate_client_version($client_version);

# the only request that doesnt require cookie
$get_cookie = $HTTP_GET_VARS['get_cookie'];
if ( !is_null($get_cookie) )
{
    $device_id = $HTTP_GET_VARS['di'];
    if ( is_null($device_id) )
        report_error(ERR_NO_DI);
    validate_di($device_id);
    serve_get_cookie($device_id);
}

# all other requests require cookie to be present
$cookie = $HTTP_GET_VARS['c'];
if ( is_null($cookie) )
    report_error(ERR_NO_COOKIE);
validate_cookie($cookie);

$reg_code = $HTTP_GET_VARS['register'];
if ( !is_null($reg_code) )
    serve_register($reg_code);

$get_random_word = $HTTP_GET_VARS['get_random_word'];
if ( !is_null($get_random_word) )
{
    if ($get_random_word != "")
        report_error(ERR_RANDOM_NOT_EMPTY);
    serve_get_random_word();
}

$recent_lookups = $HTTP_GET_VARS['recent_lookups'];
if ( !is_null( $recent_lookups ) )
{
    if ($recent_lookups != "")
        report_error(ERR_RECENT_LOOKUPS_NOT_EMPTY);
    serve_recent_lookups($cookie);
}



$get_word = $HTTP_GET_VARS['get_word'];
if ( !is_null($get_word) )
    serve_get_word($cookie,$get_word);

report_error(ERR_UKNOWN_REQUEST);
?>

