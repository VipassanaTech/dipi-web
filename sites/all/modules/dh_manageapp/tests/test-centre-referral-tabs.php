<?php
// Centre referral list: Active / Inactive / Deleted tabs, Undelete, and the edit-page centre check
// (inc/referral.inc, ref_id_load + deleted_ref_id_load in dh_manageapp.module).
// Also checks that the global referral list and the centre lists lose no referral.
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-centre-referral-tabs.php
require __DIR__ . '/lib.php';

$centres = db_query("select c_id from dh_center where c_id > 0 order by c_id limit 2")->fetchCol();
list($C, $C2) = $centres;

// The old list condition, before the tabs: every referral that is not deleted.
function t_old_ids($centre) {
  return db_query("select r_id from dh_referral r join dh_student s on s.s_id = r.r_student where (r_deleted = 0) and r_center = :c order by r_id",
    array(':c' => $centre))->fetchCol();
}
function t_tab_ids($centre, $tab) {
  return db_query("select r_id from dh_referral r join dh_student s on s.s_id = r.r_student where ".dh_referral_tab_where($centre, $tab)." and r_center = :c order by r_id",
    array(':c' => $centre))->fetchCol();
}

// L: no referral is lost, for every centre with referrals (real local data, nothing changed)
$all_same = TRUE; $checked = 0;
foreach (db_query("select distinct r_center from dh_referral where r_center <> 0")->fetchCol() as $c) {
  $old = t_old_ids($c);
  $new = array_merge(t_tab_ids($c, 'active'), t_tab_ids($c, 'inactive'));
  sort($new);
  if ($old != $new) $all_same = FALSE;
  $checked++;
}
t_ok($checked > 0 && $all_same, "L1 every centre: Active + Inactive = the old list exactly ($checked centres)");
t_eq(t_tab_ids(0, ''), t_old_ids(0), 'L2 the global referral list shows exactly what it did before');
t_eq(dh_referral_tab_where(0, 'deleted'), '(r_deleted = 0)', 'L3 the global list ignores any tab');

