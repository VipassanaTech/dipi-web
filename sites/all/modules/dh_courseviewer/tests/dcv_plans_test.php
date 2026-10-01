<?php
// sites/all/modules/dh_courseviewer/tests/dcv_plans_test.php
//
// Choosing plans when loading a course: the plan helpers and the course list's plans.
// Runs against THIS worktree's code (DRUPAL_ROOT from the file's location). The
// group-wise case changes the synthetic fixture course 9900002 (centre 5, 2026-09-30 →
// 10-03, main plan, 320 students) inside a transaction that is rolled back at the end.
if (PHP_SAPI !== 'cli') { exit; }
define('DRUPAL_ROOT', realpath(__DIR__ . '/../../../../..')); chdir(DRUPAL_ROOT);
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
foreach (array('dcv-api.inc', 'dcv-bundle.inc', 'dcv-plans.inc') as $f) {
  require_once DRUPAL_ROOT . '/sites/all/modules/dh_courseviewer/inc/' . $f;
}
function ok($c, $m) { echo ($c ? "PASS" : "FAIL") . " - $m\n"; if (!$c) { $GLOBALS['fail'] = 1; } }
function ids($plans) { return array_map(function ($p) { return $p['id']; }, $plans); }
const COURSE = 9900002;

// --- pure helpers
ok(dcv_parse_plans(NULL) === NULL, 'no ?plans= → the whole course');
ok(dcv_parse_plans('') === FALSE && dcv_parse_plans(' , ') === FALSE, 'empty → refused');
ok(dcv_parse_plans('RIGHT') === array('RIGHT'), 'one plan');
ok(dcv_parse_plans('RIGHT, LEFT,RIGHT') === array('RIGHT', 'LEFT'), 'trimmed and de-duplicated');
ok(dcv_parse_plans('LEFT-2,RIGHT-10') === array('LEFT-2', 'RIGHT-10'), 'group ids');
foreach (array('MALE', 'RIGHT-', 'RIGHT-1234', 'LEFT;drop', "RIGHT'", 'right') as $bad) {
  ok(dcv_parse_plans($bad) === FALSE, "malformed '$bad' refused");
}
ok(dcv_plans_fit_kind(array('RIGHT', 'LEFT'), 'main') && !dcv_plans_fit_kind(array('RIGHT-1'), 'main'), 'a main plan takes RIGHT/LEFT only');
ok(dcv_plans_fit_kind(array('RIGHT-1', 'LEFT-2'), 'group') && !dcv_plans_fit_kind(array('LEFT'), 'group'), 'a group-wise plan takes RIGHT-n/LEFT-n only');
ok(dcv_plan_id('M', 3, 'main') === 'RIGHT' && dcv_plan_id('F', 3, 'group') === 'LEFT-3' && dcv_plan_id('', 0, 'main') === 'LEFT', "a student's plan id");
list($sql, $args) = dcv_plans_sql(array('RIGHT'), 'main');
ok($sql === "((a_gender = 'M'))" && $args === array(), 'SQL for RIGHT');
list($sql, $args) = dcv_plans_sql(array('LEFT-2', 'RIGHT-1'), 'group');
ok($sql === "(((a_gender is null or a_gender <> 'M') and coalesce(aa_group, 0) = :pg0) or ((a_gender = 'M') and coalesce(aa_group, 0) = :pg1))"
  && $args === array(':pg0' => 2, ':pg1' => 1), 'SQL for group ids binds the group numbers');
list($sql, $args) = dcv_plans_sql(array(), 'main');
ok($sql === '(1=0)' && $args === array(), 'SQL for an empty plan list matches nothing (guard against invalid "()")');
$secs = array(
  array('key' => 'RIGHT', 'label' => 'Male', 'groups' => array(array('n' => 1), array('n' => 2))),
  array('key' => 'LEFT', 'label' => 'Female', 'groups' => array(array('n' => 1), array('n' => 2))),
);
$f = dcv_filter_sections($secs, array('RIGHT'), 'main');
ok(count($f) === 1 && $f[0]['key'] === 'RIGHT' && count($f[0]['groups']) === 2, 'main: sections cut to the chosen gender');
$f = dcv_filter_sections($secs, array('LEFT-2'), 'group');
ok(count($f) === 1 && $f[0]['key'] === 'LEFT' && $f[0]['groups'] === array(array('n' => 2)), 'group-wise: the section and its chosen groups only');

