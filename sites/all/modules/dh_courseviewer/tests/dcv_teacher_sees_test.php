<?php
// sites/all/modules/dh_courseviewer/tests/dcv_teacher_sees_test.php
//
// Teachers in the Dipi App get the course cut to their side — "Sees" on their mapping (M / F /
// B; empty = their own gender), decided by Dipi's AT course view rule (_at_view_sides). Runs
// against THIS worktree's code. Every row it creates or changes (teacher, mappings, centre
// link, the app permission on the AT Portal role, two servers, the seat plan kind) is inside
// one transaction that is rolled back at the end. Synthetic fixture course 9900002 (centre 5).
if (PHP_SAPI !== 'cli') { exit; }
define('DRUPAL_ROOT', realpath(__DIR__ . '/../../../../..')); chdir(DRUPAL_ROOT);
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
foreach (array('dcv-api.inc', 'dcv-bundle.inc', 'dcv-plans.inc') as $f) {
  require_once DRUPAL_ROOT . '/sites/all/modules/dh_courseviewer/inc/' . $f;
}
function ok($c, $m) { echo ($c ? "PASS" : "FAIL") . " - $m\n"; if (!$c) { $GLOBALS['fail'] = 1; } }
function ids($l) { return array_map(function ($c) { return $c['id']; }, $l); }
function as_user($uid, $name, $rids) {
  global $user;
  $roles = array(DRUPAL_AUTHENTICATED_RID => 'authenticated user');
  foreach ($rids as $r) { $roles[$r] = "r$r"; }
  $user = (object) array('uid' => $uid, 'name' => $name, 'roles' => $roles);
  drupal_static_reset('user_access');
}
function listed_plans($list, $course) {
  foreach ($list as $c) { if ($c['id'] === $course) { return ids($c['plans']); } }
  return NULL;
}
const COURSE = 9900002;
const DAY = '2026-10-01';
$at_rid = (int) db_query("select rid from {role} where name='AT Portal'")->fetchField();
$ca_rid = (int) db_query("select rid from {role} where name='Centre Admin'")->fetchField();
ok($at_rid > 0 && $ca_rid > 0, 'fixtures: AT Portal + Centre Admin roles');
ok(function_exists('_at_view_sides'), "Dipi's AT course view rule is loaded (dh_atportal)");
if (!empty($GLOBALS['fail'])) { echo "FAILURES\n"; exit(1); }

// Whether the real AT Portal role has the app switch is not this test's business (it is granted
// on the local and live role after deploy): note it now, compare after the rollback.
$had_app = (int) db_query("select count(*) from {role_permission} where rid = :r and permission = 'dcv teacher access'",
  array(':r' => $at_rid))->fetchField();

