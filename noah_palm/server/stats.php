<html>

<head>
<title>Stats for iNoah/Palm</title>
<link rel="stylesheet" type="text/css" href="main.css" />
</head>

<body>

<?php

# auth settings for the database, must be set before importing ez_mysql.php
define( 'DBHOST', 'localhost' );
define( 'DBUSER', 'root' );
# define( 'DBPWD', 'password' );
define( 'DBNAME', 'inoahdb' );

require_once("ez_mysql.php");

require_once("common.php");

function cmp_stats($a, $b)
{
    if ($a[1]== $b[1]) return 0;
    return ($b[1] > $a[1]) ? 1 : -1;
}

function get_device_stats()
{
    global $dict_db;

    $query = "SELECT DISTINCT dev_info from cookies;";
    $rows = $dict_db->get_results($query);

    $stats_assoc = array();
    foreach ($rows as $row)
    {
        $dev_info = $row->dev_info;
        $dev_info_decoded = decode_di($dev_info);
        $device_name = $dev_info_decoded['device_name'];
        if ( isset($stats_assoc[$device_name]) )
            $stats_assoc[$device_name] += 1;
        else
            $stats_assoc[$device_name] = 1;
    }

    $stats = array();

    $device_names = array_keys($stats_assoc);
    foreach ( $device_names as $device_name )
    {
        $count = $stats_assoc[$device_name];
        array_push($stats, array($device_name, $count) );
    }
    usort($stats, 'cmp_stats');
    return $stats;
}

function total_and_day_avg($total, $num_days)
{
    $avg = $total / $num_days;
    $avg = sprintf("%.2f", $avg);
    echo $total . " ($avg per day)";
}

function day_or_days($num)
{
    if ($num==1) return "day";
    return "days";
}

function aveg_txt($total,$num)
{
    $avg = $total / $num;
    $avg = sprintf("%.2f", $avg);
    return $avg;
}

function echo_aveg($total,$num)
{
    echo aveg_txt($total,$num);
}

function find_lookups_for_date($weekly_lookups_rows, $date)
{
    foreach( $weekly_lookups_rows as $row )
    {
        if ($row->when_date == $date )
            return $row->lookups_count;
    }
    return 0;
}

function weekly_daily_stats($header, $regs_q, $lookups_q, $limit)
{
    global $dict_db;
?>
<table id="stats" cellspacing="0">
<tr class="header">
<?php
  echo "<td>$header</td>\n";
?>
  <td>Regs</td>
  <td>Lookups</td>
</tr>
<?php
    $regs_rows = $dict_db->get_results($regs_q);
    $lookups_rows = $dict_db->get_results($lookups_q);

    $selected = false;
    $rows_count = 0;
    foreach ( $regs_rows as $row )
    {
        $when_date = $row->when_date;
        $regs_count = $row->regs_count;
        $lookups_count = find_lookups_for_date($lookups_rows, $when_date);
        if ($selected)
            echo "<tr class=\"selected\">\n";
        else
            echo "<tr>\n";
        if ( $header=="Day" )
            echo "  <td><a href=\"stats.php?daily=$when_date\">$when_date</a></td>\n";
        else
            echo "  <td>$when_date</td>\n";
        echo "  <td>$regs_count</td>\n";
        if ( $header=="Day" )
            echo "  <td><a href=\"stats.php?daily_lookups=$when_date\">$lookups_count</a></td>\n";
        else
            echo "  <td>$lookups_count</td>\n";
        echo "</tr>\n";
        if ($selected)
            $selected = false;
        else
            $selected = true;
        $rows_count += 1;
        if ( $limit > 0 )
        {
            if ( $rows_count >= $limit )
                break;
        }
    }
    echo "</table>\n";
    return $rows_count;
}

function recent_registrations($limit)
{
    global $dict_db;
?>
<table id="stats" cellspacing="0">
<tr class="header">
  <td>Date</td>
  <td>Name</td>
  <td>Device</td>
</tr>

<?php
    $recent_regs_q = "SELECT cookie, dev_info, reg_code, DATE_FORMAT(when_created,'%Y-%m-%d') as when_created, disabled_p FROM cookies ORDER BY when_created DESC LIMIT $limit;";
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
        $device_name = $dev_info_decoded['device_name'];
        $hotsync_name = 'Unavailable';
        if (isset($dev_info_decoded['HS']))
            $hotsync_name = $dev_info_decoded['HS'];
        echo "  <td>$hotsync_name</td>\n";
        echo "  <td>$device_name</td>\n";
        echo "</tr>\n";
        if ($selected)
            $selected = false;
        else
            $selected = true;
    }
    echo "</table>\n";
}