// --- the fixture course (main plan): counts straight from the DB
$count = array('RIGHT' => 0, 'LEFT' => 0);
foreach (db_query("select a_gender from dh_applicant where a_course = :c and a_attended = 1 and a_type = 'Student'",
    array(':c' => COURSE)) as $r) {
  $count[$r->a_gender === 'M' ? 'RIGHT' : 'LEFT']++;
}
ok($count['RIGHT'] > 0 && $count['LEFT'] > 0 && $count['RIGHT'] + $count['LEFT'] === 320, 'fixture has both genders, 320 students');
$plans = dcv_course_plans(COURSE);
ok(ids($plans) === array('RIGHT', 'LEFT'), 'main: RIGHT then LEFT');
ok($plans[0]['label'] === 'Male' && $plans[1]['label'] === 'Female', 'main labels');
ok($plans[0]['students'] === $count['RIGHT'] && $plans[1]['students'] === $count['LEFT'], 'main counts');
$list = dcv_build_course_list(array(5), FALSE, '2026-10-01');
$mine = array_values(array_filter($list, function ($c) { return $c['id'] === COURSE; }));
ok(count($mine) === 1 && $mine[0]['plans'] === $plans, 'the course list carries the plans');
ok(json_encode($mine[0]['plans']) !== FALSE, 'plans JSON-encode');

// --- group-wise, inside a rolled-back transaction
$txn = db_transaction();
try {
  db_update('dh_course')->fields(array('c_seat_plan' => 1))->condition('c_id', COURSE)->execute();
  // Male students with an even a_id → group 1, everyone else → group 2.
  foreach (db_query("select a_id, a_gender from dh_applicant where a_course = :c and a_attended = 1 and a_type = 'Student'",
      array(':c' => COURSE)) as $r) {
    $grp = ($r->a_gender === 'M' && $r->a_id % 2 === 0) ? 1 : 2;
    db_update('dh_applicant_attended')->fields(array('aa_group' => $grp))->condition('aa_applicant', $r->a_id)->execute();
  }
  $expect = array();
  foreach (db_query("select a_gender, coalesce(aa_group, 0) as g from dh_applicant left join dh_applicant_attended on a_id = aa_applicant
      where a_course = :c and a_attended = 1 and a_type = 'Student'", array(':c' => COURSE)) as $r) {
    $id = dcv_plan_id($r->a_gender, $r->g, 'group');
    $expect[$id] = (isset($expect[$id]) ? $expect[$id] : 0) + 1;
  }
  $plans = dcv_course_plans(COURSE);
  $got = array(); foreach ($plans as $p) { $got[$p['id']] = $p['students']; }
  $e = $expect; ksort($e); ksort($got);
  ok($got === $e, 'group-wise: one plan per group and gender, with counts');
  $order = array_keys($expect);
  usort($order, function ($a, $b) {
    list($ka, $ga) = explode('-', $a); list($kb, $gb) = explode('-', $b);
    return ((int) $ga - (int) $gb) ?: strcmp($ka, $kb);
  });
  ok(ids($plans) === $order, 'group-wise order: by group, LEFT before RIGHT');
  ok($plans[0]['id'] === 'RIGHT-1' && $plans[0]['label'] === 'Group 1 · Male', 'group-wise label');
}
finally {
  $txn->rollback();
}
ok(dh_course_plan(COURSE, 'seat') === 'main', 'fixture restored');

echo empty($GLOBALS['fail']) ? "ALL PASS\n" : "FAILURES\n";
exit(empty($GLOBALS['fail']) ? 0 : 1);
