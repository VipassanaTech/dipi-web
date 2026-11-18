<?php
// Referral matching on the referral's typed details OR its linked student's details (set_referral / dh_referral_pick,
// update_referral_flag, dh_referral_twins), and the placeholder prefix of dh_referral_match_sql.
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-referral-match.php
require __DIR__ . '/lib.php';

$who = array('f_name' => 'A', 'l_name' => 'B', 'email' => 'e@x.invalid', 'phone_mobile' => '', 'pancard' => 'ABCDE1234F', 'aadhar' => '', 'voter_id' => '', 'passport' => '');
$m = dh_referral_match_sql($who, 'a.a_');
t_eq($m[0], '(a.a_pancard = :rm_pancard or (a.a_f_name = :rm_f and a.a_l_name = :rm_l and (a.a_email = :rm_email)))', 'P1 default prefix: today\'s SQL text');
$m2 = dh_referral_match_sql($who, 'a.a_', 'rs');
t_eq(array_keys($m2[1]), array(':rs_pancard', ':rs_email', ':rs_f', ':rs_l'), 'P2 prefix rs: placeholders :rs_*');
t_eq(array_keys($m2[3]), array(':rso_pancard'), 'P3 order placeholders :rso_*');

t_in_txn(function () {
  $course = t_make_course(array('c_start' => date('Y-m-d', strtotime('+10 days')), 'c_end' => date('Y-m-d', strtotime('+20 days'))));
  $started = t_make_course();
  $centre = (int) db_query('select c_center from dh_course where c_id = :c', array(':c' => $course))->fetchField();
  $ref = function ($over) {
    return (int) db_insert('dh_referral')->fields($over + array('r_student' => 0, 'r_center' => 0, 'r_reason' => 'zzq', 'r_listed_by' => 'zzq',
      'r_start' => date('Y-m-d'), 'r_end' => date('Y-m-d', strtotime('+60 days')), 'r_listed_10d' => 1, 'r_created_by' => 0, 'r_updated_by' => 0, 'r_moved' => date('Y-m-d H:i:s')))->execute();
  };
  $flag = function ($a) { return (int) db_query('select a_referral from dh_applicant where a_id = :a', array(':a' => $a))->fetchField(); };

  $a1 = t_make_applicant($course, array('a_f_name' => 'Zzqtest', 'a_l_name' => 'Matchone', 'a_email' => 'zzqm1@example.invalid', 'a_attended' => 0));
  $r1 = $ref(array('r_f_name' => 'Zzqtest', 'r_l_name' => 'Matchone', 'r_email' => 'zzqm1@example.invalid'));
  t_eq(dh_referral_pick($a1), $r1, 'M1 typed details match');

  $s2 = t_make_student(array('s_f_name' => 'Zzqtest', 's_l_name' => 'Matchtwo', 's_phone_mobile' => '9833333333'));
  $r2 = $ref(array('r_f_name' => 'Zzqtest', 'r_l_name' => 'Matchtwo', 'r_phone_mobile' => '9800000001', 'r_student' => $s2));
  $a2 = t_make_applicant($course, array('a_f_name' => 'Zzqtest', 'a_l_name' => 'Matchtwo', 'a_phone_mobile' => '9833333333', 'a_attended' => 0));
  t_eq(dh_referral_pick($a2), $r2, 'M2 the linked student\'s newer mobile matches');
  db_update('dh_referral')->fields(array('r_student' => 0))->condition('r_id', $r2)->execute();
  t_eq(dh_referral_pick($a2), 0, 'M3 without the link, the old typed mobile does not match');
  db_update('dh_referral')->fields(array('r_student' => $s2))->condition('r_id', $r2)->execute();

  $r2b = $ref(array('r_f_name' => 'Zzqtest', 'r_l_name' => 'Matchtwo', 'r_phone_mobile' => '9833333333'));
  db_update('dh_applicant')->fields(array('a_referral' => $r2b, 'a_updated_by' => 0))->condition('a_id', $a2)->execute();
  t_eq(dh_referral_pick($a2), $r2b, 'M4 two match: the current flag is kept');

  $r3 = $ref(array('r_f_name' => 'Zzqtest', 'r_l_name' => 'Matchthree', 'r_email' => 'zzqm3@example.invalid', 'r_deleted' => 1));
  $other = (int) db_query('select c_id from dh_center where c_id > 0 and c_id <> :c order by c_id limit 1', array(':c' => $centre))->fetchField();
  $r3b = $ref(array('r_f_name' => 'Zzqtest', 'r_l_name' => 'Matchthree', 'r_email' => 'zzqm3@example.invalid', 'r_center' => $other));
  $a3 = t_make_applicant($course, array('a_f_name' => 'Zzqtest', 'a_l_name' => 'Matchthree', 'a_email' => 'zzqm3@example.invalid', 'a_attended' => 0));
  t_eq(dh_referral_pick($a3), 0, 'M5 a deleted referral and another centre\'s referral never match');

  $a4 = t_make_applicant($course, array('a_f_name' => '', 'a_l_name' => '', 'a_email' => 'zzqm4@example.invalid', 'a_attended' => 0));
  t_eq(dh_referral_pick($a4), 0, 'M6 an email but no name: no crash, no match');

  $r5n = $ref(array('r_f_name' => 'Zzqtest', 'r_l_name' => 'Matchfive', 'r_email' => 'zzqm5@example.invalid'));
  $r5p = $ref(array('r_f_name' => 'Other', 'r_l_name' => 'Person', 'r_pancard' => 'ZZQMF1234Z'));
  $a5 = t_make_applicant($course, array('a_f_name' => 'Zzqtest', 'a_l_name' => 'Matchfive', 'a_email' => 'zzqm5@example.invalid', 'a_pancard' => 'ZZQMF1234Z', 'a_attended' => 0));
  t_eq(dh_referral_pick($a5), $r5p, 'M7 a PAN match ranks before a name match');

  set_referral($a1);
  t_eq($flag($a1), $r1, 'S1 set_referral stores the pick');

  // Referral side: flags applications of courses that have not started, by typed OR linked details.
  $a6 = t_make_applicant($course, array('a_f_name' => 'Zzqtest', 'a_l_name' => 'Matchsix', 'a_email' => 'zzqm6@example.invalid', 'a_attended' => 0));
  $s7 = t_make_student(array('s_f_name' => 'Zzqtest', 's_l_name' => 'Matchsix', 's_phone_mobile' => '9844444444'));
  $a7 = t_make_applicant($course, array('a_f_name' => 'Zzqtest', 'a_l_name' => 'Matchsix', 'a_phone_mobile' => '9844444444', 'a_attended' => 0));
  $a8 = t_make_applicant($started, array('a_f_name' => 'Zzqtest', 'a_l_name' => 'Matchsix', 'a_email' => 'zzqm6@example.invalid', 'a_attended' => 0));
  $r6 = $ref(array('r_f_name' => 'Zzqtest', 'r_l_name' => 'Matchsix', 'r_email' => 'zzqm6@example.invalid', 'r_student' => $s7));
  update_referral_flag($r6);
  t_eq(array($flag($a6), $flag($a7), $flag($a8)), array($r6, $r6, 0), 'U1 typed and linked matches flagged; a started course is not');
  db_update('dh_referral')->fields(array('r_deleted' => 1))->condition('r_id', $r6)->execute();
  update_referral_flag($r6);
  t_eq(array($flag($a6), $flag($a7)), array(0, 0), 'U2 a deleted referral clears its flags');

  // Twin warning (centre referrals): the same typed person, or the same linked student.
  $t1 = $ref(array('r_center' => $centre, 'r_f_name' => 'Zzqtest', 'r_l_name' => 'Twin', 'r_email' => 'zzqtwin@example.invalid'));
  $t2 = $ref(array('r_center' => $centre, 'r_f_name' => 'Zzqtest', 'r_l_name' => 'Twin', 'r_email' => 'zzqtwin@example.invalid'));
  $t3 = $ref(array('r_center' => $centre, 'r_f_name' => 'Zzqtest', 'r_l_name' => 'Twinx', 'r_student' => $s7));
  $t4 = $ref(array('r_center' => $centre, 'r_f_name' => 'Zzqtest', 'r_l_name' => 'Twiny', 'r_student' => $s7));
  t_eq(array_map(function ($t) { return (int) $t->r_id; }, dh_referral_twins($t1)), array($t2), 'W1 same typed person');
  t_eq(array_map(function ($t) { return (int) $t->r_id; }, dh_referral_twins($t3)), array($t4), 'W2 same linked student');
  t_eq(dh_referral_twins($t1)[0]->name, 'Zzqtest Twin', 'W3 the name comes from the referral');
});
t_done();
