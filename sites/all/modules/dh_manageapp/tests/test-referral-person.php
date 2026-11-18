<?php
// Referral person details: field map, form row, matching row, change log, link setter, can-edit (inc/referral-person.inc).
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-referral-person.php
require __DIR__ . '/lib.php';

$map = dh_referral_person_map();
t_eq(count($map), 33, 'M1 33 form fields map to referral columns');
t_eq(array($map['s_voter_id'], $map['sci_service'], $map['s_city']), array('r_voter_id', 'r_cnt_service', 'r_city'), 'M2 sample mappings');
$cols = db_query("select column_name from information_schema.columns where table_schema = database() and table_name = 'dh_referral'")->fetchCol();
t_eq(array_values(array_diff($map, $cols)), array(), 'M3 every mapped column exists on dh_referral');
t_eq(array_values(array_diff(array_keys(dh_referral_log_labels()), $cols)), array(), 'M4 every logged column exists on dh_referral');

$ref = array('r_id' => 5, 'r_f_name' => 'Zzqtest', 'r_l_name' => 'Person', 'r_dob' => '1980-02-03', 'r_email' => 'z@example.invalid',
  'r_phone_mobile' => '9800000000', 'r_aadhar' => '', 'r_pancard' => 'ABCDE1234F', 'r_voter_id' => NULL, 'r_passport' => '', 'r_cnt_10d' => 3, 'r_end' => '2027-01-01');
$row = dh_referral_form_row($ref);
t_eq(array($row['s_f_name'], $row['sci_10d'], $row['r_end']), array('Zzqtest', 3, '2027-01-01'), 'F1 form row has s_*, sci_* and r_* keys');
t_eq(dh_referral_who_row($ref), array('a_f_name' => 'Zzqtest', 'a_l_name' => 'Person', 'a_dob' => '1980-02-03', 'a_email' => 'z@example.invalid',
  'a_phone_mobile' => '9800000000', 'a_aadhar' => '', 'a_pancard' => 'ABCDE1234F', 'a_voter_id' => '', 'a_passport' => ''), 'F2 matching row: a_* keys, NULL as empty');
t_eq(array(dh_referral_log_value('r_m_name', NULL), dh_referral_log_value('r_listed_stp', '1'), dh_referral_log_value('r_cnt_10d', '4'), dh_referral_log_value('r_reason', 'x')),
  array('(empty)', 'Yes', '4', "'x'"), 'F3 log values: empty, yes/no, number, quoted text');

$orig_user = $GLOBALS['user'];
t_in_txn(function () {
  global $user;
  $mk = function ($over) {
    return (int) db_insert('dh_referral')->fields($over + array('r_student' => 0, 'r_center' => 0, 'r_reason' => 'zzq', 'r_listed_by' => 'zzq',
      'r_start' => date('Y-m-d'), 'r_end' => date('Y-m-d', strtotime('+30 days')), 'r_created_by' => 0, 'r_updated_by' => 0))->execute();
  };
  $r = $mk(array());
  $n = dh_referral_log_changes($r, array('r_f_name' => 'Old', 'r_m_name' => NULL, 'r_cnt_10d' => '2', 'r_listed_10d' => '0', 'r_end' => '2027-01-01'),
    array('r_f_name' => 'New', 'r_m_name' => '', 'r_cnt_10d' => 2, 'r_listed_10d' => '1', 'r_end' => '2027-01-01'));
  $lines = db_query('select rl_event, rl_msg from dh_referral_log where rl_referral = :r order by rl_id', array(':r' => $r))->fetchAll(PDO::FETCH_ASSOC);
  t_eq($n, 2, 'L1 two fields changed, two lines (NULL equals empty, 2 equals \'2\')');
  t_eq($lines, array(array('rl_event' => 'Edited', 'rl_msg' => "First Name: 'Old' -> 'New'"),
    array('rl_event' => 'Edited', 'rl_msg' => 'Listed for 10-Day: No -> Yes')), 'L2 line text: label, old, new');
  t_eq(dh_referral_log_changes($r, array('r_reason' => 'a'), array('r_reason' => 'a')), 0, 'L3 nothing changed: no line');

  t_ok(dh_referral_set_link($r, 0, 'test') === FALSE, 'K1 the same link (none) changes nothing');
  $s = t_make_student();
  t_ok(dh_referral_set_link($r, $s, 'typed match') === TRUE, 'K2 a new link is set');
  t_eq(db_query('select r_student from dh_referral where r_id = :r', array(':r' => $r))->fetchField(), $s, 'K3 r_student holds the link');
  t_eq(db_query("select rl_msg from dh_referral_log where rl_referral = :r and rl_event = 'Link' order by rl_id desc limit 1", array(':r' => $r))->fetchField(),
    "Linked to student $s (typed match)", 'K4 the link is logged');
  t_eq(dh_referral_link_summary(0), '', 'K5 no link, no summary line');
  t_eq(dh_referral_link_summary($s), 'Student record found: no courses in DIPI yet', 'K6 a linked record without courses');

  // Can edit = the route permission plus the referral's scope.
  $rid = function ($name) { return (int) db_query('select rid from role where name = :n', array(':n' => $name))->fetchField(); };
  $as = function ($uid, $name, $role) use ($rid) { global $user; $user = (object) array('uid' => $uid, 'name' => $name, 'roles' => array(2 => 'authenticated user', $rid($role) => $role)); drupal_static_reset('user_access'); };
  $centre = (int) db_query('select min(c_id) from dh_center where c_id > 0')->fetchField();
  $t = db_query("select t_id, concat(t_code, '.', t_gender) as login from dh_teacher where t_status = 'Active' order by t_id limit 2")->fetchAll(PDO::FETCH_ASSOC);
  $g = $mk(array('r_teacher' => $t[0]['t_id']));
  $o = $mk(array('r_teacher' => $t[1]['t_id']));
  $c = $mk(array('r_center' => $centre));
  $uc = db_query('select * from dh_user_center order by uc_id limit 1')->fetchAssoc();
  unset($uc['uc_id']);
  db_insert('dh_user_center')->fields(array_merge($uc, array('uc_user' => 999999992, 'uc_center' => $centre, 'uc_deleted' => 0)))->execute();
  $as(999999991, 'zzqrefadmin', 'Referral Admin');
  t_eq(array(dh_referral_can_edit($g), dh_referral_can_edit($c)), array(TRUE, FALSE), 'E1 Referral Admin: global yes, centre no');
  $as(999999992, 'zzqcentre', 'Centre Admin');
  t_eq(array(dh_referral_can_edit($c), dh_referral_can_edit($g)), array(TRUE, FALSE), 'E2 Centre Admin of that centre: centre yes, global no');
  $as(999999993, 'zzqother', 'Centre Admin');
  t_eq(dh_referral_can_edit($c), FALSE, 'E3 Centre Admin of another centre: no');
  $as(999999994, $t[0]['login'], 'AT Portal');
  t_eq(array(dh_referral_can_edit($g), dh_referral_can_edit($o), dh_referral_can_edit($c)), array(TRUE, FALSE, FALSE), 'E4 AT: own yes, another teacher\'s no, centre no');
  t_eq(dh_referral_can_edit(0), FALSE, 'E5 an unknown referral: no');
});
$GLOBALS['user'] = $orig_user;
t_done();
