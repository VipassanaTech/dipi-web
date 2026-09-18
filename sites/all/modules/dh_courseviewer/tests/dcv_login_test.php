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

echo empty($GLOBALS['fail']) ? "ALL PASS\n" : "FAILURES\n";
