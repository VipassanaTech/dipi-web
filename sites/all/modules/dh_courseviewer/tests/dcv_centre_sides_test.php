<?php
// sites/all/modules/dh_courseviewer/tests/dcv_centre_sides_test.php
//
// A centre user gets the side(s) of a course that Dipi's gender permissions allow — 'access male'
// / 'access female' ("Access Male / Female Applications"), as on the website's applicant pages
// and lists: both → the whole course; one → that side only; neither → no access. A centre user
// who also teaches the course gets their centre side plus their teacher ("Sees") side.
// Runs against THIS worktree's code. Every row it creates (roles, a teacher, a mapping, a centre
// link) is inside one transaction that is rolled back at the end. Synthetic fixture course
// 9900002 (centre 5, 2026-09-30 → 10-03).
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
function listed_plans($course) {
  foreach (dcv_build_course_list(dcv_scope_centres(), dcv_teacher_id(), DAY, dcv_centre_sides()) as $c) {
    if ($c['id'] === $course) { return ids($c['plans']); }
  }
  return NULL;
}
const COURSE = 9900002;
const DAY = '2026-10-01';

// --- pure rules
ok(dcv_gender_sides(TRUE, TRUE) === array('M', 'F') && dcv_gender_sides(TRUE, FALSE) === array('M')
  && dcv_gender_sides(FALSE, TRUE) === array('F') && dcv_gender_sides(FALSE, FALSE) === array(), 'gender permissions → sides');
ok(dcv_course_sides(TRUE, array('M', 'F'), FALSE) === NULL, 'centre, both genders → the whole course');
ok(dcv_course_sides(TRUE, array('M'), FALSE) === array('M'), 'centre, male only → M');
ok(dcv_course_sides(TRUE, array(), FALSE) === array(), 'centre, neither → nothing');
ok(dcv_course_sides(FALSE, array('M', 'F'), FALSE) === array(), 'not their centre, not a teacher → nothing');
ok(dcv_course_sides(FALSE, array('M', 'F'), array('F')) === array('F'), 'not their centre, teacher F → F');
ok(dcv_course_sides(TRUE, array('M'), array('F')) === array('M', 'F'), 'centre M + teacher F → both sides (exact genders, not the whole course)');
ok(dcv_course_sides(TRUE, array('M', 'F'), array('M')) === NULL, 'centre both + teacher → the whole course');
ok(dcv_course_sides(TRUE, array(), array('F')) === array('F'), 'centre neither + teacher F → F');

$at_rid = (int) db_query("select rid from {role} where name = 'AT Portal'")->fetchField();
$at_perms_before = db_query("select permission from {role_permission} where rid = :r order by permission", array(':r' => $at_rid))->fetchCol();

