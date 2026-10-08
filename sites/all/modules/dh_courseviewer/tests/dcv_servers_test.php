<?php
// sites/all/modules/dh_courseviewer/tests/dcv_servers_test.php
//
// Dhamma servers in the Dipi App: each plan carries its servers, and a side limit (a teacher's
// "Sees") keeps only that side's genders. Runs against THIS worktree's code. Turns four
// students of the synthetic fixture course 9900002 into servers (and one into an unknown
// gender) inside a transaction that is rolled back at the end.
if (PHP_SAPI !== 'cli') { exit; }
define('DRUPAL_ROOT', realpath(__DIR__ . '/../../../../..')); chdir(DRUPAL_ROOT);
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
foreach (array('dcv-api.inc', 'dcv-bundle.inc', 'dcv-plans.inc') as $f) {
  require_once DRUPAL_ROOT . '/sites/all/modules/dh_courseviewer/inc/' . $f;
}
function ok($c, $m) { echo ($c ? "PASS" : "FAIL") . " - $m\n"; if (!$c) { $GLOBALS['fail'] = 1; } }
function ids($plans) { return array_map(function ($p) { return $p['id']; }, $plans); }
function fullname($id) {
  return (string) db_query("select concat_ws(' ', nullif(trim(a_f_name),''), nullif(trim(a_m_name),''), nullif(trim(a_l_name),''))
    from dh_applicant where a_id = :i", array(':i' => $id))->fetchField();
}
const COURSE = 9900002;

// --- pure helpers
ok(dcv_sides_sql(NULL) === array('', array()), 'no side limit → no condition');
ok(dcv_sides_sql(array()) === array(' and (1=0)', array()), 'no sides → nothing matches');
ok(dcv_sides_sql(array('F')) === array(' and a_gender in (:dcv_sides)', array(':dcv_sides' => array('F'))), 'side limit → exact genders');
ok(dcv_plan_on_sides('RIGHT', array('M')) && dcv_plan_on_sides('LEFT-2', array('F')) && dcv_plan_on_sides('LEFT', array('M', 'F')), 'plans on the sides');
ok(!dcv_plan_on_sides('LEFT', array('M')) && !dcv_plan_on_sides('RIGHT-1', array('F')), 'plans off the sides');

// D7's DatabaseTransaction::__destruct() COMMITS a transaction that was never rolled back,
// so the rollback sits in finally.
$txn = db_transaction();
try {
  $pick = function ($gender, $n) {
    return array_map('intval', db_query("select a_id from dh_applicant where a_course = :c and a_attended = 1
      and a_type = 'Student' and a_gender = :g order by a_id limit $n", array(':c' => COURSE, ':g' => $gender))->fetchCol());
  };
  $srvM = $pick('M', 2); $srvF = $pick('F', 2);
  db_update('dh_applicant')->fields(array('a_type' => 'Sevak'))->condition('a_id', array_merge($srvM, $srvF), 'IN')->execute();
  // One Female student with no recorded gender: in the LEFT plan, but on no teacher's side.
  $unknown = (int) db_query("select a_id from dh_applicant where a_course = :c and a_attended = 1 and a_type = 'Student'
    and a_gender = 'F' order by a_id desc limit 1", array(':c' => COURSE))->fetchField();
  db_update('dh_applicant')->fields(array('a_gender' => ''))->condition('a_id', $unknown)->execute();
  $count = function ($gender) {
    return (int) db_query("select count(*) from dh_applicant where a_course = :c and a_attended = 1 and a_type = 'Student'
      and a_gender = :g", array(':c' => COURSE, ':g' => $gender))->fetchField();
  };
  $nM = $count('M'); $nF = $count('F');

  // --- course list plans: students and servers
  $plans = dcv_course_plans(COURSE);
  ok(ids($plans) === array('RIGHT', 'LEFT'), 'plans: RIGHT then LEFT');
  ok($plans[0]['students'] === $nM && $plans[0]['servers'] === 2, 'Male: its students and 2 servers');
  ok($plans[1]['students'] === $nF + 1 && $plans[1]['servers'] === 2, 'Female plan: its students (with the unknown gender) and 2 servers');
  $fOnly = dcv_course_plans(COURSE, array('F'));
  ok(ids($fOnly) === array('LEFT') && $fOnly[0]['students'] === $nF && $fOnly[0]['servers'] === 2, 'side F: LEFT only, exact Female counts');
  ok(dcv_course_plans(COURSE, array()) === array(), 'no sides: no plans');

  // --- bundle: servers per plan
  $full = dcv_build_bundle(COURSE);
  ok(count($full['servers']) === 4, 'whole course: 4 servers');
  $okPlan = TRUE;
  foreach ($full['servers'] as $id => $rec) {
    $want = in_array((int) $id, $srvM, TRUE) ? 'RIGHT' : 'LEFT';
    if ($rec['plan'] !== $want || $rec['identity']['type'] !== 'Sevak') { $okPlan = FALSE; }
  }
  ok($okPlan, 'each server: its plan by gender, type Sevak');
  ok(count(array_intersect(array_keys($full['servers']), array_keys($full['students']))) === 0, 'servers are not students');
  $seatIds = array_map(function ($s) { return $s['student_id']; }, $full['seats']);
  ok(count(array_intersect($seatIds, array_merge($srvM, $srvF))) === 0, 'servers have no seat');
  ok(isset($full['students'][(string) $unknown]), 'the whole course keeps the unknown-gender student');

  $male = dcv_build_bundle(COURSE, array('RIGHT'));
  $mk = array_map('intval', array_keys($male['servers'])); sort($mk);
  ok($mk === $srvM, 'RIGHT: only the Male servers');

  $sideF = dcv_build_bundle(COURSE, array('LEFT'), array('F'));
  $genders = array();
  foreach ($sideF['students'] as $st) { $genders[$st['identity']['gender']] = TRUE; }
  ok(array_keys($genders) === array('F') && count($sideF['students']) === $nF, 'side F: Female students only, the unknown gender left out');
  $fk = array_map('intval', array_keys($sideF['servers'])); sort($fk);
  ok($fk === $srvF, 'side F: only the Female servers');
  ok(count(array_filter($sideF['seats'], function ($s) { return $s['section'] !== 'LEFT'; })) === 0, 'side F: Female seats only');
  ok(!in_array($unknown, array_map(function ($s) { return $s['student_id']; }, $sideF['seats']), TRUE), 'side F: no seat of the unknown gender');
  $json = json_encode($sideF);
  $leak = FALSE;
  foreach (array_merge($srvM, array($unknown)) as $id) {
    $nm = fullname($id);
    if ($nm !== '' && strpos($json, json_encode($nm)) !== FALSE) { $leak = TRUE; }
  }
  ok(!$leak, 'side F: no Male server or unknown-gender name anywhere in the JSON');
  $none = dcv_build_bundle(COURSE, array('LEFT'), array());
  ok(count((array) $none['students']) === 0 && count((array) $none['servers']) === 0 && count($none['seats']) === 0, 'no sides: nobody');

  // --- group-wise: servers follow their group
  db_update('dh_course')->fields(array('c_seat_plan' => 1))->condition('c_id', COURSE)->execute();
  foreach (db_query("select a_id, a_gender from dh_applicant where a_course = :c and a_attended = 1", array(':c' => COURSE)) as $r) {
    $grp = ($r->a_gender === 'M' && $r->a_id % 2 === 0) ? 1 : 2;
    db_update('dh_applicant_attended')->fields(array('aa_group' => $grp))->condition('aa_applicant', $r->a_id)->execute();
  }
  $g = dcv_build_bundle(COURSE, array('RIGHT-1', 'RIGHT-2'));
  $okG = count($g['servers']) === 2;
  foreach ($g['servers'] as $id => $rec) {
    if ($rec['plan'] !== 'RIGHT-' . (((int) $id % 2 === 0) ? 1 : 2)) { $okG = FALSE; }
  }
  ok($okG, 'group-wise: each Male server in its group plan');
  $servers = 0;
  foreach (dcv_course_plans(COURSE) as $p) { $servers += $p['servers']; }
  ok($servers === 4, 'group-wise: the 4 servers counted across the group plans');
}
finally {
  $txn->rollback();
}
ok(dh_course_plan(COURSE, 'seat') === 'main'
  && (int) db_query("select count(*) from dh_applicant where a_course = :c and a_type = 'Sevak'", array(':c' => COURSE))->fetchField() === 0,
  'fixture restored');

echo empty($GLOBALS['fail']) ? "ALL PASS\n" : "FAILURES\n";
exit(empty($GLOBALS['fail']) ? 0 : 1);
