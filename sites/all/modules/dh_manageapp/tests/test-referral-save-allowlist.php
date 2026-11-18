<?php
// dh_referral_save takes only the form's referral keys (no mass assignment of r_readonly, r_deleted, r_student, ...),
// and an edit keeps a link that still matches the typed details (no flip between duplicate student records).
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-referral-save-allowlist.php
require __DIR__ . '/lib.php';
require_once __DIR__ . '/test-referral-form-input.inc';

t_in_txn(function () {
  global $user;
  $keep = $user;
  $get = function ($r) { return db_query('select * from dh_referral where r_id = :r', array(':r' => $r))->fetchAssoc(); };
  $s1 = t_make_student(array('s_f_name' => 'Zzqtest', 's_l_name' => 'Allowone', 's_email' => 'zzqallow1@example.invalid'));
  $other = t_make_student(array('s_f_name' => 'Zzqtest', 's_l_name' => 'Allowtwo', 's_email' => 'zzqallow2@example.invalid'));
  $t = db_query("select t_id, concat(t_code, '.', t_gender) as login from dh_teacher where t_status = 'Active' order by t_id limit 1")->fetchAssoc();
  $mk = function () use ($s1) {
    return (int) db_insert('dh_referral')->fields(array('r_student' => $s1, 'r_center' => 0, 'r_reason' => 'zzq', 'r_listed_by' => 'zzq', 'r_readonly' => 1, 'r_deleted' => 0,
      'r_start' => date('Y-m-d'), 'r_end' => date('Y-m-d', strtotime('+30 days')), 'r_listed_10d' => 1, 'r_created_by' => 7, 'r_updated_by' => 0,
      'r_moved' => '2026-01-02 03:04:05', 'r_email_notification_date' => '2026-02-03', 'r_f_name' => 'Zzqtest', 'r_l_name' => 'Allowone', 'r_email' => 'zzqallow1@example.invalid'))->execute();
  };
  $cols = array('r_readonly', 'r_deleted', 'r_student', 'r_moved', 'r_created_by', 'r_email_notification_date', 'r_id', 'r_center');
  foreach (array('referral' => 0, 'at-portal' => 0) as $route => $centre) {
    $r = $mk();
    $before = $get($r);
    if ($route == 'at-portal')
      $user = (object) array('uid' => 999999994, 'name' => $t['login'], 'roles' => array(2 => 'authenticated user'));
    $in = t_input(array('s_l_name' => 'allowone', 's_email' => 'zzqallow1@example.invalid', 'r_reason' => 'changed by allowlist test',
      'r_readonly' => '0', 'r_deleted' => '1', 'r_student' => (string) $other, 'r_moved' => '2000-01-01 00:00:00', 'r_created_by' => '12345',
      'r_email_notification_date' => '2000-01-01', 'r_id' => (string) ($r + 1000), 'r_center' => '9'));
    dh_referral_save($in, array(), $r, $route, $centre);
    $after = $get($r);
    $same = TRUE;
    foreach ($cols as $c) if ((string) $after[$c] !== (string) $before[$c]) $same = FALSE;
    t_ok($same, "M1 [$route] posted r_readonly/r_deleted/r_student/r_moved/r_created_by/r_email_notification_date/r_id/r_center are ignored");
    t_eq($after['r_reason'], 'changed by allowlist test', "M2 [$route] an allowed field did change");
    $user = $keep;
  }

  // F8: two records match the typed details; a referral linked to the HIGHER id keeps it after a no-change save.
  $dup_lo = t_make_student(array('s_f_name' => 'Zzqtest', 's_l_name' => 'Allowdup', 's_email' => 'zzqallowdup@example.invalid'));
  $dup_hi = t_make_student(array('s_f_name' => 'Zzqtest', 's_l_name' => 'Allowdup', 's_email' => 'zzqallowdup@example.invalid'));
  $r = (int) db_insert('dh_referral')->fields(array('r_student' => $dup_hi, 'r_center' => 0, 'r_reason' => 'zzq', 'r_listed_by' => 'zzq',
    'r_start' => date('Y-m-d'), 'r_end' => date('Y-m-d', strtotime('+30 days')), 'r_listed_10d' => 1, 'r_created_by' => 0, 'r_updated_by' => 0,
    'r_moved' => date('Y-m-d H:i:s'), 'r_f_name' => 'Zzqtest', 'r_l_name' => 'Allowdup', 'r_email' => 'zzqallowdup@example.invalid'))->execute();
  dh_referral_save(t_input(array('s_l_name' => 'allowdup', 's_email' => 'zzqallowdup@example.invalid')), array(), $r, 'referral', 0);
  t_eq((int) $get($r)['r_student'], $dup_hi, 'L1 a no-change save keeps a link that still matches (the higher of two duplicates)');
  t_ok($dup_lo < $dup_hi, 'L2 (the duplicates are two different records)');
});
t_done();
