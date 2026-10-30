<?php
require __DIR__ . '/lib.php';
echo "Student matching rules\n";
t_ok(dh_aadhaar_valid('999941057058'), 'valid Aadhaar passes');
t_ok(dh_aadhaar_valid('9999 4105 7058'), 'spaces are ignored');
t_ok(!dh_aadhaar_valid('999941057059'), 'wrong check digit fails');
t_ok(!dh_aadhaar_valid('012345678901'), 'cannot start with 0 or 1');
t_ok(!dh_aadhaar_valid('99994105705'), '11 digits fails');
foreach (array('0', 'NA', '111111111111', '12345') as $v) t_ok(dh_id_is_junk('aadhar', $v), "aadhar '$v' is junk");
t_ok(!dh_id_is_junk('aadhar', '999941057058'), 'valid Aadhaar is not junk');
t_ok(dh_id_is_junk('pancard', 'ABCDE1234'), 'short PAN is junk');
t_ok(!dh_id_is_junk('pancard', 'ZZZPZ0001Z'), 'PAN in the right format is not junk');
t_ok(dh_id_is_junk('passport', '12345'), 'short passport is junk');
t_ok(!dh_id_is_junk('passport', 'Z1234567'), 'passport of 8 is not junk');
t_ok(dh_id_is_junk('voter_id', 'NA'), 'voter NA is junk');

t_in_txn(function () {
  $base = array('a_f_name' => '', 'a_l_name' => '', 'a_dob' => '', 'a_email' => '', 'a_phone_mobile' => '', 'a_aadhar' => '', 'a_pancard' => '', 'a_voter_id' => '', 'a_passport' => '');
  $s1 = t_make_student(array('s_l_name' => 'Alpha', 's_aadhar' => '999941057058', 's_dob' => '1980-01-01'));
  $m = dh_student_find(array_merge($base, array('a_f_name' => 'Zzqtest', 'a_l_name' => 'Changed', 'a_aadhar' => '999941057058', 'a_dob' => '1990-05-05')));
  t_eq($m['sid'], $s1, 'ID + same first name matches'); t_eq($m['strong'], FALSE, 'ID + first name only is not strong');
  $m = dh_student_find(array_merge($base, array('a_f_name' => 'Other', 'a_l_name' => 'Alpha', 'a_aadhar' => '999941057058', 'a_dob' => '1980-01-01')));
  t_eq($m['sid'], $s1, 'ID + same date of birth matches'); t_eq($m['strong'], TRUE, 'ID + same date of birth is strong');
  $m = dh_student_find(array_merge($base, array('a_f_name' => 'Other', 'a_l_name' => 'Alpha', 'a_aadhar' => '999941057058', 'a_dob' => '1991-02-02')));
  t_eq($m['sid'], 0, 'ID alone does not match');
  t_make_student(array('s_l_name' => 'Gamma', 's_aadhar' => '0'));
  $m = dh_student_find(array_merge($base, array('a_f_name' => 'Zzqtest', 'a_l_name' => 'Beta', 'a_aadhar' => '0')));
  t_eq($m['sid'], 0, 'junk ID never matches');
  foreach (array('Ann', 'Bob', 'Cid', 'Dee') as $n) t_make_student(array('s_f_name' => $n, 's_aadhar' => '999971658847'));
  $m = dh_student_find(array_merge($base, array('a_f_name' => 'Ann', 'a_l_name' => 'Delta', 'a_aadhar' => '999971658847')));
  t_eq($m['sid'], 0, 'ID shared by 4 first names never matches');
  $s3 = t_make_student(array('s_l_name' => "D'Souza", 's_email' => 'zzq-dsouza@example.invalid'));
  $m = dh_student_find(array_merge($base, array('a_f_name' => 'Zzqtest', 'a_l_name' => "D'Souza", 'a_email' => 'zzq-dsouza@example.invalid')));
  t_eq($m['sid'], $s3, 'name + email matches, apostrophe included'); t_eq($m['how'], 'name+email', 'how = name+email');
  $m = dh_student_find(array_merge($base, array('a_f_name' => 'Zzqnobody', 'a_l_name' => 'Nobody', 'a_dob' => '1979-12-31')));
  t_eq($m['how'], 'none', 'no match');
});
t_done();
