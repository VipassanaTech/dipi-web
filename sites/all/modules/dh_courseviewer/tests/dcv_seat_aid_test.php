<?php
// sites/all/modules/dh_courseviewer/tests/dcv_seat_aid_test.php
//
// Dipi can mark a student in the main (or group) grid as sitting on a chowky or a chair at
// their own seat (dh_applicant_attended.aa_seat_aid = 'CW' / 'CH'). The bundle carries it as
// seat_aid on the seat and in the student's seating block — an extra field only, so older
// apps (which ignore unknown fields) are unaffected; the chowky/chair grid flags don't change.
// Runs against THIS worktree's code; the marks are set inside a rolled-back transaction on the
// synthetic fixture course 9900002.
if (PHP_SAPI !== 'cli') { exit; }
define('DRUPAL_ROOT', realpath(__DIR__ . '/../../../../..')); chdir(DRUPAL_ROOT);
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
function ok($c, $m) { echo ($c ? "PASS" : "FAIL") . " - $m\n"; if (!$c) { $GLOBALS['fail'] = 1; } }

ok(function_exists('dcv_seat_aid'), 'dcv_seat_aid defined');
if (function_exists('dcv_seat_aid')) {
  ok(dcv_seat_aid('CW') === 'CW' && dcv_seat_aid('CH') === 'CH', 'CW / CH kept');
  ok(dcv_seat_aid(' cw') === 'CW', 'trimmed, upper-cased');
  ok(dcv_seat_aid('') === '' && dcv_seat_aid(NULL) === '' && dcv_seat_aid('XX') === '', 'anything else -> ""');
}

const COURSE = 9900002;
$ids = db_query("select a_id from dh_applicant join dh_applicant_attended on aa_applicant = a_id
  where a_course = :c and a_attended = 1 and a_type = 'Student' and aa_chowky = 0 and aa_chair = 0
  order by a_id limit 3", array(':c' => COURSE))->fetchCol();
ok(count($ids) === 3, 'fixture: three main-grid students');
list($cw, $ch, $bad) = array_map('intval', $ids);

$txn = db_transaction();
try {
  db_update('dh_applicant_attended')->fields(array('aa_seat_aid' => 'CW'))->condition('aa_applicant', $cw)->execute();
  db_update('dh_applicant_attended')->fields(array('aa_seat_aid' => 'CH'))->condition('aa_applicant', $ch)->execute();
  db_update('dh_applicant_attended')->fields(array('aa_seat_aid' => 'XX'))->condition('aa_applicant', $bad)->execute();

  $b = dcv_build_bundle(COURSE);
  $seat = array();
  foreach ($b['seats'] as $s) { $seat[$s['student_id']] = $s; }
  ok(array_key_exists('seat_aid', $seat[$cw]), 'every seat carries seat_aid');
  ok($seat[$cw]['seat_aid'] === 'CW' && $seat[$ch]['seat_aid'] === 'CH', 'CW / CH exported on the seat');
  ok($seat[$bad]['seat_aid'] === '', 'unknown value exported as ""');
  ok(!$seat[$cw]['chowky'] && !$seat[$cw]['chair'] && !$seat[$ch]['chowky'] && !$seat[$ch]['chair'],
    'chowky / chair grid flags unchanged (the student stays in the main grid for old apps too)');
  $others = array_filter($b['seats'], function ($s) use ($cw, $ch) { return !in_array($s['student_id'], array($cw, $ch), true); });
  ok(count(array_filter($others, function ($s) { return $s['seat_aid'] !== ''; })) === 0, 'unmarked seats: seat_aid ""');
  ok($b['students'][(string) $cw]['seating']['seat_aid'] === 'CW', 'student seating block carries CW');
  ok($b['students'][(string) $ch]['seating']['seat_aid'] === 'CH', 'student seating block carries CH');
  ok($b['schema_version'] === 2, 'schema_version unchanged (additive field)');
}
finally {
  $txn->rollback();
}
ok(db_query("select count(*) from dh_applicant_attended where aa_applicant in (:ids) and aa_seat_aid <> ''",
  array(':ids' => $ids))->fetchField() == 0, 'rolled back: fixture marks cleared');

echo empty($GLOBALS['fail']) ? "ALL PASS\n" : "FAILURES\n";