$txn = db_transaction();
try {
  $role = function ($name, $perms) {
    $r = (object) array('name' => $name); user_role_save($r);
    $rid = (int) db_query("select rid from {role} where name = :n", array(':n' => $name))->fetchField();
    user_role_grant_permissions($rid, $perms);
    return $rid;
  };
  $deo_m = $role('zz dcv deo male', array('access zero day', 'access male'));
  $deo_f = $role('zz dcv deo female', array('access zero day', 'access female'));
  $dzu   = $role('zz dcv day zero', array('access zero day'));
  $ca    = (int) db_query("select rid from {role} where name = 'Centre Admin'")->fetchField();
  $at    = (int) db_query("select rid from {role} where name = 'AT Portal'")->fetchField();
  // The AT Portal role must open the app for this test's teacher, whatever the local switch is.
  user_role_grant_permissions($at, array('dcv teacher access', 'at view courses'));
  $uid = 999992;
  db_insert('dh_user_center')->fields(array('uc_user' => $uid, 'uc_center' => 5, 'uc_deleted' => 0,
    'uc_created_by' => 1, 'uc_updated' => date('Y-m-d H:i:s'), 'uc_updated_by' => 1))->execute();
  // One server per side, so the side limit is checked on servers too.
  $first = function ($g) {
    return (int) db_query("select a_id from dh_applicant where a_course = :c and a_attended = 1 and a_type = 'Student'
      and a_gender = :g order by a_id limit 1", array(':c' => COURSE, ':g' => $g))->fetchField();
  };
  $sm = $first('M'); $sf = $first('F');
  db_update('dh_applicant')->fields(array('a_type' => 'Sevak'))->condition('a_id', array($sm, $sf), 'IN')->execute();
  $nM = (int) db_query("select count(*) from dh_applicant where a_course = :c and a_attended = 1 and a_type = 'Student' and a_gender = 'M'",
    array(':c' => COURSE))->fetchField();

  // --- Data Entry Operator Male: only the Male side
  as_user($uid, 'zz_deo_m', array($deo_m));
  ok(dcv_api_access(), 'DEO Male: may use the app');
  ok(dcv_centre_sides() === array('M'), 'DEO Male: centre side M');
  ok(dcv_course_access(COURSE) === array('M'), 'DEO Male: course access = M');
  ok(listed_plans(COURSE) === array('RIGHT'), 'DEO Male: list shows the Male plan only');
  ok(dcv_bundle_scope(COURSE, NULL) === array(array('RIGHT'), array('M')), 'DEO Male, no ?plans= (older apps): the Male plan only');
  ok(dcv_bundle_scope(COURSE, 'LEFT') === 'Unknown plan.', 'DEO Male: the Female plan is refused');
  list($p, $s) = dcv_bundle_scope(COURSE, NULL);
  $b = dcv_build_bundle(COURSE, $p, $s);
  $g = array(); foreach ($b['students'] as $st) { $g[$st['identity']['gender']] = TRUE; }
  ok(array_keys($g) === array('M') && count($b['students']) === $nM, 'DEO Male: bundle has Male students only');
  ok(array_map('intval', array_keys((array) $b['servers'])) === array($sm), 'DEO Male: the Male server only');
  ok(array_map(function ($x) { return $x['key']; }, $b['sections']) === array('RIGHT')
    && count(array_filter($b['seats'], function ($x) { return $x['section'] !== 'RIGHT'; })) === 0, 'DEO Male: Male section and seats only');
  $json = json_encode($b); $leak = 0;
  foreach (db_query("select concat_ws(' ', nullif(trim(a_f_name),''), nullif(trim(a_m_name),''), nullif(trim(a_l_name),'')) as n
      from dh_applicant where a_course = :c and a_attended = 1 and (a_gender is null or a_gender <> 'M')", array(':c' => COURSE)) as $r) {
    if ($r->n !== '' && strpos($json, json_encode($r->n)) !== FALSE) { $leak++; }
  }
  ok($leak === 0, 'DEO Male: no Female student or server name anywhere in the JSON');

  // --- Data Entry Operator Female: only the Female side
  as_user($uid, 'zz_deo_f', array($deo_f));
  ok(dcv_course_access(COURSE) === array('F') && listed_plans(COURSE) === array('LEFT'), 'DEO Female: Female side only');
  ok(dcv_bundle_scope(COURSE, 'RIGHT') === 'Unknown plan.', 'DEO Female: the Male plan is refused');

  // --- Day Zero User alone (neither gender): no app access
  as_user($uid, 'zz_dzu', array($dzu));
  ok(!dcv_api_access(), 'Day Zero User with neither gender: the app is refused (403)');
  ok(dcv_course_access(COURSE) === FALSE && listed_plans(COURSE) === NULL, '… no course access, nothing listed');
  ok(dcv_bundle_scope(COURSE, NULL) === 'Not authorised for this course.', '… and the download is refused');

  // --- Day Zero User + DEO Female: Female
  as_user($uid, 'zz_dzu_f', array($dzu, $deo_f));
  ok(dcv_api_access() && dcv_course_access(COURSE) === array('F'), 'Day Zero User + DEO Female: Female side');

  // --- Centre Admin (both): unchanged, the whole course
  as_user($uid, 'zz_ca', array($ca));
  ok(dcv_course_access(COURSE) === 'full' && listed_plans(COURSE) === array('RIGHT', 'LEFT'), 'Centre Admin: the whole course, as before');
  ok(dcv_bundle_scope(COURSE, NULL) === array(NULL, NULL), 'Centre Admin: no ?plans= → the whole course');

  // --- DEO Male who also teaches the course with Sees F: both sides, exact genders
  $tid = (int) db_insert('dh_teacher')->fields(array('t_code' => 'ZZDCV3', 't_gender' => 'M', 't_f_name' => 'Test',
    't_status' => 'Active', 't_created_by' => 1, 't_updated_by' => 1))->execute();
  db_insert('dh_course_teacher')->fields(array('ct_course' => COURSE, 'ct_teacher' => $tid, 'ct_type' => 'Assisting',
    'ct_status' => 'Confirmed', 'ct_sees' => 'F', 'ct_group' => 0, 'ct_created_by' => 1,
    'ct_updated' => date('Y-m-d H:i:s'), 'ct_updated_by' => 1))->execute();
  // As on live, the AT Portal role itself holds both Male and Female, so a DEO Male login that is
  // also an AT has both genders — as on the website — and gets the whole course of its centre.
  as_user($uid, 'ZZDCV3.M', array($deo_m, $at));
  ok(dcv_centre_sides() === array('M', 'F') && dcv_course_access(COURSE) === 'full',
    'DEO Male + AT Portal (which holds both genders, as on live): the whole course, as on the website');
  // If the AT Portal role held no gender permission, the same login would get its centre side (M)
  // plus its teacher side (Sees F): both sides, exact genders, not the whole course.
  user_role_revoke_permissions($at, array('access male', 'access female'));
  as_user($uid, 'ZZDCV3.M', array($deo_m, $at));
  ok(dcv_teacher_id() === $tid && dcv_centre_sides() === array('M'), 'DEO Male + teacher: centre side M, teacher id resolved');
  ok(dcv_course_access(COURSE) === array('M', 'F'), 'DEO Male + teacher Sees F: both sides (not "full")');
  ok(listed_plans(COURSE) === array('RIGHT', 'LEFT'), '… both plans listed');
  ok(dcv_bundle_scope(COURSE, NULL) === array(array('RIGHT', 'LEFT'), array('M', 'F')), '… downloads both plans, exact genders only');
}
finally {
  $txn->rollback();
}
drupal_static_reset('user_access');
ok((int) db_query("select count(*) from {role} where name like 'zz dcv %'")->fetchField() === 0
  && (int) db_query("select count(*) from dh_teacher where t_code = 'ZZDCV3'")->fetchField() === 0
  && (int) db_query("select count(*) from dh_user_center where uc_user = 999992")->fetchField() === 0
  && (int) db_query("select count(*) from dh_course_teacher where ct_course = :c and ct_sees = 'F' and ct_teacher not in (select t_id from dh_teacher)", array(':c' => COURSE))->fetchField() === 0
  && (int) db_query("select count(*) from dh_applicant where a_course = :c and a_type = 'Sevak'", array(':c' => COURSE))->fetchField() === 0
  && db_query("select permission from {role_permission} where rid = :r order by permission", array(':r' => $at_rid))->fetchCol() === $at_perms_before,
  'rolled back: no test roles, teacher, mapping, centre link or servers left; AT Portal permissions as before');

echo empty($GLOBALS['fail']) ? "ALL PASS\n" : "FAILURES\n";
exit(empty($GLOBALS['fail']) ? 0 : 1);
