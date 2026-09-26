<?php
// sites/all/modules/dh_courseviewer/tests/dcv_teacher_access_test.php
//
// Teachers (ATs) mapped to a course may use the app for it, as in Dipi's AT portal:
// only Confirmed Conducting/Assisting mappings count (trainees / cancelled don't).
// Runs against THIS worktree's code (DRUPAL_ROOT from the file's location). Every
// row it creates (teacher, mappings, centre link) is inside a transaction that is
// rolled back at the end, so the local DB is left untouched. Uses the synthetic
// fixture course 9900002 (centre 5, 2026-09-30 → 10-03) with an explicit "today".
define('DRUPAL_ROOT', realpath(__DIR__ . '/../../../../..')); chdir(DRUPAL_ROOT);
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
require_once DRUPAL_ROOT . '/sites/all/modules/dh_courseviewer/inc/dcv-api.inc';
function ok($c, $m) { echo ($c ? "PASS" : "FAIL") . " - $m\n"; if (!$c) { $GLOBALS['fail'] = 1; } }

foreach (array('dcv_api_access', 'dcv_teacher_id', 'dcv_scope_centres', 'dcv_teacher_mapped') as $f) {
  ok(function_exists($f), "$f defined");
}
if (!empty($GLOBALS['fail'])) { echo "FAILURES\n"; exit(1); }

