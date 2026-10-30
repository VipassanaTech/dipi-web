<?php
// Test helpers: bootstrap, refuse prod, made-up data in a transaction that is always rolled back.
// Run any test from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/<test>.php
if (php_sapi_name() !== 'cli') { echo "Command line only.\n"; exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1'; $_SERVER['HTTP_HOST'] = 'localhost'; $_SERVER['SCRIPT_NAME'] = '/index.php';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
if (stripos(db_query("select @@hostname")->fetchField(), 'vri') !== FALSE) { echo "Refusing: tests run on local only.\n"; exit(1); }

$GLOBALS['t_fail'] = 0; $GLOBALS['t_pass'] = 0; $GLOBALS['t_mail'] = array();
function t_ok($cond, $label) { if ($cond) { $GLOBALS['t_pass']++; echo "  ok   $label\n"; } else { $GLOBALS['t_fail']++; echo "  FAIL $label\n"; } }
// Strict-enough comparison. Database values come back as strings, so '1' equals 1, but NULL, '' and 0 are all different.
// NULL equals only NULL. Arrays are equal when they have the same keys (any order) and equal values (same rules).
// An array never equals a non-array. Anything else is compared as strings with ===.
function t_same($got, $want) {
  if ($got === NULL || $want === NULL) return $got === $want;
  if (is_array($got) || is_array($want)) {
    if (!is_array($got) || !is_array($want) || count($got) != count($want)) return FALSE;
    foreach ($got as $k => $v) if (!array_key_exists($k, $want) || !t_same($v, $want[$k])) return FALSE;
    return TRUE;
  }
  return (string) $got === (string) $want;
}
function t_eq($got, $want, $label) { $same = t_same($got, $want); t_ok($same, $label . ($same ? '' : ' (got ' . var_export($got, TRUE) . ', want ' . var_export($want, TRUE) . ')')); }
function t_done() { echo "\n{$GLOBALS['t_pass']} passed, {$GLOBALS['t_fail']} failed\n"; exit($GLOBALS['t_fail'] ? 1 : 0); }
// Used as mail_fn: records the applicant, sends nothing.
function t_record_mail($type, $a_id, $event) { $GLOBALS['t_mail'][] = (int) $a_id; return array('email_res_id' => 'test'); }
// Runs $fn inside a transaction that is always rolled back.
function t_in_txn($fn) { $txn = db_transaction(); try { $fn(); } catch (Throwable $e) { t_ok(FALSE, 'unexpected error: ' . $e->getMessage()); } $txn->rollback(); }
// A made-up ended course, copied from the newest course row (not a 1-DayOSC), not finalized.
function t_make_course($over = array()) {
  $c = db_query("select c.* from dh_course c join dh_type_detail td on td.td_id = c.c_course_type where c.c_center > 0 and td.td_key <> '1-DayOSC' order by c.c_id desc limit 1")->fetchAssoc();
  unset($c['c_id']);
  $c = array_merge($c, array('c_start' => date('Y-m-d', strtotime('-12 days')), 'c_end' => date('Y-m-d', strtotime('-2 days')),
    'c_finalized' => 0, 'c_processed' => 1, 'c_finalized_tstamp' => NULL, 'c_finalize_requested' => 0, 'c_finalize_mail' => 1,
    'c_reopened' => NULL, 'c_reopened_by' => NULL, 'c_deleted' => 0, 'c_cancelled' => 0), $over);
  return (int) db_insert('dh_course')->fields($c)->execute();
}
// A made-up attended applicant: no email, no mobile, no photo, no PDF.
function t_make_applicant($course, $over = array(), $aa = array()) {
  static $n = 0; $n++;
  $centre = db_query("select c_center from dh_course where c_id = :c", array(':c' => $course))->fetchField();
  $a = array_merge(array('a_center' => $centre, 'a_course' => $course, 'a_type' => 'Student', 'a_gender' => 'M', 'a_old' => 0,
    'a_attended' => 1, 'a_status' => 'Confirmed', 'a_f_name' => 'Zzqtest', 'a_l_name' => "Person$n", 'a_dob' => '1980-01-0' . (($n % 9) + 1),
    'a_email' => '', 'a_phone_mobile' => '', 'a_photo' => '', 'a_uri' => '', 'a_aadhar' => '', 'a_pancard' => '', 'a_voter_id' => '',
    'a_passport' => '', 'a_created_by' => 0, 'a_updated_by' => 0), $over);
  $cols = db_query("desc dh_applicant")->fetchCol();
  $id = (int) db_insert('dh_applicant')->fields(array_intersect_key($a, array_flip($cols)))->execute();
  db_insert('dh_applicant_attended')->fields(array_merge(array('aa_applicant' => $id, 'aa_created_by' => 0, 'aa_updated_by' => 0, 'aa_group' => 1), $aa))->execute();
  return $id;
}
// A made-up existing student.
function t_make_student($over = array()) {
  return (int) db_insert('dh_student')->fields(array_merge(array('s_f_name' => 'Zzqtest', 's_l_name' => 'Existing', 's_created_by' => 0,
    's_updated_by' => 0, 's_updated' => date('Y-m-d H:i:s')), $over))->execute();
}
