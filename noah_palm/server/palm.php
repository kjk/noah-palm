<?php

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

$sample_def = '!fawn
$n
@young deer
!dun
!grayish brown
!greyish brown
!fawn
$n
@a color varying around light grayish brown
#she wore a dun raincoat
!fawn
$v
@have fawns, of deer
!fawn
!toady
!truckle
!bootlick
!kowtow
!kotow
!suck up
$v
@court favor by cringing or flattering
!fawn
!crawl
!creep
!cringe
!cower
!grovel
$v
@show submission or fear
';

# get definition of $word from the database. Returns an empty string
# if there is no definition.
function get_word_def($word)
{
    $ret = "";
    $conn_id = @mysql_connect(DBHOST, DBUSER, '');
    if ( !$conn_id )
    {
        report_error( ERR_DB_CONNECT_FAIL );
        # return $err_prefix . " couldn't connect. " . mysql_error() ;
    }

    if ( !mysql_select_db("inoahdb") )
    {
        report_err( ERR_DB_SELECTDB_FAIL );
        #return $err_prefix . " couldn't select db. " . mysql_error();
    }

    $query = "SELECT def FROM words WHERE word='$word';";
    $result_id = mysql_query( $query );
    if ( !$result_id)
    {
        report_error( ERR_DB_SELECT_FAIL );
        # return $err_prefix . " select failed, query was=$query. " . mysql_error();
    }
    if ($row = mysql_fetch_row ($result_id))
        $ret = $row[0];
    else
        $ret = "";
    mysql_free_result ($result_id);
    return $ret;
}

function get_words_count()
{
    # TODO: get words count as select count(*) from words;
    return 50000;
}

function get_error_msg($err)
{
    $txt = "ERROR: $err";
    return $txt;
}

function validate_protocol_version( $pv )
{
    # print "protocol version: $pv";
    if ($pv != '1')
        report_error(ERR_INVALID_PV);
}

function validate_client_version( $cv )
{
    # print "client versin: $cv";
    if ($cv != '0.5')
        report_error(ERR_INVALID_CV);
}

function report_error( $err )
{
    print "ERROR";
    print "Error number $err occured on the server.";
    exit;
}

# write MSG response to returning stream
function write_MSG( $msg )
{
    print "MSG";
    print $msg;
}


# write DEF response to returning stream
function write_DEF( $def )
{
    print "DEF";
    print $def;
}

function write_WORDLIST( $list )
{
    print "WORDLIST";
    print $list;
}

function serve_get_cookie($di)
{
    # TODO:
    # generate a new random, unique cookie
    # insert cookie and $di into cookies table
    # return the cookie to the client
    print "COOKIE";
    $magic_cookie = "berake";
    print $magic_cookie;
    exit;
}

function serve_get_random_word()
{
    global $sample_def;
    word_no = rand( 1, get_words_count()-1);
    # TODO: get the word as select def from words where word_no=$word_no
    write_DEF($sample_def);
    exit;
}

function serve_get_word($word)
{
    global $sample_def;

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
    if ( $def == "" )
        write_MSG("Definition of word $word not found");
    else
        write_DEF($def);
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
$cookie = $HTTP_GET_VARS['cookie'];
if ( is_null($cookie) )
    report_error(ERR_NO_COOKIE);

validate_cookie($cookie);

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