$txn = db_transaction();
try {
  // Start from a known state, whatever the real role has: no app switch, the AT course view on.
  user_role_revoke_permissions($at_rid, array('dcv teacher access'));
  user_role_grant_permissions($at_rid, array('at view courses'));
  $tid = (int) db_insert('dh_teacher')->fields(array(
    't_code' => 'ZZDCV2', 't_gender' => 'M', 't_f_name' => 'Test', 't_status' => 'Active',
    't_created_by' => 1, 't_updated_by' => 1,
  ))->execute();
  $uid = 999991; $name = 'ZZDCV2.M';   // a fake account id; Dipi's AT username = t_code.t_gender
  $map = function ($type, $sees) use ($tid) {
    return db_insert('dh_course_teacher')->fields(array(
      'ct_course' => COURSE, 'ct_teacher' => $tid, 'ct_type' => $type, 'ct_status' => 'Confirmed',
      'ct_sees' => $sees, 'ct_group' => 0, 'ct_created_by' => 1, 'ct_updated' => date('Y-m-d H:i:s'), 'ct_updated_by' => 1,
    ))->execute();
  };
  $ct = $map('Conducting', NULL);
  $sees = function ($ct_id, $v) { db_update('dh_course_teacher')->fields(array('ct_sees' => $v))->condition('ct_id', $ct_id)->execute(); };
  $first = function ($gender) {
    return (int) db_query("select a_id from dh_applicant where a_course = :c and a_attended = 1 and a_type = 'Student'
      and a_gender = :g order by a_id limit 1", array(':c' => COURSE, ':g' => $gender))->fetchField();
  };
  $sm = $first('M'); $sf = $first('F');
  db_update('dh_applicant')->fields(array('a_type' => 'Sevak'))->condition('a_id', array($sm, $sf), 'IN')->execute();
  $nM = (int) db_query("select count(*) from dh_applicant where a_course = :c and a_attended = 1 and a_type = 'Student' and a_gender = 'M'",
    array(':c' => COURSE))->fetchField();

  as_user($uid, $name, array($at_rid));
  ok(dcv_teacher_id() === FALSE && dcv_course_access(COURSE) === FALSE, "without 'dcv teacher access': refused");
  user_role_grant_permissions($at_rid, array('dcv teacher access'));
  ok(dcv_teacher_id() === $tid, 'the AT resolves to their teacher id');

  // --- no Sees → their own gender (M)
  ok(dcv_teacher_sides($tid, COURSE) === array('M'), 'no Sees: own gender');
  $list = dcv_build_course_list(array(), $tid, DAY);
  ok(listed_plans($list, COURSE) === array('RIGHT'), 'list: only the Male plan');
  foreach ($list as $c) {
    if ($c['id'] === COURSE) { ok($c['plans'][0]['students'] === $nM && $c['plans'][0]['servers'] === 1, 'list: Male counts only'); }
  }
  ok(dcv_course_access(COURSE) === array('M'), 'access: their side');
  ok(dcv_user_can_access_course(COURSE), 'can access: yes');
  ok(dcv_bundle_scope(COURSE, NULL) === array(array('RIGHT'), array('M')), 'no ?plans= (older apps): their plans only');
  ok(dcv_bundle_scope(COURSE, 'RIGHT') === array(array('RIGHT'), array('M')), 'own plan: OK');
  ok(dcv_bundle_scope(COURSE, 'LEFT') === 'Unknown plan.' && dcv_bundle_scope(COURSE, 'RIGHT,LEFT') === 'Unknown plan.', "the other side's plan: refused");
  ok(dcv_bundle_scope(COURSE, 'x') === 'Unknown plan.', 'malformed: refused');
  list($p, $s) = dcv_bundle_scope(COURSE, NULL);
  $b = dcv_build_bundle(COURSE, $p, $s);
  $g = array(); foreach ($b['students'] as $st) { $g[$st['identity']['gender']] = TRUE; }
  ok(array_keys($g) === array('M') && count($b['students']) === $nM, 'bundle: Male students only');
  ok(array_map('intval', array_keys($b['servers'])) === array($sm), 'bundle: the Male server only');
  ok(array_map(function ($x) { return $x['key']; }, $b['sections']) === array('RIGHT'), 'bundle: Male section only');
  $json = json_encode($b); $leak = 0;
  foreach (db_query("select concat_ws(' ', nullif(trim(a_f_name),''), nullif(trim(a_m_name),''), nullif(trim(a_l_name),'')) as n
      from dh_applicant where a_course = :c and a_attended = 1 and a_gender <> 'M'", array(':c' => COURSE)) as $r) {
    if ($r->n !== '' && strpos($json, json_encode($r->n)) !== FALSE) { $leak++; }
  }
  ok($leak === 0, 'bundle: no Female name anywhere in the JSON');

  // --- Sees F, then B
  $sees($ct, 'F');
  ok(dcv_teacher_sides($tid, COURSE) === array('F'), 'Sees F');
  ok(dcv_bundle_scope(COURSE, NULL) === array(array('LEFT'), array('F')), 'Sees F: the Female plan');
  ok(listed_plans(dcv_build_course_list(array(), $tid, DAY), COURSE) === array('LEFT'), 'Sees F: list shows LEFT only');
  $sees($ct, 'B');
  ok(dcv_teacher_sides($tid, COURSE) === array('M', 'F'), 'Sees B: both');
  ok(dcv_bundle_scope(COURSE, NULL) === array(array('RIGHT', 'LEFT'), array('M', 'F')), 'Sees B: both plans');
  ok(listed_plans(dcv_build_course_list(array(), $tid, DAY), COURSE) === array('RIGHT', 'LEFT'), 'Sees B: both listed');

  // --- a Female teacher with no Sees → Female
  $sees($ct, NULL);
  db_update('dh_teacher')->fields(array('t_gender' => 'F'))->condition('t_id', $tid)->execute();
  ok(dcv_teacher_sides($tid, COURSE) === array('F'), 'female teacher, no Sees: Female');
  db_update('dh_teacher')->fields(array('t_gender' => 'M'))->condition('t_id', $tid)->execute();

  // --- two mappings → only what both allow
  $sees($ct, 'M');
  $ct2 = $map('Assisting', 'F');
  ok(dcv_teacher_sides($tid, COURSE) === array(), 'two mappings M + F: nothing in common');
  ok(dcv_course_access(COURSE) === FALSE && dcv_bundle_scope(COURSE, NULL) === 'Not authorised for this course.', '… so the course is refused');
  ok(listed_plans(dcv_build_course_list(array(), $tid, DAY), COURSE) === NULL, '… and not listed');
  $sees($ct2, 'B');
  ok(dcv_teacher_sides($tid, COURSE) === array('M'), 'two mappings M + B: Male');

  // --- trainee mappings only → not a teacher of the course
  db_update('dh_course_teacher')->fields(array('ct_type' => 'Training'))->condition('ct_id', array($ct, $ct2), 'IN')->execute();
  ok(dcv_teacher_sides($tid, COURSE) === FALSE && dcv_course_access(COURSE) === FALSE, 'trainee mappings: nothing');
  db_update('dh_course_teacher')->fields(array('ct_type' => 'Conducting', 'ct_sees' => NULL))->condition('ct_id', $ct)->execute();

  // --- an inactive teacher is not a teacher of the course
  ok(dcv_teacher_sides($tid, COURSE) === array('M'), 'active teacher: Male');
  db_update('dh_teacher')->fields(array('t_status' => 'Inactive'))->condition('t_id', $tid)->execute();
  ok(dcv_teacher_sides($tid, COURSE) === FALSE, 'inactive teacher: not a teacher of the course');
  db_update('dh_teacher')->fields(array('t_status' => 'Active'))->condition('t_id', $tid)->execute();
  ok(dcv_teacher_sides($tid, COURSE) === array('M'), 'active again: Male');

  // --- dual role: a centre user of centre 5 who also teaches → the full course
  db_insert('dh_user_center')->fields(array('uc_user' => $uid, 'uc_center' => 5, 'uc_deleted' => 0,
    'uc_created_by' => 1, 'uc_updated' => date('Y-m-d H:i:s'), 'uc_updated_by' => 1))->execute();
  as_user($uid, $name, array($at_rid, $ca_rid));
  ok(dcv_course_access(COURSE) === 'full', 'dual role: the centre role wins');
  ok(dcv_bundle_scope(COURSE, NULL) === array(NULL, NULL), 'dual role: the whole course');
  ok(listed_plans(dcv_build_course_list(dcv_scope_centres(), dcv_teacher_id(), DAY), COURSE) === array('RIGHT', 'LEFT'), 'dual role: both plans listed');

  // --- without the app permission → not a teacher
  as_user($uid, $name, array($at_rid));
  user_role_revoke_permissions($at_rid, array('dcv teacher access'));
  drupal_static_reset('user_access');
  ok(dcv_teacher_id() === FALSE && dcv_course_access(COURSE) === FALSE, "no 'dcv teacher access': refused");
  user_role_grant_permissions($at_rid, array('dcv teacher access'));
  drupal_static_reset('user_access');

  // --- the app switch on but Dipi's AT course view off → not a teacher either
  user_role_revoke_permissions($at_rid, array('at view courses'));
  drupal_static_reset('user_access');
  ok(user_access('dcv teacher access') && dcv_teacher_id() === FALSE && dcv_course_access(COURSE) === FALSE,
    "'dcv teacher access' without 'at view courses': refused");
  user_role_grant_permissions($at_rid, array('at view courses'));
  drupal_static_reset('user_access');
  ok(dcv_teacher_id() === $tid, "both on again: the AT is a teacher");

  // --- group-wise: the Male side gets every Male group and nothing else
  db_update('dh_course')->fields(array('c_seat_plan' => 1))->condition('c_id', COURSE)->execute();
  foreach (db_query("select a_id, a_gender from dh_applicant where a_course = :c and a_attended = 1", array(':c' => COURSE)) as $r) {
    $grp = ($r->a_gender === 'M' && $r->a_id % 2 === 0) ? 1 : 2;
    db_update('dh_applicant_attended')->fields(array('aa_group' => $grp))->condition('aa_applicant', $r->a_id)->execute();
  }
  $gp = ids(dcv_course_plans(COURSE, array('M')));
  ok($gp === array('RIGHT-1', 'RIGHT-2'), 'group-wise, side M: the Male group plans only');
  ok(dcv_bundle_scope(COURSE, NULL) === array(array('RIGHT-1', 'RIGHT-2'), array('M')), 'group-wise: no ?plans= → their group plans');
  ok(dcv_bundle_scope(COURSE, 'LEFT-2') === 'Unknown plan.', 'group-wise: a Female group refused');
}
finally {
  $txn->rollback();
}
drupal_static_reset('user_access');
ok((int) db_query("select count(*) from dh_teacher where t_code = 'ZZDCV2'")->fetchField() === 0
  && (int) db_query("select count(*) from {role_permission} where rid = :r and permission = 'dcv teacher access'", array(':r' => $at_rid))->fetchField() === $had_app
  && dh_course_plan(COURSE, 'seat') === 'main', 'rolled back: no test teacher, the role keeps the app switch it had, fixture as before');

echo empty($GLOBALS['fail']) ? "ALL PASS\n" : "FAILURES\n";
exit(empty($GLOBALS['fail']) ? 0 : 1);
