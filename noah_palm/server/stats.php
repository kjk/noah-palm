<html>
<body>

<?php

require( "dbsettings.inc" );

require_once("ez_mysql.php");

$dict_db = new inoah_db(DBUSER, '', DBNAME, DBHOST);

$unique_cookies_query = "SELECT COUNT(DISTINCT cookie) FROM cookies;";
$unique_devices_query = "SELECT COUNT(DISTINCT dev_info) FROM cookies;";

$unique_cookies = $dict_db->get_var($unique_cookies_query);
$unique_devices = $dict_db->get_var($unique_devices_query);

$first_cookie_creation_q = "SELECT when_created FROM cookies ORDER BY when_created ASC LIMIT 1;";
$last_cookie_creation_q = "SELECT when_created FROM cookies ORDER BY when_created DESC LIMIT 1;";

$first_cookie_creation_time = $dict_db->get_var($first_cookie_creation_q);
$last_cookie_creation_time = $dict_db->get_var($last_cookie_creation_q);
$num_days_q = "SELECT TO_DAYS($last_cookie_creation_time)-TO_DAYS($first_cookie_creation_time);";
$num_days = $dict_db->get_var($num_days_q);
$num_days += 2;

function total_and_day_avg($total)
{
    global $num_days;
    $avg = $total / $num_days;
    echo $total . " ($avg per day)";
}

function day_or_days($num)
{
    if ($num==1) return "day";
    return "days";
}

?>

iNoah has been published for <?php echo $num_days . " " . day_or_days($num_days) ?>. <br>
Unique cookies created so far: <?php total_and_day_avg($unique_cookies) ?>. <br>
Unique devices registered so far: <?php total_and_day_avg($unique_devices) ?>. <br>

</body>
</html>