function show_device_stats()
{
?>
<table id="stats" cellspacing="0">
<tr class="header">
  <td>Device name</td>
  <td>Count</td>
</tr>

<?php
    $stats = get_device_stats();
    $selected = false;

    foreach ( $stats as $stat )
    {
        $device_name = $stat[0];
        $count = $stat[1];

        if ($selected)
            echo "<tr class=\"selected\">\n";
        else
            echo "<tr>\n";
        echo "  <td>$device_name</td>\n";
        echo "  <td>$count</td>\n";
        echo "</tr>\n";
        if ($selected)
            $selected = false;
        else
            $selected = true;
    }
    echo "</table>\n";
}

function recent_lookups($limit)
{
    global $dict_db;
?>
<table id="stats" cellspacing="0">
<tr class="header">
  <td>Word 1</td>
  <td>Word 2</td>
  <td>Word 3</td>
</tr>

<?php
    $limit = $limit * 3;
    $recent_lookups_q = "SELECT word, query_time FROM request_log ORDER BY query_time DESC LIMIT $limit;";
    $recent_lookups_rows = $dict_db->get_results($recent_lookups_q);

    $selected = false;
    $which_word = 1;
    foreach ( $recent_lookups_rows as $row )
    {
        if ( $which_word == 1 )
            $word1 = $row->word;
        if ( $which_word == 2 )
            $word2 = $row->word;
        if ( $which_word == 3 )
            $word3 = $row->word;
        $which_word += 1;

        if ($which_word == 4)
        {
            $which_word = 1;
            if ($selected)
                echo "<tr class=\"selected\">\n";
            else
                echo "<tr>\n";
            echo "  <td>$word1</td>\n";
            echo "  <td>$word2</td>\n";
            echo "  <td>$word3</td>\n";
            echo "</tr>\n";
            if ($selected)
                $selected = false;
            else
                $selected = true;
        }
    }
    echo "</table>\n";
}

function summary()
{
    global $dict_db;

    echo "<b>Summary</b>&nbsp;|&nbsp;";
    echo "<a href=\"stats.php?active_users=1\">Active users</a>";
    echo "<p>";

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
?>

    iNoah has been published for <?php echo $num_days . " " . day_or_days($num_days) ?>. <br>
    Unique cookies created: <?php total_and_day_avg($unique_cookies, $num_days) ?>. <br>
    Unique devices registered: <?php total_and_day_avg($unique_devices, $num_days) ?>. <br>
    Total requests: <?php total_and_day_avg($total_requests, $num_days) ?> which is 
    <?php echo_aveg($total_requests,$unique_devices) ?> per unique device (unique user?). <br>
    <p>

    <table>
    <tr>
      <td>Weekly stats</td>
      <td>Daily stats</td>
    </tr>
    <tr>

    <?php
        echo "<td>\n";
        $weekly_regs_q = "select DATE_FORMAT(when_created, '%Y-%U') as when_date, count(*) as regs_count from cookies group by when_date order by when_date desc;";
        $weekly_lookups_q = "select DATE_FORMAT(query_time, '%Y-%U') as when_date, count(*) as lookups_count from request_log group by when_date order by when_date desc;";
        $rows_count = weekly_daily_stats("Week", $weekly_regs_q, $weekly_lookups_q, -1);
        echo "</td>\n";

        echo "<td>\n";
        $daily_regs_q = "select DATE_FORMAT(when_created, '%Y-%m-%d') as when_date, count(*) as regs_count from cookies group by when_date order by when_date desc;";
        $daily_lookups_q = "select DATE_FORMAT(query_time, '%Y-%m-%d') as when_date, count(*) as lookups_count from request_log group by when_date order by when_date desc;";
        weekly_daily_stats("Day", $daily_regs_q, $daily_lookups_q, $rows_count);
        echo "</td>\n";
    ?>
    </tr>
    </table>
    <p>

    <table>
    <tr>
      <td>Recent registrations</td>
      <td>Recent lookups</td>
    </tr>
    <tr>
    <?php
        $limit = 20;
        echo "<td>\n";
        recent_registrations($limit);
        echo "</td>\n";

        echo "<td>\n";
        recent_lookups($limit);
        echo "</td>\n";
    ?>
    </tr>
    </table>

    <?php
        show_device_stats();
}

