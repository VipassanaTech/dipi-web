<?php
// AT Portal: full-form edit of the AT's own referrals only, delete refuses r-only and is logged.
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-referral-at-edit.php
require __DIR__ . '/lib.php';

$items = dh_atportal_menu();
t_eq(isset($items['at-portal/referral/edit/%atrefedit']) ? $items['at-portal/referral/edit/%atrefedit']['page callback'] : '', 'dh_addedit_referral', 'R1 the AT edit route uses the referral form');

$keep = $GLOBALS['user'];
t_in_txn(function () {
  global $user;
  $t = db_query("select t_id, concat(t_code, '.', t_gender) as login from dh_teacher where t_status = 'Active' order by t_id limit 2")->fetchAll(PDO::FETCH_ASSOC);
  $ref = function ($over) {
    return (int) db_insert('dh_referral')->fields($over + array('r_student' => 0, 'r_center' => 0, 'r_reason' => 'zzq', 'r_listed_by' => 'zzq',
      'r_start' => date('Y-m-d'), 'r_end' => date('Y-m-d', strtotime('+30 days')), 'r_listed_10d' => 1, 'r_created_by' => 0, 'r_updated_by' => 0,
      'r_moved' => date('Y-m-d H:i:s'), 'r_f_name' => 'Zzqtest', 'r_l_name' => 'Atedit'))->execute();
  };
  $own = $ref(array('r_teacher' => $t[0]['t_id']));
  $own_del = $ref(array('r_teacher' => $t[0]['t_id'], 'r_deleted' => 1));
  $theirs = $ref(array('r_teacher' => $t[1]['t_id']));
  $own_ro = $ref(array('r_teacher' => $t[0]['t_id'], 'r_readonly' => 1));
  $user = (object) array('uid' => 999999996, 'name' => $t[0]['login'], 'roles' => array(2 => 'authenticated user'));
  t_eq(array(atrefedit_load($own), atrefedit_load($own_del), atrefedit_load($theirs), atrefedit_load('x')), array($own, FALSE, FALSE, FALSE),
    'L1 edit opens only the AT\'s own referral that is not deleted');
  t_eq(atrefedit_load($own_ro), $own_ro, 'L2 an r-only referral can be edited (decision 9)');

  $app = db_query('select * from dh_referral where r_id = :r', array(':r' => $own))->fetchAssoc();
  $in = dh_referral_form_row($app);
  foreach (array('s_dob', 'r_start', 'r_end', 'r_course_start') as $k)
    $in[$k] = array('date' => $k === 's_dob' ? '' : (string) $in[$k]);
  $in['s_phone_mobile'] = '9877777777';
  $in['r_teacher'] = $t[1]['t_id'];                         // a tampered teacher is ignored
  dh_referral_save($in, array(), $own, 'at-portal', 'referral');
  $after = db_query('select r_teacher, r_center, r_phone_mobile from dh_referral where r_id = :r', array(':r' => $own))->fetchAssoc();
  t_eq($after, array('r_teacher' => $t[0]['t_id'], 'r_center' => 0, 'r_phone_mobile' => '9877777777'), 'E1 the AT edit saves; the teacher stays the AT; still global');

  t_eq(dh_at_referral_delete($own_ro), 'readonly', 'D1 an r-only referral is not deleted');
  t_eq(db_query('select r_deleted from dh_referral where r_id = :r', array(':r' => $own_ro))->fetchField(), 0, 'D2 it stays');
  $a = t_make_applicant(t_make_course(), array('a_referral' => $own));
  t_eq(dh_at_referral_delete($own), 'deleted', 'D3 an ordinary referral is deleted');
  t_eq(array((int) db_query('select r_deleted from dh_referral where r_id = :r', array(':r' => $own))->fetchField(), (int) db_query('select a_referral from dh_applicant where a_id = :a', array(':a' => $a))->fetchField()),
    array(1, 0), 'D4 deleted, and its flags cleared');
  t_eq(db_query("select rl_msg from dh_referral_log where rl_referral = :r and rl_event = 'Deleted'", array(':r' => $own))->fetchField(), 'Referral deleted (AT Portal)', 'D5 the delete is logged');
});
$GLOBALS['user'] = $keep;
t_done();