t_in_txn(function () use ($C, $C2) {
  $before = dh_centre_referral_counts($C);
  $mk = function ($centre, $end_days, $deleted, $person = array()) {
    static $n = 0; $n++;
    $s = t_make_student(array_merge(array('s_l_name' => "Reftab$n", 's_email' => "zzqreftab$n@example.invalid"), $person));
    return (int) db_insert('dh_referral')->fields(array('r_student' => $s, 'r_center' => $centre, 'r_reason' => 'zzq test',
      'r_listed_by' => 'zzq', 'r_start' => date('Y-m-d', strtotime('-60 days')), 'r_end' => date('Y-m-d', strtotime("$end_days days")),
      'r_listed_10d' => 1, 'r_deleted' => $deleted, 'r_created_by' => 0, 'r_updated_by' => 0))->execute();
  };
  $A  = $mk($C, '+30', 0);            // active
  $T0 = $mk($C, '+0', 0);             // ends today: still active
  $I  = $mk($C, '-3', 0);             // inactive
  $DA = $mk($C, '+30', 1);            // deleted, end date ahead
  $DI = $mk($C, '-3', 1);             // deleted, end date passed
  $G  = $mk(0, '+30', 0);             // a global referral
  $GD = $mk(0, '+30', 1);             // a deleted global referral
  $X  = $mk($C2, '+30', 1);           // deleted, other centre

  // T: tabs
  $after = dh_centre_referral_counts($C);
  t_eq(array($after['active'] - $before['active'], $after['inactive'] - $before['inactive'], $after['deleted'] - $before['deleted']),
    array(2, 1, 2), 'T1 counts: +2 Active (one ends today), +1 Inactive, +2 Deleted');
  t_ok(in_array($A, t_tab_ids($C, 'active')) && in_array($T0, t_tab_ids($C, 'active')), 'T2 Active holds end date ahead and end date today');
  t_ok(in_array($I, t_tab_ids($C, 'inactive')) && !in_array($I, t_tab_ids($C, 'active')), 'T3 Inactive holds end date passed, and only there');
  t_ok(in_array($DA, t_tab_ids($C, 'deleted')) && in_array($DI, t_tab_ids($C, 'deleted')) && !in_array($X, t_tab_ids($C, 'deleted')), 'T4 Deleted holds this centre\'s deleted ones, both end dates');
  t_ok(!in_array($G, array_merge(t_tab_ids($C, 'active'), t_tab_ids($C, 'inactive'), t_tab_ids($C, 'deleted'))), 'T5 a global referral is on no centre tab');
  t_eq(array(dh_referral_tab_of(0, 1), dh_referral_tab_of(0, 0), dh_referral_tab_of(1, 1), dh_referral_tab_of(1, 0)),
    array('active', 'inactive', 'deleted', 'deleted'), 'T6 tab of a referral');
  $bar = dh_centre_referral_tab_bar($C, 'inactive');
  t_ok(strpos($bar, '<li class="active"><a href="/center-referral/'.$C.'?show=inactive">') !== FALSE, 'T7 the open tab is marked');

  // E: the edit page opens a referral only under its own centre (as an admin, who has every centre)
  $GLOBALS['user'] = user_load(1);
  $load = function ($path, $id) { $_GET['q'] = $path; return ref_id_load($id); };
  t_eq($load("center-referral/$C/edit/$A", $A), $A, 'E1 own centre: opens');
  t_eq($load("center-referral/$C/edit/$I", $I), $I, 'E2 an inactive one opens for editing');
  t_eq($load("center-referral/$C2/edit/$A", $A), FALSE, 'E3 under another centre\'s address: refused');
  t_eq($load("center-referral/$C/edit/$G", $G), FALSE, 'E4 a global referral under a centre address: refused');
  t_eq($load("center-referral/0/edit/$A", $A), FALSE, 'E5 centre 0 address: refused');
  t_eq($load("center-referral/$C/edit/$DA", $DA), FALSE, 'E6 a deleted one does not open for editing');
  t_eq($load("referral/0/edit/$G", $G), $G, 'E7 the global edit page is unchanged');
  t_eq($load("referral/0/$G/delete", $G), $G, 'E8 the global delete page is unchanged');
  t_eq(array(deleted_ref_id_load($DA, $C), deleted_ref_id_load($DA, $C2), deleted_ref_id_load($A, $C), deleted_ref_id_load($GD, 0), deleted_ref_id_load('1 or 1', $C)),
    array($DA, FALSE, FALSE, FALSE, FALSE), 'E9 Undelete page: only a deleted referral of that centre');

  // W: same person with another referral at the centre
  $twin = $mk($C, '+30', 1, array('s_l_name' => 'Reftwin', 's_email' => 'zzqtwin@example.invalid'));
  $twin_live = $mk($C, '-3', 0, array('s_l_name' => 'Reftwin', 's_email' => 'zzqtwin@example.invalid'));
  $twin_other = $mk($C2, '+30', 0, array('s_l_name' => 'Reftwin', 's_email' => 'zzqtwin@example.invalid'));
  $tw = dh_referral_twins($twin);
  t_eq(array_map(function ($r) { return array($r->r_id, $r->active); }, $tw), array(array($twin_live, 0)), 'W1 the inactive twin at the same centre is found (not the other centre\'s)');
  t_eq(dh_referral_twins($DA), array(), 'W2 a different person has no twin');

  // U: Undelete
  $course = t_make_course(array('c_center' => $C, 'c_start' => date('Y-m-d', strtotime('+10 days')), 'c_end' => date('Y-m-d', strtotime('+20 days'))));
  $da_person = db_query("select s_f_name, s_l_name, s_email from dh_referral join dh_student on s_id = r_student where r_id = :r", array(':r' => $DA))->fetchAssoc();
  $app = t_make_applicant($course, array('a_f_name' => $da_person['s_f_name'], 'a_l_name' => $da_person['s_l_name'], 'a_email' => $da_person['s_email'], 'a_referral' => 0));
  $res = dh_referral_undelete($DA);
  t_eq($res['tab'], 'active', 'U1 end date ahead: back under Active');
  t_eq(db_query("select r_deleted from dh_referral where r_id = :r", array(':r' => $DA))->fetchField(), 0, 'U2 no longer deleted');
  t_eq(db_query("select rl_event from dh_referral_log where rl_referral = :r order by rl_id desc limit 1", array(':r' => $DA))->fetchField(), 'Undeleted', 'U3 activity log: Undeleted');
  t_eq(db_query("select a_referral from dh_applicant where a_id = :a", array(':a' => $app))->fetchField(), $DA, 'U4 the matching application in a course not yet started is flagged again');
  t_eq(dh_referral_undelete($DA), FALSE, 'U5 a second Undelete does nothing');
  $res = dh_referral_undelete($DI);
  t_eq($res['tab'], 'inactive', 'U6 end date passed: back under Inactive');
  t_ok(strpos(db_query("select rl_msg from dh_referral_log where rl_referral = :r order by rl_id desc limit 1", array(':r' => $DI))->fetchField(), 'Inactive') !== FALSE, 'U7 the log line says Inactive');
  t_eq(array(dh_referral_undelete($A), dh_referral_undelete($GD)), array(FALSE, FALSE), 'U8 not deleted / global: refused');
  $res = dh_referral_undelete($twin);
  t_eq(count($res['twins']), 1, 'U9 Undelete reports the twin');
  $final = dh_centre_referral_counts($C);
  t_eq(array($final['active'] - $before['active'], $final['inactive'] - $before['inactive'], $final['deleted'] - $before['deleted']),
    array(4, 3, 0), 'U10 counts after the three undeletes');
});

t_done();
