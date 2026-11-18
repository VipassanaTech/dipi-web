<?php
// Link finder (dh_referral_link_find) and finalize's student match skipping open referral copies (dh_student_find).
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-referral-link.php
require __DIR__ . '/lib.php';

t_in_txn(function () {
  $s = t_make_student(array('s_f_name' => 'Zzqtest', 's_l_name' => 'Linkone', 's_email' => 'zzqlink1@example.invalid', 's_pancard' => 'ZZQLK1234Z'));
  $who = array('a_f_name' => 'Zzqtest', 'a_l_name' => 'Linkone', 'a_dob' => '', 'a_email' => 'zzqlink1@example.invalid', 'a_phone_mobile' => '',
    'a_aadhar' => '', 'a_pancard' => '', 'a_voter_id' => '', 'a_passport' => '');
  t_eq(dh_referral_link_find($who), $s, 'N1 name + email finds the student');
  t_eq(dh_referral_link_find(array('a_pancard' => 'ZZQLK1234Z', 'a_email' => '') + $who), $s, 'N2 PAN + same first name finds the student');
  t_eq(dh_referral_link_find(array('a_f_name' => 'Other', 'a_pancard' => 'ZZQLK1234Z', 'a_email' => '') + $who), 0, 'N3 PAN with another first name and no date of birth: no link');
  t_eq(dh_referral_link_find(array('a_pancard' => 'NO', 'a_email' => '') + $who), 0, 'N4 a placeholder ID never links');

  db_insert('dh_referral_copy')->fields(array('rc_student' => $s, 'rc_referral' => 0, 'rc_group' => 'none', 'rc_status' => 'open', 'rc_created' => date('Y-m-d H:i:s')))->execute();
  t_eq(dh_referral_link_find($who), 0, 'O1 an open referral copy is never found');
  t_eq(dh_student_find(array('a_f_name' => 'Zzqtest', 'a_l_name' => 'Linkone', 'a_email' => 'zzqlink1@example.invalid'))['sid'], 0, 'O2 finalize\'s match skips it too');
  db_update('dh_referral_copy')->fields(array('rc_status' => 'kept'))->condition('rc_student', $s)->execute();
  t_eq(dh_referral_link_find($who), $s, 'O3 a kept copy is a normal student again');
  t_eq(dh_referral_link_find($who, array($s)), 0, 'O4 an explicit skip list is honoured');
  t_eq(dh_referral_link_find($who, array()), $s, 'O5 an empty skip list skips nothing');
});
t_done();
