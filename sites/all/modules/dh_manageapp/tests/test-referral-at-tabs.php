<?php
// AT Portal tabs (Active / Inactive / Deleted) with Undelete, which restarts the expiry cron; main list active only.
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-referral-at-tabs.php
require __DIR__ . '/lib.php';

t_eq(array(dh_referral_tab_cond('active'), dh_referral_tab_cond('inactive'), dh_referral_tab_cond('deleted'), dh_referral_tab_cond('x')),
  array('(r_deleted = 0) and (r_end >= curdate())', '(r_deleted = 0) and (r_end < curdate())', '(r_deleted = 1)', '(r_deleted = 0) and (r_end >= curdate())'), 'C1 tab conditions');
t_eq(dh_referral_tab_where(0, ''), '(r_deleted = 0) and (r_end >= curdate())', 'C2 the main list shows active referrals only');

t_in_txn(function () {
  $t = db_query("select t_id from dh_teacher where t_status = 'Active' order by t_id limit 2")->fetchCol();
  $ref = function ($over) use ($t) {
    return (int) db_insert('dh_referral')->fields($over + array('r_student' => 0, 'r_center' => 0, 'r_teacher' => $t[0], 'r_reason' => 'zzq', 'r_listed_by' => 'zzq',
      'r_start' => date('Y-m-d', strtotime('-200 days')), 'r_end' => date('Y-m-d', strtotime('+30 days')), 'r_listed_10d' => 1, 'r_created_by' => 0, 'r_updated_by' => 0,
      'r_moved' => date('Y-m-d H:i:s'), 'r_f_name' => 'Zzqtest', 'r_l_name' => 'Attab'))->execute();
  };
  $before = dh_at_referral_counts($t[0]);
  $a = $ref(array());
  $i = $ref(array('r_end' => date('Y-m-d', strtotime('-5 days'))));
  $old = date('Y-m-d', strtotime('-150 days'));
  $d = $ref(array('r_end' => date('Y-m-d', strtotime('-40 days')), 'r_deleted' => 1, 'r_email_notification_date' => $old));
  $x = $ref(array('r_teacher' => $t[1]));
  $after = dh_at_referral_counts($t[0]);
  t_eq(array($after['active'] - $before['active'], $after['inactive'] - $before['inactive'], $after['deleted'] - $before['deleted']), array(1, 1, 1), 'T1 counts: one per tab, another teacher\'s not counted');
  $ids = function ($tab) use ($t) { return db_query(dh_referral_list_sql(dh_referral_tab_cond($tab) . ' and r_center = 0 and r_teacher = :t'), array(':t' => $t[0]))->fetchCol(2); };
  t_ok(in_array($a, $ids('active')) && in_array($i, $ids('inactive')) && in_array($d, $ids('deleted')) && !in_array($x, $ids('active')), 'T2 each referral on its own tab');

  $res = dh_referral_undelete($d, TRUE);
  $row = db_query('select r_deleted, r_email_notification_date from dh_referral where r_id = :r', array(':r' => $d))->fetchAssoc();
  t_eq(array($res['tab'], $row['r_deleted'], $row['r_email_notification_date']), array('inactive', 0, NULL), 'U1 AT undelete: back under Inactive, expiry-email date cleared');
  $cron_would_delete = db_query('select count(*) from dh_referral where r_id = :r and r_deleted = 0 and r_end < CURRENT_DATE() and datediff(CURRENT_DATE(), r_email_notification_date) > 100
    and r_email_notification_date is not null and r_teacher > 0 and r_readonly = 0', array(':r' => $d))->fetchField();
  t_eq($cron_would_delete, 0, 'U2 the nightly cron will not delete it again that night');
  t_eq(db_query("select count(*) from dh_referral_log where rl_referral = :r and rl_event = 'Undeleted'", array(':r' => $d))->fetchField(), 1, 'U3 logged');
  $centre = (int) db_query('select min(c_id) from dh_center where c_id > 0')->fetchField();
  $c = $ref(array('r_center' => $centre, 'r_deleted' => 1, 'r_teacher' => NULL));
  t_eq(dh_referral_undelete($c, TRUE), FALSE, 'U4 the AT undelete never touches a centre referral');
  t_ok(dh_referral_undelete($c) !== FALSE, 'U5 the centre undelete still works');
});
t_done();
