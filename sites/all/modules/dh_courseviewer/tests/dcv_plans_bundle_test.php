<?php
// sites/all/modules/dh_courseviewer/tests/dcv_plans_bundle_test.php
//
// The bundle carries only the chosen plans; without ?plans= it is the whole course.
// Runs against THIS worktree's code. The group-wise case changes the synthetic fixture
// course 9900002 inside a transaction that is rolled back at the end.
if (PHP_SAPI !== 'cli') { exit; }
define('DRUPAL_ROOT', realpath(__DIR__ . '/../../../../..')); chdir(DRUPAL_ROOT);
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
foreach (array('dcv-api.inc', 'dcv-bundle.inc', 'dcv-plans.inc') as $f) {
  require_once DRUPAL_ROOT . '/sites/all/modules/dh_courseviewer/inc/' . $f;
}
function ok($c, $m) { echo ($c ? "PASS" : "FAIL") . " - $m\n"; if (!$c) { $GLOBALS['fail'] = 1; } }
const COURSE = 9900002;

// The fixture leaves a_conf_no blank for all 320 students, so the "no Female data
// leaks" check below (which greps for a real Female conf_no) would have nothing to
// find. Seed one Female student's conf_no with a sentinel value, inside a
// transaction rolled back at the very end (same pattern as the group-wise section
// further down), so the check exercises real data instead of trivially passing.
// Wrapped in try/finally: D7's DatabaseTransaction::__destruct() COMMITS a
// transaction that was never explicitly rolled back, so a thrown/fatal error
// partway through must not skip the rollback (the seed is shared-DB state).
$conf_txn = db_transaction();
try {
  $seed_id = db_query("select a_id from dh_applicant
      where a_course = :c and a_attended = 1 and a_type = 'Student' and (a_gender is null or a_gender <> 'M')
      limit 1", array(':c' => COURSE))->fetchField();
  db_update('dh_applicant')->fields(array('a_conf_no' => 'DCVTEST01'))->condition('a_id', $seed_id)->execute();

  $full = dcv_build_bundle(COURSE);
  ok(count($full['students']) === 320, 'no plans: the whole course (320 students)');
  ok(count($full['seats']) === count(dcv_course_seats(COURSE)), 'no plans: every seat');

  $male = dcv_build_bundle(COURSE, array('RIGHT'));
  $maleIds = array(); $femaleConf = NULL;
  foreach ($full['students'] as $id => $st) {
    if ($st['identity']['gender'] === 'M') { $maleIds[] = (string) $id; }
    elseif ($femaleConf === NULL && $st['identity']['conf_no'] !== '') { $femaleConf = $st['identity']['conf_no']; }
  }
  $gotIds = array_map('strval', array_keys($male['students'])); sort($gotIds); sort($maleIds);
  ok($gotIds === $maleIds, 'RIGHT: exactly the Male students');
  ok(count(array_filter($male['seats'], function ($s) { return $s['section'] !== 'RIGHT'; })) === 0, 'RIGHT: only Male seats');
  ok(count($male['seats']) === count(array_filter($full['seats'], function ($s) { return $s['section'] === 'RIGHT'; })), 'RIGHT: every Male seat');
  ok(array_map(function ($s) { return $s['key']; }, $male['sections']) === array('RIGHT'), 'RIGHT: only the Male section');
  ok($femaleConf !== NULL && strpos(json_encode($male), json_encode($femaleConf)) === FALSE, 'RIGHT: no Female student anywhere in the JSON');
  ok($male['course'] === $full['course'] && $male['expires_after'] === $full['expires_after'] && $male['schema_version'] === 2, 'header, expiry, schema unchanged');

  $both = dcv_build_bundle(COURSE, array('RIGHT', 'LEFT'));
  ok(count($both['students']) === 320 && count($both['seats']) === count($full['seats']) && count($both['sections']) === count($full['sections']), 'both plans = the whole course');

  ok(dcv_requested_plans(COURSE, NULL) === NULL, 'request without plans → whole course');
  ok(dcv_requested_plans(COURSE, 'RIGHT') === array('RIGHT'), 'request RIGHT');
  ok(dcv_requested_plans(COURSE, 'RIGHT-1') === FALSE, 'a group id on a main-plan course is refused');
  ok(dcv_requested_plans(COURSE, 'x') === FALSE && dcv_requested_plans(COURSE, '') === FALSE, 'malformed / empty refused');

  $txn = db_transaction();
  try {
    db_update('dh_course')->fields(array('c_seat_plan' => 1))->condition('c_id', COURSE)->execute();
    foreach (db_query("select a_id, a_gender from dh_applicant where a_course = :c and a_attended = 1 and a_type = 'Student'",
        array(':c' => COURSE)) as $r) {
      $grp = ($r->a_gender === 'M' && $r->a_id % 2 === 0) ? 1 : 2;
      db_update('dh_applicant_attended')->fields(array('aa_group' => $grp))->condition('aa_applicant', $r->a_id)->execute();
    }
    $want = array();
    foreach (db_query("select a_id, a_gender, coalesce(aa_group, 0) as g from dh_applicant left join dh_applicant_attended on a_id = aa_applicant
        where a_course = :c and a_attended = 1 and a_type = 'Student'", array(':c' => COURSE)) as $r) {
      if (dcv_plan_id($r->a_gender, $r->g, 'group') === 'RIGHT-1') { $want[] = (string) $r->a_id; }
    }
    $g = dcv_build_bundle(COURSE, array('RIGHT-1'));
    $got = array_map('strval', array_keys($g['students'])); sort($got); sort($want);
    ok(count($want) > 0 && $got === $want, 'group-wise RIGHT-1: exactly its students');
    ok(count(array_filter($g['seats'], function ($s) { return !($s['section'] === 'RIGHT' && $s['group'] === 1); })) === 0, 'group-wise RIGHT-1: only its seats');
    ok(count($g['seats']) === count($want), 'group-wise RIGHT-1: every one of its seats');
    ok(array_map(function ($s) { return $s['key']; }, $g['sections']) === array('RIGHT'), 'group-wise: only the Male section');
    ok(dcv_requested_plans(COURSE, 'RIGHT') === FALSE && dcv_requested_plans(COURSE, 'RIGHT-1,LEFT-2') === array('RIGHT-1', 'LEFT-2'), 'a group-wise course takes group ids only');
  }
  finally {
    $txn->rollback();
  }
  ok(dh_course_plan(COURSE, 'seat') === 'main', 'fixture restored');
}
finally {
  $conf_txn->rollback();
}

echo empty($GLOBALS['fail']) ? "ALL PASS\n" : "FAILURES\n";
exit(empty($GLOBALS['fail']) ? 0 : 1);
