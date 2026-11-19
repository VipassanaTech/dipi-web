<?php
// deploy/2026-11-18-lc-transfer-decision-fix.php on made-up applications. The script runs as its own process, so
// this test makes committed rows and deletes them all at the end (no transaction).
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-lc-transfer-decision-fix.php
require __DIR__ . '/lib.php';
$script = 'sites/all/modules/dh_manageapp/deploy/2026-11-18-lc-transfer-decision-fix.php';
$log0 = (int) db_query("select max(l_id) from dh_log")->fetchField();

// One made-up LC application in R-ATTransfer: moved there by $uid, course starting in $days, decision $d.
$made = array('a' => array(), 'c' => array());
function t_df_app($uid, $days, $d) {
  global $made;
  $c = db_query("select * from dh_course where c_id = 3440")->fetchAssoc(); unset($c['c_id']);
  $course = (int) db_insert('dh_course')->fields(array_merge($c, array('c_start' => date('Y-m-d', strtotime("$days days")),
    'c_end' => date('Y-m-d', strtotime(($days + 46) . ' days')), 'c_name' => 'Zzqtest 45 Day', 'c_deleted' => 0, 'c_cancelled' => 0,
    'c_finalized' => 0, 'c_reopened' => NULL, 'c_reopened_by' => NULL)))->execute();
  $made['c'][] = $course;   // recorded at once, so cleanup never misses a row
  $aid = (int) db_insert('dh_applicant')->fields(array('a_center' => 6, 'a_course' => $course, 'a_type' => 'Student', 'a_gender' => 'M',
    'a_old' => 1, 'a_status' => 'R-ATReview', 'a_f_name' => 'Zzqtest', 'a_m_name' => '', 'a_l_name' => 'Fix', 'a_dob' => '1980-01-01',
    'a_email' => '', 'a_phone_mobile' => '', 'a_photo' => '', 'a_uri' => '', 'a_created_by' => 0, 'a_updated_by' => 0))->execute();
  $made['a'][] = $aid;
  db_insert('dh_applicant_lc')->fields(array('al_applicant' => $aid, 'al_recommending' => 'Zzqtest Rat', 'al_area_at' => 'Zzqtest Aat',
    'al_recommending_approved' => $d, 'al_area_at_approved' => 'Pending'))->execute();
  _update_status($aid, 'R-ATTransfer', $uid);
  return $aid;
}
function t_df_run($args) {
  global $script;
  return (string) shell_exec('cd ' . escapeshellarg(DRUPAL_ROOT) . ' && php7.4 ' . $script . ' ' . $args . ' 2>&1 | grep -v Deprecated');
}
function t_df_dec($aid) { return db_query("select al_recommending_approved from dh_applicant_lc where al_applicant = :a", array(':a' => $aid))->fetchField(); }

try {
  $x1 = t_df_app(11, '+30', 'Pending');                 // email page, upcoming, Pending -> fix
  $x2 = t_df_app(15, '+30', 'Pending');                 // DIPI (AT had no email) -> skip
  $x3 = t_df_app(11, '-5', 'Pending');                  // email page, course started -> skip
  $x4 = t_df_app(11, '+30', 'Transfer to Registrar');   // already right -> skip
  $x5 = t_df_app(777140, '+30', 'Pending');             // Portal / DIPI person -> skip
  $ids = implode(',', array($x1, $x2, $x3, $x4, $x5));   // (the decision column allows only Pending/Approved/Rejected/Transfer to Registrar)

  $o = t_df_run("--ids=$ids");
  t_ok(strpos($o, "app $x1: will be fixed") !== FALSE, 'D1 dry run lists the email-page transfer to fix');
  t_ok(strpos($o, "app $x2: skip") !== FALSE && strpos($o, "app $x3: skip") !== FALSE && strpos($o, "app $x4: skip") !== FALSE && strpos($o, "app $x5: skip") !== FALSE, 'D1 ... and skips the other four');
  t_eq(t_df_dec($x1), 'Pending', 'D2 a dry run changes nothing');

  $o = t_df_run('--apply');
  t_ok(strpos($o, '--apply needs --ids') !== FALSE && t_df_dec($x1) === 'Pending', 'A0 --apply without --ids refuses');

  $o = t_df_run("--apply --ids=$ids");
  t_eq(t_df_dec($x1), 'Transfer to Registrar', 'A1 apply fixes the email-page transfer');
  t_eq(array(t_df_dec($x2), t_df_dec($x3), t_df_dec($x4), t_df_dec($x5)), array('Pending', 'Pending', 'Transfer to Registrar', 'Pending'), 'A1 ... and leaves the other four');
  $logs = db_query("select l_identifier, l_user from dh_log where l_id > :l and l_event = 'LC Workflow' and l_msg like 'Recommending AT decision set to %'", array(':l' => $log0))->fetchAllKeyed();
  t_ok(isset($logs[$x1]) && count($logs) == 1, 'A2 one Activity Log line for the fixed application');
  $sys = db_query("select td_val1 from dh_type_detail where td_type = 'COURSE-APPLICANT' and td_key = 'COURSE-SYSTEM-UID'")->fetchField();
  t_eq((int) $logs[$x1], (int) $sys, 'A2 ... under the system user');
  t_eq(db_query("select a_status from dh_applicant where a_id = :a", array(':a' => $x1))->fetchField(), 'R-ATTransfer', 'A3 status unchanged');
  t_eq(db_query("select al_recommending from dh_applicant_lc where al_applicant = :a", array(':a' => $x1))->fetchField(), 'Zzqtest Rat', 'A3 recommending AT unchanged');
  $o = t_df_run("--apply --ids=$ids");
  t_ok(strpos($o, "app $x1: skip") !== FALSE, 'A4 a second apply skips them (already fixed)');

  $o = t_df_run("--rollback --ids=$x1,$x2,$x4");
  t_eq(array(t_df_dec($x1), t_df_dec($x2), t_df_dec($x4)), array('Pending', 'Pending', 'Transfer to Registrar'), 'R1 rollback puts back only what the script changed');
}
finally {
  foreach ($made['a'] as $a) {
    db_delete('dh_applicant_lc')->condition('al_applicant', $a)->execute();
    db_delete('dh_applicant')->condition('a_id', $a)->execute();
  }
  foreach ($made['c'] as $c) {
    db_delete('dh_course')->condition('c_id', $c)->execute();
    db_delete('dh_course_log')->condition('crl_course', $c)->execute();
  }
  if ($made['a'] || $made['c'])
    db_query("delete from dh_log where l_id > :l and l_identifier in (:ids)", array(':l' => $log0, ':ids' => array_map('strval', array_merge($made['a'], $made['c']))));
  echo "  cleanup: " . count($made['a']) . " made-up applications removed\n";
}
t_done();