const COURSE = 9900002;         // centre 5
const DAY = '2026-10-01';       // a day the fixture course is running
$other = (int) db_query("select c_id from dh_course where c_center<>5 and c_deleted=0 and c_cancelled=0
  and c_start<=:d and c_end>=:d limit 1", array(':d' => DAY))->fetchField();
$at_rid = (int) db_query("select rid from {role} where name='AT Portal'")->fetchField();
$ca_rid = (int) db_query("select rid from {role} where name='Centre Admin'")->fetchField();
ok($other > 0 && $at_rid > 0 && $ca_rid > 0, 'fixtures: another running course, AT Portal + Centre Admin roles');

function ids($list) { return array_map(function ($c) { return $c['id']; }, $list); }
function as_user($uid, $name, $rids) {
  global $user;
  $roles = array(DRUPAL_AUTHENTICATED_RID => 'authenticated user');
  foreach ($rids as $r) { $roles[$r] = "r$r"; }
  $user = (object) array('uid' => $uid, 'name' => $name, 'roles' => $roles);
  drupal_static_reset('user_access');
}
function map_teacher($tid, $course, $type, $status) {
  return db_insert('dh_course_teacher')->fields(array(
    'ct_course' => $course, 'ct_teacher' => $tid, 'ct_type' => $type, 'ct_status' => $status,
    'ct_group' => 0, 'ct_created_by' => 1, 'ct_updated' => date('Y-m-d H:i:s'), 'ct_updated_by' => 1,
  ))->execute();
}

$txn = db_transaction();
try {
  $tid = (int) db_insert('dh_teacher')->fields(array(
    't_code' => 'ZZDCV1', 't_gender' => 'M', 't_f_name' => 'Test', 't_status' => 'Active',
    't_created_by' => 1, 't_updated_by' => 1,
  ))->execute();
  $uid = 999990;          // a fake account id; nothing is written to {users}
  $name = 'ZZDCV1.M';     // Dipi's AT username = t_code.t_gender

  // --- Which mappings count --------------------------------------------------
  ok(!in_array(COURSE, ids(dcv_build_course_list(array(), $tid, DAY))), 'no mapping → course not listed');
  $ct = map_teacher($tid, COURSE, 'Training', 'Confirmed');
  ok(!in_array(COURSE, ids(dcv_build_course_list(array(), $tid, DAY))), 'trainee mapping → not listed');
  ok(!dcv_teacher_mapped($tid, COURSE), 'trainee mapping → not mapped');
  db_update('dh_course_teacher')->fields(array('ct_type' => 'Assisting', 'ct_status' => 'Cancelled'))->condition('ct_id', $ct)->execute();
  ok(!in_array(COURSE, ids(dcv_build_course_list(array(), $tid, DAY))), 'cancelled mapping → not listed');
  ok(!dcv_teacher_mapped($tid, COURSE), 'cancelled mapping → not mapped');
  db_update('dh_course_teacher')->fields(array('ct_status' => 'Received'))->condition('ct_id', $ct)->execute();
  ok(!dcv_teacher_mapped($tid, COURSE), 'unconfirmed (Received) mapping → not mapped');
  db_update('dh_course_teacher')->fields(array('ct_status' => 'Confirmed'))->condition('ct_id', $ct)->execute();
  ok(in_array(COURSE, ids(dcv_build_course_list(array(), $tid, DAY))), 'Confirmed Assisting → listed');
  ok(dcv_teacher_mapped($tid, COURSE), 'Confirmed Assisting → mapped');
  db_update('dh_course_teacher')->fields(array('ct_type' => 'Conducting'))->condition('ct_id', $ct)->execute();
  ok(dcv_teacher_mapped($tid, COURSE), 'Confirmed Conducting → mapped');
  ok(!dcv_teacher_mapped($tid, $other), 'another course → not mapped');

  // --- Running courses only, same as centre users: Day 0 (c_start) to the last day
  ok(in_array(COURSE, ids(dcv_build_course_list(array(), $tid, '2026-09-30'))), 'Day 0 (start date) → listed');
  ok(in_array(COURSE, ids(dcv_build_course_list(array(), $tid, '2026-10-03'))), 'last day → listed');
  ok(!in_array(COURSE, ids(dcv_build_course_list(array(), $tid, '2026-09-29'))), 'day before Day 0 → not listed');
  ok(!in_array(COURSE, ids(dcv_build_course_list(array(), $tid, '2026-10-04'))), 'day after the last day → not listed');

  // --- Lists: teacher only, centre only, both (no duplicates) ----------------
  $t_only = dcv_build_course_list(array(), $tid, DAY);
  ok(ids($t_only) === array(COURSE), 'teacher-only list = just their course');
  ok(array_keys($t_only[0]) === array('id', 'name', 'centre', 'start_date', 'end_date'), 'field whitelist unchanged');
  ok(dcv_build_course_list(array(), FALSE, DAY) === array(), 'no centres + no teacher → empty');
  $c_only = ids(dcv_build_course_list(array(5), FALSE, DAY));
  ok(in_array(COURSE, $c_only), 'centre user (regression): centre 5 lists the course');
  $both = ids(dcv_build_course_list(array(5), $tid, DAY));
  ok(count(array_keys($both, COURSE)) === 1, 'dual role: course listed once');
  ok(count($both) === count($c_only), 'dual role: nothing extra when mapped course is in their centre');
  map_teacher($tid, $other, 'Assisting', 'Confirmed');
  $both = ids(dcv_build_course_list(array(5), $tid, DAY));
  ok(in_array($other, $both) && in_array(COURSE, $both), 'dual role: centre courses + mapped course at another centre');

  // --- Who the API lets in, and to what --------------------------------------
  db_insert('dh_user_center')->fields(array('uc_user' => $uid, 'uc_center' => 5, 'uc_deleted' => 0,
    'uc_created_by' => 1, 'uc_updated' => date('Y-m-d H:i:s'), 'uc_updated_by' => 1))->execute();

  as_user($uid, $name, array());   // logged in, no Dipi role
  ok(!dcv_api_access(), 'no role → API refused');
  ok(dcv_teacher_id() === FALSE, 'no AT permission → not treated as a teacher');
  ok(!dcv_user_can_access_course(COURSE), 'no role → course refused');

  as_user($uid, $name, array($at_rid));   // AT only (also has a centre link, which must not count)
  ok(dcv_api_access(), 'AT → API allowed');
  ok(dcv_teacher_id() === $tid, 'AT → resolved to their teacher id');
  ok(dcv_scope_centres() === array(), 'AT without "access zero day" → no centre scope');
  ok(dcv_user_can_access_course(COURSE), 'AT → mapped course allowed');
  ok(dcv_user_can_access_course($other), 'AT → second mapped course allowed');
  db_delete('dh_course_teacher')->condition('ct_teacher', $tid)->condition('ct_course', $other)->execute();
  ok(!dcv_user_can_access_course($other), 'AT → unmapped course refused');

  as_user($uid, 'nobody.M', array($at_rid));   // AT role but no matching active teacher
  ok(dcv_teacher_id() === FALSE && !dcv_user_can_access_course(COURSE), 'AT role, no teacher record → refused');

  as_user($uid, $name, array($ca_rid));   // centre user (regression)
  ok(dcv_api_access(), 'centre user → API allowed');
  ok(dcv_scope_centres() === array(5), 'centre user → their centre');
  ok(dcv_teacher_id() === FALSE, 'centre user without AT permission → not a teacher');
  ok(dcv_user_can_access_course(COURSE), 'centre user → course in their centre allowed');
  ok(!dcv_user_can_access_course($other), 'centre user → other centre refused');

  as_user($uid, $name, array($ca_rid, $at_rid));   // both roles
  ok(dcv_user_can_access_course(COURSE) && dcv_teacher_id() === $tid, 'dual role → centre + teacher both apply');
}
finally {
  $txn->rollback();
}
ok((int) db_query("select count(*) from dh_teacher where t_code='ZZDCV1'")->fetchField() === 0, 'rolled back: no test teacher left');

echo empty($GLOBALS['fail']) ? "ALL PASS\n" : "FAILURES\n";
