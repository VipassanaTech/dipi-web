<?php
// sites/all/modules/dh_courseviewer/tests/dcv_login_test.php
// Bootstrapped test. Run: php7.4 <thisfile>
define('DRUPAL_ROOT', '/dhamma/web/dipinew'); chdir(DRUPAL_ROOT);
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
require_once DRUPAL_ROOT . '/sites/all/modules/dh_courseviewer/inc/dcv-api.inc';
function ok($c, $m) { echo ($c ? "PASS" : "FAIL") . " - $m\n"; if (!$c) { $GLOBALS['fail'] = 1; } }

// dcv_login must be defined before testing
ok(function_exists('dcv_login'), 'dcv_login defined');
// user_authenticate is the core primitive dcv_login() relies on.
ok(function_exists('user_authenticate'), 'user_authenticate available');
// Wrong password for admin must fail authentication.
ok(user_authenticate('admin', 'definitely-wrong-pw-xyz') === FALSE, 'bad password rejected');

// Flood control primitives dcv_login() relies on for brute-force throttling.
ok(function_exists('flood_is_allowed'), 'flood_is_allowed available');
ok(function_exists('flood_register_event'), 'flood_register_event available');
ok(function_exists('user_load_by_name'), 'user_load_by_name available');

// Clean-state IP gate must pass (no prior failed attempts registered for this window).
$ip_limit  = variable_get('user_failed_login_ip_limit', 50);
$ip_window = variable_get('user_failed_login_ip_window', 3600);
ok(flood_is_allowed('failed_login_attempt_ip', $ip_limit, $ip_window) === TRUE, 'clean-state IP flood gate passes');

// Register/clear cycle against a throwaway per-user identifier — must not leave residue.
// flood_is_allowed() returns (count-in-window < threshold), so with threshold 2 the
// gate flips to FALSE only once the 2nd event has been registered.
$test_identifier = 'dcv-test-' . getmypid();
flood_clear_event('failed_login_attempt_user', $test_identifier); // ensure clean start
ok(flood_is_allowed('failed_login_attempt_user', 2, 21600, $test_identifier) === TRUE, 'throwaway identifier starts allowed');
flood_register_event('failed_login_attempt_user', 21600, $test_identifier);
ok(flood_is_allowed('failed_login_attempt_user', 2, 21600, $test_identifier) === TRUE, 'still allowed after 1st registration at limit 2');
flood_register_event('failed_login_attempt_user', 21600, $test_identifier);
ok(flood_is_allowed('failed_login_attempt_user', 2, 21600, $test_identifier) === FALSE, 'blocked after 2nd registration at limit 2');
flood_clear_event('failed_login_attempt_user', $test_identifier);
ok(flood_is_allowed('failed_login_attempt_user', 2, 21600, $test_identifier) === TRUE, 'allowed again after flood_clear_event');

echo empty($GLOBALS['fail']) ? "ALL PASS\n" : "FAILURES\n";
