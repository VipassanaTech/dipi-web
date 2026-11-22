<?php
// sites/all/modules/dh_courseviewer/tests/dcv_combined_seating_test.php
//
// Two courses sharing one hall (dh_course.c_combined_seat_course). Dipi's seating/ and
// group-seating/ pages draw the opened course AND the course its own link names (one-way), so
// the bundle does the same: that course's seated students, students and servers come too, with
// the same filters (attended, the side limit, the chosen plans), each marked with its course_id,
// and the header names the partner (combined_with). A partner at another centre is never used.
// Runs against THIS worktree's code. The synthetic fixture course 9900002 (centre 5) is split
// into a pair inside a transaction that is rolled back at the end.
if (PHP_SAPI !== 'cli') { exit; }
define('DRUPAL_ROOT', realpath(__DIR__ . '/../../../../..')); chdir(DRUPAL_ROOT);
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
foreach (array('dcv-api.inc', 'dcv-bundle.inc', 'dcv-plans.inc') as $f) {
  require_once DRUPAL_ROOT . '/sites/all/modules/dh_courseviewer/inc/' . $f;
}
function ok($c, $m) { echo ($c ? "PASS" : "FAIL") . " - $m\n"; if (!$c) { $GLOBALS['fail'] = 1; } }
function seat_ids($b) { $o = array_map(function ($s) { return $s['student_id']; }, $b['seats']); sort($o); return $o; }
function keys_of($m) { $o = array_map('intval', array_keys((array) $m)); sort($o); return $o; }
function totals($plans) { $t = array(0, 0); foreach ($plans as $p) { $t[0] += $p['students']; $t[1] += $p['servers']; } return $t; }
const COURSE = 9900002;   // centre 5
const PARTNER = 9900902;  // made here, rolled back

ok(!db_query("select 1 from dh_course where c_id = :c", array(':c' => PARTNER))->fetchField(), 'fixture: the partner id is free');
ok((int) db_query("select c_combined_seat_course from dh_course where c_id = :c", array(':c' => COURSE))->fetchField() === 0, 'fixture: the course has no partner yet');
ok(dcv_seat_courses(COURSE) === array(COURSE), 'no link: the plan is the course alone');