# show daily lookup stats i.e. words that have been looked up today
function daily_lookups_stats($date)
{
    global $dict_db;

    echo '<a href="stats.php">Summary</a>&nbsp;|&nbsp;';
    echo "<a href=\"stats.php?daily=$date\">Daily per user stats for $date</a>&nbsp;|&nbsp;";
    echo "<b>Daily lookup stats for $date&nbsp;</b>";

    echo "<p>";

    $today_lookups_q = "SELECT word,cookie,TO_DAYS(query_time) as today_day_no FROM request_log WHERE DATE_FORMAT(query_time, '%Y-%m-%d')='$date';";
    $today_lookups_rows = $dict_db->get_results($today_lookups_q);

?>
    <table id="stats" cellspacing="0">
    <tr class="header">
      <td>Word requested</td>
      <td>Word found</td>
      <td>User</td>
      <td>Total requests</td>
      <td>Days registered
      <td>Lookups per day</td>
    </tr>
<?php
    $selected = false;
    foreach ( $today_lookups_rows as $row )
    {
        $word_requested = $row->word;
        $today_day_no = $row->today_day_no;
        $cookie = $row->cookie;

        $dev_info_q = "SELECT dev_info,TO_DAYS(when_created) AS reg_day_no FROM cookies WHERE cookie='$cookie';";
        $rows = $dict_db->get_results($dev_info_q);

        $reg_day_no = $rows[0]->reg_day_no;
        $days_registered = $today_day_no-$reg_day_no+1;

        $dev_info = $rows[0]->dev_info;
        $dev_info_decoded = decode_di($dev_info);
        $device_name = $dev_info_decoded['device_name'];
        $hotsync_name = 'Unavailable';
        if (isset($dev_info_decoded['HS']))
            $hotsync_name = $dev_info_decoded['HS'];
        
        $word_found = "";
        $word_row = get_word_row_try_stemming($word_requested);
        if ($word_row)
            if ($word_row->word != $word_requested)
                $word_requested = $word_row->word;

        $total_lookups_q = "SELECT COUNT(*) FROM request_log WHERE cookie='$cookie'";
        $total_lookups_count = $dict_db->get_var($total_lookups_q);

        $lookups_per_day = aveg_txt($total_lookups_count,$days_registered);

        if ($selected)
            echo "<tr class=\"selected\">\n";
        else
            echo "<tr>\n";

        if ($selected)
            echo "<tr class=\"selected\">\n";
        else
            echo "<tr>\n";
        if ($word_row)
            echo "  <td>$word_requested</td>\n";
        else
            echo "  <td><font color=\"red\">$word_requested</font></td>\n";
        echo "  <td>$word_found</td>\n";
        echo "  <td><a href=\"stats.php?user=$cookie\">$hotsync_name</a></td>\n";
        echo "  <td>$total_lookups_count</td>\n";
        echo "  <td>$days_registered</td>\n";
        echo "  <td>$lookups_per_day</td>\n";
        echo "</tr>\n";
        if ($selected)
            $selected = false;
        else
            $selected = true;
    }
    exit;
}

# show daily stats for users who looked up something, ordered by count
function daily_stats($date)
{
    global $dict_db;

    echo '<a href="stats.php">Summary</a>&nbsp;|&nbsp;';
    echo "<b>Daily per user stats for $date</b>&nbsp;|&nbsp;";
    echo "<a href=\"stats.php?daily_lookups=$date\">Daily lookup stats for $date&nbsp;</a>";
    echo "<p>";
?>
    <table id="stats" cellspacing="0">
    <tr class="header">
      <td>User</td>
      <td>Device</td>
      <td>Today</td>
      <td>Total</td>
      <td>Days registered</td>
      <td>Lookups per day</td>

    </tr>

<?php

    # show lookups today per user
    $user_lookups_q = "SELECT COUNT(cookie) AS cnt,cookie,TO_DAYS(query_time) as today_day_no FROM request_log WHERE DATE_FORMAT(query_time, '%Y-%m-%d')='$date' GROUP BY cookie ORDER BY cnt DESC;";
    $user_lookups_rows = $dict_db->get_results($user_lookups_q);

    $selected = false;
    foreach ( $user_lookups_rows as $row )
    {
        $today_lookups_count = $row->cnt;
        $cookie = $row->cookie;
        $today_day_no = $row->today_day_no;

        $dev_info_q = "SELECT dev_info,TO_DAYS(when_created) AS reg_day_no FROM cookies WHERE cookie='$cookie'";
        $rows = $dict_db->get_results($dev_info_q);
        $dev_info = $rows[0]->dev_info;
        $reg_day_no = $rows[0]->reg_day_no;

        $days_registered = $today_day_no-$reg_day_no+1;

        $dev_info_decoded = decode_di($dev_info);
        $device_name = $dev_info_decoded['device_name'];
        $hotsync_name = 'Unavailable';
        if (isset($dev_info_decoded['HS']))
            $hotsync_name = $dev_info_decoded['HS'];

        $total_lookups_q = "SELECT COUNT(*) FROM request_log WHERE cookie='$cookie'";
        $total_lookups_count = $dict_db->get_var($total_lookups_q);

        $lookups_per_day = aveg_txt($total_lookups_count,$days_registered);

        if ($selected)
            echo "<tr class=\"selected\">\n";
        else
            echo "<tr>\n";

        echo "  <td><a href=\"stats.php?user=$cookie\">$hotsync_name</a></td>\n";
        echo "  <td>$device_name</td>\n";
        echo "  <td>$today_lookups_count</td>\n";
        echo "  <td>$total_lookups_count</td>\n";
        echo "  <td>$days_registered</td>\n";
        echo "  <td>$lookups_per_day</td>\n";
        echo "</tr>\n";
        if ($selected)
            $selected = false;
        else
            $selected = true;
    }

    echo "</table>\n";
    exit;
}

