<?php
// sites/all/modules/dh_courseviewer/tests/dcv_students_test.php
define('DRUPAL_ROOT', '/dhamma/web/dipinew'); chdir(DRUPAL_ROOT);
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
require_once DRUPAL_ROOT . '/sites/all/modules/dh_courseviewer/inc/dcv-bundle.inc';
function ok($c, $m) { echo ($c ? "PASS" : "FAIL") . " - $m\n"; if (!$c) { $GLOBALS['fail'] = 1; } }

$students = dcv_course_students(9900002);
ok(is_array($students), 'students is array');
ok(count($students) === 320, 'fixture has 320 attended students'); // verified 2026-09-18
$first = reset($students);
$id = key($students);
ok(is_string($id) && ctype_digit($id), 'keyed by string student id');
foreach (array('name','gender','age','language','old_student','type','health','meditation_history','special_requests','accommodation','flags') as $k) {
  ok(array_key_exists($k, $first), "student has field: $k");
}
ok(is_array($first['health']) && array_key_exists('physical', $first['health']), 'health is nested');
ok(is_array($first['flags']), 'flags is array');
ok($first['type'] === 'Student', 'type is Student');
ok(is_int($first['age']) || $first['age'] === null, 'age is int or null');

echo empty($GLOBALS['fail']) ? "ALL PASS\n" : "FAILURES\n";
