<html>

<head>
<title>Stats for iNoah/Palm</title>
<link rel="stylesheet" type="text/css" href="main.css" />
</head>

<body>

<?php

require( "dbsettings.inc" );

require_once("ez_mysql.php");

require_once("common.php");

// $type = ;

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
$num_days += 1;

$total_requests_q = "SELECT COUNT(*) FROM request_log;";
$total_requests = $dict_db->get_var($total_requests_q);

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

function aveg($total,$num)
{
    $avg = $total / $num;
    echo $avg;
}

function stats_for_a_day($day)
{


}

?>

iNoah has been published for <?php echo $num_days . " " . day_or_days($num_days) ?>. <br>
Unique cookies created so far: <?php total_and_day_avg($unique_cookies) ?>. <br>
Unique devices registered so far: <?php total_and_day_avg($unique_devices) ?>. <br>
Total requests so far: <?php total_and_day_avg($total_requests) ?> which is 
<?php aveg($total_requests,$unique_devices) ?> per unique device (unique user?). <br>
<p>
Weekly stats:
<p>
<table id="stats" cellspacing="0">
<tr class="header">
  <td>Week</td>
  <td>Registrations</td>
  <td>Lookups</td>
</tr>
<?php

function find_lookups_for_date($weekly_lookups_rows, $date)
{
    foreach( $weekly_lookups_rows as $row )
    {
        if ($row->when_date == $date )
            return $row->lookups_count;
    }
    return 0;
}

    $weekly_regs_q = "select DATE_FORMAT(when_created, '%Y-%U') as when_date, count(*) as regs_count from cookies group by when_date order by when_date desc;";
    $weekly_regs_rows = $dict_db->get_results($weekly_regs_q);

    $weekly_lookups_q = "select DATE_FORMAT(query_time, '%Y-%U') as when_date, count(*) as lookups_count from request_log group by when_date order by when_date desc;";
    $weekly_lookups_rows = $dict_db->get_results($weekly_lookups_q);

    $selected = false;
    foreach ( $weekly_regs_rows as $row )
    {
        $when_date = $row->when_date;
        $regs_count = $row->regs_count;
        $lookups_count = find_lookups_for_date($weekly_lookups_rows, $when_date);
        if ($selected)
            echo "<tr class=\"selected\">\n";
        else
            echo "<tr>\n";
        echo "  <td>$when_date</td>\n";
        echo "  <td>$regs_count</td>\n";
        echo "  <td>$lookups_count</td>\n";
        echo "</tr>\n";
        if ($selected)
            $selected = false;
        else
            $selected = true;
    }
?>
</table>
<p>
Recent registrations:
<p>
<table id="stats" cellspacing="0">
<?php
    $recent_regs_max = 15;
    $recent_regs_q = "SELECT * FROM cookies ORDER BY when_created DESC LIMIT $recent_regs_max;";
    $recent_regs_rows = $dict_db->get_results($recent_regs_q);

    $selected = false;
    foreach ( $recent_regs_rows as $row )
    {
        $when_created = $row->when_created;
        $dev_info = $row->dev_info;
        $dev_info_decoded = decode_di($dev_info);
        if ($selected)
            echo "<tr class=\"selected\">\n";
        else
            echo "<tr>\n";
        echo "  <td>$when_created</td>\n";
        echo "  <td>$dev_info</td>\n";
        echo "  <td>$dev_info_decoded</td>\n";
        echo "</tr>\n";
        if ($selected)
            $selected = false;
        else
            $selected = true;
    }
?>
</table>

</body>
</html>