function active_users_stats()
{
    global $dict_db;

    echo '<a href="stats.php">Summary</a>&nbsp;|&nbsp;';
    echo "<b>Active users</b>";
    echo "<p>";

    $active_users_q = "SELECT COUNT(cookie) AS cnt, cookie FROM request_log GROUP BY cookie HAVING cnt>9;";
    $active_users_rows = $dict_db->get_results($active_users_q);
    $active_users = count($active_users_rows);

    echo "Active users (made than 9 requests): $active_users \n<p>";
?>
    <table id="stats" cellspacing="0">
    <tr class="header">
      <td>User</td>
      <td>Device</td>
      <td>Total</td>
      <td>Days registered</td>
      <td>Lookups per day</td>
    </tr>

<?php
    $today_day_no_q = "SELECT TO_DAYS(NOW());";
    $today_day_no = $dict_db->get_var($today_day_no_q);

    $user_lookups_q = "SELECT COUNT(cookie) AS cnt,cookie FROM request_log GROUP BY cookie ORDER BY cnt DESC;";
    $user_lookups_rows = $dict_db->get_results($user_lookups_q);

    $selected = false;
    foreach ( $user_lookups_rows as $row )
    {
        $total_lookups_count = $row->cnt;
        $cookie = $row->cookie;

        if ( $total_lookups_count < 10 )
            break;

        $dev_info_q = "SELECT dev_info,TO_DAYS(when_created) AS reg_day_no FROM cookies WHERE cookie='$cookie'";
        $rows = $dict_db->get_results($dev_info_q);
        $dev_info = $rows[0]->dev_info;
        $reg_day_no = $rows[0]->reg_day_no;

        $days_registered = $today_day_no-$reg_day_no+1;

        $dev_info_decoded = decode_di($dev_info);
        $device_name = $dev_info_decoded['device_name'];
        $hotsync_name = 'Unavailable';
        if (isset($dev_info_decoded['HS']))
            $hotsync_name = $dev_info_decoded['HS'];

        $total_lookups_q = "SELECT COUNT(*) FROM request_log WHERE cookie='$cookie'";
        $total_lookups_count = $dict_db->get_var($total_lookups_q);

        $lookups_per_day = aveg_txt($total_lookups_count,$days_registered);

        if ($selected)
            echo "<tr class=\"selected\">\n";
        else
            echo "<tr>\n";

        echo "  <td><a href=\"stats.php?user=$cookie\">$hotsync_name</a></td>\n";
        echo "  <td>$device_name</td>\n";
        echo "  <td>$total_lookups_count</td>\n";
        echo "  <td>$days_registered</td>\n";
        echo "  <td>$lookups_per_day</td>\n";
        echo "</tr>\n";
        if ($selected)
            $selected = false;
        else
            $selected = true;
    }

    echo "</table>\n";
    exit;
}


function user_stats($cookie)
{
    global $dict_db;

    echo '<a href="stats.php">Summary</a>&nbsp;|&nbsp;';
    echo "<b>User info for user with cookie $cookie</b>";
    echo "<p>";
    exit;
}


$action = 'summary';
if ( isset( $HTTP_GET_VARS['daily'] ) )
{
    $action = 'daily';
    $date = $HTTP_GET_VARS['daily'];
}

if ( isset( $HTTP_GET_VARS['daily_lookups'] ) )
{
    $action = 'daily_lookups';
    $date = $HTTP_GET_VARS['daily_lookups'];
}

if ( isset( $HTTP_GET_VARS['user'] ) )
{
    $action = 'user';
    $cookie = $HTTP_GET_VARS['user'];
}

if ( isset( $HTTP_GET_VARS['active_users'] ) )
{
    $action = 'active_users';
}

$dict_db = new inoah_db(DBUSER, '', DBNAME, DBHOST);

if ($action == 'summary')
    summary();
elseif ($action == 'daily')
    daily_stats($date);
elseif ($action == 'daily_lookups')
    daily_lookups_stats($date);
elseif ($action == 'user')
    user_stats($cookie);
elseif ($action == 'active_users')
    active_users_stats();
else
{
    # shouldnt happen
    summary();
}    
?>


</body>
</html>

