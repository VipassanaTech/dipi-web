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

// --- Phase 3A: enriched fields ---
$s0 = $seats[0];
foreach (array('cell','dining','lang_discourse') as $k) {
    ok(array_key_exists($k, $s0), "seat has field: $k");
    ok(is_string($s0[$k]), "seat.$k is string");
}
// cell is plan-effective: for the main-plan fixture it must equal aa_cell.
$plan_cell = function_exists('dh_course_plan') ? dh_course_plan(9900002, 'cell') : 'main';
ok($plan_cell === 'main', 'fixture cell plan is main');
$byId = array();
foreach ($seats as $s) { $byId[$s['student_id']] = $s; }
$row = db_query("select a_id, aa_cell, aa_dining from dh_applicant
                 left join dh_applicant_attended on a_id=aa_applicant
                 where a_course=9900002 and a_attended=1 and a_type='Student'
                   and aa_seat_row is not null and aa_seat_col is not null limit 1")->fetchObject();
ok(isset($byId[(int)$row->a_id]), 'sample student present in seats');
ok($byId[(int)$row->a_id]['cell'] === (string)(is_null($row->aa_cell)?'':$row->aa_cell), 'cell matches aa_cell (main plan)');
ok($byId[(int)$row->a_id]['dining'] === (string)(is_null($row->aa_dining)?'':$row->aa_dining), 'dining matches aa_dining (main plan)');
// lang resolution sanity (independent of fixture): 'TA' resolves to a dh_languages name.
$ta = db_query("select l_name from dh_languages where lower(l_code)='ta' limit 1")->fetchField();
ok($ta !== false && $ta !== null, 'dh_languages has a row for code ta (resolution target exists)');

echo empty($GLOBALS['fail']) ? "ALL PASS\n" : "FAILURES\n";
