<?php
global $base_url;
global $user;

if (!user_access('access center dashboard')) {
    exit;
}

$center_id = isset($_SESSION['center_id']) ? $_SESSION['center_id'] : "";

if ($center_id == "" || empty($center_id)) {
    echo "<div style='text-align:center;'><h2>Center id missing.</h2></div>";
    exit;
}

$_SESSION['user_password'] = $user->pass;
$_SESSION['user_id'] = $user->uid;
$_SESSION['user_name'] = $user->name;
$_SESSION['base_url'] = $base_url;

$session_id = trim(session_id());
$dashboard_page_url = 'http://admin.win/CenterTeacher.php?session_id=' . $session_id;
if ($session_id != "" && !empty($session_id)) {
    echo '<iframe  id ="myframe" src="' . $dashboard_page_url . '" frameborder="0" style="border:0;width:100%; height:100%;position: fixed;left:-1px; margin-top:-8px;"></iframe>';
}