// D7's DatabaseTransaction::__destruct() COMMITS a transaction that was never rolled back,
// so the rollback sits in finally.
$txn = db_transaction();
try {
  // Two students become servers (one will move to the partner), so each course has one.
  $pick = function ($course, $gender, $n, $desc = FALSE) {
    $o = $desc ? 'desc' : 'asc';
    return array_map('intval', db_query("select a_id from dh_applicant where a_course = :c and a_attended = 1
      and a_type = 'Student' and a_gender = :g order by a_id $o limit $n", array(':c' => $course, ':g' => $gender))->fetchCol());
  };
  $srvA = $pick(COURSE, 'F', 1, TRUE); $srvB = $pick(COURSE, 'M', 1, TRUE);
  db_update('dh_applicant')->fields(array('a_type' => 'Sevak'))->condition('a_id', array_merge($srvA, $srvB), 'IN')->execute();

  $before = dcv_build_bundle(COURSE);
  $beforePlans = dcv_course_plans(COURSE);
  $beforeF = dcv_course_plans(COURSE, array('F'));

  // Split: the partner course (same centre) takes 4 Male + 6 Female seated students and 1 server.
  $row = db_query("select * from dh_course where c_id = :c", array(':c' => COURSE))->fetchAssoc();
  $row['c_id'] = PARTNER; $row['c_name'] = 'TEST COURSE combined partner'; $row['c_combined_seat_course'] = 0;
  db_insert('dh_course')->fields($row)->execute();
  // Dipi's dh_course trigger rebuilds c_name from centre, type and dates, so read what it stored.
  $pname = (string) db_query("select c_name from dh_course where c_id = :c", array(':c' => PARTNER))->fetchField();
  $movedM = $pick(COURSE, 'M', 4); $movedF = $pick(COURSE, 'F', 6);
  $moved = array_merge($movedM, $movedF);
  $movedSorted = $moved; sort($movedSorted);
  $stay = $pick(COURSE, 'M', 1, TRUE)[0];   // a seated student who stays in the course
  db_update('dh_applicant')->fields(array('a_course' => PARTNER))->condition('a_id', array_merge($moved, $srvB), 'IN')->execute();
  $link = function ($from, $to) {
    db_update('dh_course')->fields(array('c_combined_seat_course' => $to))->condition('c_id', $from)->execute();
  };

  // --- not linked: each course alone
  $alone = dcv_build_bundle(COURSE);
  ok(count($alone['seats']) === count($before['seats']) - 10, 'not linked: the moved students are gone from the course');
  ok($alone['course']['combined_with'] === NULL, 'not linked: combined_with is null');

  // --- linked one way, course → partner (as Dipi draws course's plan)
  $link(COURSE, PARTNER);
  ok(dcv_seat_courses(COURSE) === array(COURSE, PARTNER), 'linked: the plan is the course and its partner');
  $a = dcv_build_bundle(COURSE);
  ok(seat_ids($a) === seat_ids($before), 'linked: every seat in the hall is back');
  ok(keys_of($a['students']) === keys_of($before['students']), 'linked: every student is back');
  $srvIds = keys_of($a['servers']);
  ok($srvIds === keys_of(array_flip(array_merge($srvA, $srvB))), "linked: both courses' servers");
  $byCourse = array();
  foreach ($a['seats'] as $s) { $byCourse[$s['student_id']] = $s['course_id']; }
  $okMarks = TRUE;
  foreach ($byCourse as $id => $c) { if ($c !== (in_array($id, $moved, TRUE) ? PARTNER : COURSE)) { $okMarks = FALSE; } }
  ok($okMarks, 'each seat says its own course');
  ok($a['students'][(string) $moved[0]]['course_id'] === PARTNER && $a['students'][(string) $moved[0]]['course']['course_name'] === $pname,
    "a partner student: course_id and its own course's name");
  ok($a['students'][(string) $stay]['course_id'] === COURSE && $byCourse[$stay] === COURSE, "a course student: the course's id");
  ok($a['servers'][(string) $srvB[0]]['course_id'] === PARTNER && $a['servers'][(string) $srvA[0]]['course_id'] === COURSE, 'servers say their course');
  ok($pname !== '' && $a['course']['combined_with'] === array('id' => PARTNER, 'name' => $pname), 'header: combined_with names the partner');
  ok($a['course']['id'] === COURSE && $a['schema_version'] === 2, 'header: still the opened course; schema 2');
  ok(totals(dcv_course_plans(COURSE)) === totals($beforePlans), 'plan counts: both courses (students and servers)');

  // the same filters apply to the partner's people
  $m = dcv_build_bundle(COURSE, NULL, array('M'));
  $genders = array();
  foreach ($m['students'] as $st) { $genders[$st['identity']['gender']] = 1; }
  ok(array_keys($genders) === array('M') && count(array_intersect(seat_ids($m), $movedF)) === 0
    && count(array_intersect(seat_ids($m), $movedM)) === 4, 'side M: Male only, from both courses');
  $f = dcv_build_bundle(COURSE, array('LEFT'));
  ok(count(array_intersect(seat_ids($f), $movedF)) === 6 && count(array_intersect(seat_ids($f), $movedM)) === 0, 'plan LEFT: Female only, from both courses');
  ok(dcv_course_plans(COURSE, array('F')) === $beforeF, 'side F: the same Female plan and counts as the whole course had');
  db_update('dh_applicant')->fields(array('a_attended' => 0))->condition('a_id', $movedF[0])->execute();
  ok(!in_array($movedF[0], seat_ids(dcv_build_bundle(COURSE)), TRUE), 'a partner student not attended is left out');
  db_update('dh_applicant')->fields(array('a_attended' => 1))->condition('a_id', $movedF[0])->execute();

  // --- one-way stays one-way: the partner's own plan is the partner alone
  $b = dcv_build_bundle(PARTNER);
  ok(seat_ids($b) === $movedSorted, "partner (no link of its own): only its own seats");
  ok($b['course']['combined_with'] === NULL && keys_of($b['servers']) === array($srvB[0]), 'partner: no combined_with, its own server');
  $link(PARTNER, COURSE);
  ok(seat_ids(dcv_build_bundle(PARTNER)) === seat_ids($before), 'partner linked back: the whole hall');

  // --- a bad link is never used
  db_update('dh_course')->fields(array('c_center' => 6))->condition('c_id', PARTNER)->execute();
  ok(dcv_seat_courses(COURSE) === array(COURSE), 'partner at another centre: not used');
  $x = dcv_build_bundle(COURSE);
  ok(count(array_intersect(seat_ids($x), $moved)) === 0 && $x['course']['combined_with'] === NULL, "another centre's students never come");
  db_update('dh_course')->fields(array('c_center' => 5))->condition('c_id', PARTNER)->execute();
  $link(COURSE, COURSE);
  ok(dcv_seat_courses(COURSE) === array(COURSE), 'a link to itself: the course alone');
  $link(COURSE, 98765432);
  ok(dcv_seat_courses(COURSE) === array(COURSE), 'a link to a missing course: the course alone');
}
finally {
  $txn->rollback();
}
ok(!db_query("select 1 from dh_course where c_id = :c", array(':c' => PARTNER))->fetchField(), 'rolled back: no partner course');
ok((int) db_query("select count(*) from dh_applicant where a_course = :c", array(':c' => PARTNER))->fetchField() === 0, 'rolled back: no one moved');

echo empty($GLOBALS['fail']) ? "ALL PASS\n" : "FAILURES\n";
exit(empty($GLOBALS['fail']) ? 0 : 1);
