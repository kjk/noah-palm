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
    print "Error number $err occured on the server.";
    exit;
}

# write MSG response to returning stream
function write_MSG( $msg )
{
    print "MESSAGE\n";
    print $msg;
}

# write DEF response to returning stream
function write_DEF( $word, $def )
{
    print "DEF\n";
    print $word;
    print "\n";
    print $def;
}

function write_WORDLIST( $list )
{
    print "WORDLIST\n";
    print $list;
}

function serve_get_cookie($di)
{
    # TODO:
    # generate a new random, unique cookie
    # insert cookie and $di into cookies table
    # return the cookie to the client
    print "COOKIE\n";
    $magic_cookie = "berake";
    print $magic_cookie;
    exit;
}

function serve_register($reg_code)
{
    if ( $reg_code == 'valid' )
        write_MSG("Registration has been succesful. Enjoy!");
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

    write_DEF($word, $def);
    exit;
}

function get_word_def($word)
{
    global $dict_db;
    $query = "SELECT def FROM words WHERE word='$word';";
    $def = $dict_db->get_var($query); 
    return $def;
}

function serve_get_word($word)
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

    $def = get_word_def($word);

    if ( $def != "" )
    {
        write_DEF($word,$def);
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
            write_DEF($word,$def);
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
            write_DEF($word,$def);
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
            write_DEF($word,$def);
            exit;
        }
    }

    # didnt find the word
    write_MSG("Definition of word $word not found");
    exit;
}

# check if $device_id is in proper format
function validate_di($device_id)
{
    # TODO

}

# check if $cookie is a valid cookie (was assigned by us and is not disabled)
function validate_cookie($cookie)
{
    # TODO
    if ( $cookie == 'invalid' )
    {
        write_MSG( "Invalid cookie of value '$cookie'");
        exit;
    }
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

$get_word = $HTTP_GET_VARS['get_word'];
if ( !is_null($get_word) )
    serve_get_word($get_word);

report_error(ERR_UKNOWN_REQUEST);
?>

