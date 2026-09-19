<?php
// sites/all/modules/dh_courseviewer/tests/dcv_seats_test.php
define('DRUPAL_ROOT', '/dhamma/web/dipinew'); chdir(DRUPAL_ROOT);
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
require_once DRUPAL_ROOT . '/sites/all/modules/dh_courseviewer/inc/dcv-bundle.inc';
function ok($c, $m) { echo ($c ? "PASS" : "FAIL") . " - $m\n"; if (!$c) { $GLOBALS['fail'] = 1; } }

$seats = dcv_course_seats(9900002); // main plan (c_seat_plan=0)
ok(is_array($seats), 'seats is array');
ok(count($seats) > 0, 'has occupied seats');
ok(count($seats) <= 320, 'no more seats than attended students');
$s = $seats[0];
foreach (array('section','group','row','col','label','chowky','chair','student_id','name','acco','age','old_student','short_courses','long_courses','backrest') as $k) {
  ok(array_key_exists($k, $s), "seat has field: $k");
}
ok(in_array($s['section'], array('RIGHT','LEFT')), 'section is RIGHT/LEFT');
ok(is_int($s['row']) && is_int($s['col']), 'row/col are ints');
ok(is_bool($s['backrest']) && is_bool($s['chowky']), 'flags are bool');
// every seat maps to a real student in the students set
$students = dcv_course_students(9900002);
$orphans = 0;
foreach ($seats as $x) { if (!isset($students[(string) $x['student_id']])) { $orphans++; } }
ok($orphans === 0, 'every seat maps to a student record');

echo empty($GLOBALS['fail']) ? "ALL PASS\n" : "FAILURES\n";
