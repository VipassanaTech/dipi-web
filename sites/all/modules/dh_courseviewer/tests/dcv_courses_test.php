<?php
// sites/all/modules/dh_courseviewer/tests/dcv_courses_test.php
define('DRUPAL_ROOT', '/dhamma/web/dipinew'); chdir(DRUPAL_ROOT);
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
require_once DRUPAL_ROOT . '/sites/all/modules/dh_courseviewer/inc/dcv-api.inc';
function ok($c, $m) { echo ($c ? "PASS" : "FAIL") . " - $m\n"; if (!$c) { $GLOBALS['fail'] = 1; } }

ok(function_exists('dcv_user_centres'), 'dcv_user_centres defined');
ok(function_exists('dcv_build_course_list'), 'dcv_build_course_list defined');

// Build the course list for a caller scoped to centre 5. dcv_build_course_list()
// is the pure-ish core of dcv_courses() (takes an explicit centre list so we can
// test without faking a session).
$list = dcv_build_course_list(array(5));
ok(is_array($list), 'course list is array');
ok(count($list) > 0, 'centre 5 has courses');
$ids = array();
foreach ($list as $c) { $ids[] = $c['id']; }
ok(in_array(9900002, $ids), 'fixture course 9900002 present');
$row = null;
foreach ($list as $c) { if ($c['id'] == 9900002) { $row = $c; } }
ok($row['centre'] == 5, 'course centre = 5');
ok($row['start_date'] === '2026-09-30' && $row['end_date'] === '2026-10-03', 'course dates');
ok(array_keys($row) === array('id','name','centre','start_date','end_date'), 'course list field whitelist');

echo empty($GLOBALS['fail']) ? "ALL PASS\n" : "FAILURES\n";
