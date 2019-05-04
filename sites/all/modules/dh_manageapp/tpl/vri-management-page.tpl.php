<?php
global $base_url;
global $user;

if (!user_access('access all center dashboard')) {
    exit;
}

unset($_SESSION['center_id']);
$_SESSION['user_password'] = $user->pass;
$_SESSION['user_id'] = $user->uid;
$_SESSION['user_name'] = $user->name;
$_SESSION['base_url'] = $base_url;
$session_id = trim(session_id());

$dashboard_page_url = variable_get('stats_url').'/vri_management.php?session_id=' . $session_id;
if ($user->uid == "" || empty($user->uid)) {
    echo "<div style='text-align:center;'><h2>Invalid User.</h2></div>";
    exit;
} else {
    echo '<iframe  id ="myframe" src="' . $dashboard_page_url . '" frameborder="0" style="border:0;width:100%; height:100%;position: fixed;left:-1px;margin-top:-8px;"></iframe>';
}
