<?php
// Referral form save (dh_referral_save): person details on the referral, never dh_student; per-field log; link on add and edit.
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-referral-form.php
require __DIR__ . '/lib.php';

function t_input($over = array()) {
  return $over + array('s_f_name' => 'zzqtest', 's_m_name' => '', 's_l_name' => 'formone', 's_gender' => 'M', 's_dob' => array('date' => '1980-04-05'),
    's_email' => 'zzqform1@example.invalid', 's_phone_home' => '', 's_phone_mobile' => '9855555555', 's_phone_office' => '', 's_address' => 'Somewhere',
    's_city' => '', 's_state' => '', 's_country' => 'IN', 's_zip' => '', 's_education' => '', 's_occupation' => '', 's_company' => '', 's_department' => '',
    's_designation' => '', 's_aadhar' => '', 's_pancard' => '', 's_voter_id' => '', 's_passport' => '',
    'sci_teen' => '0', 'sci_10d' => '3', 'sci_stp' => '', 'sci_spl' => '0', 'sci_20d' => '0', 'sci_30d' => '0', 'sci_45d' => '0', 'sci_60d' => '0', 'sci_tsc' => '0', 'sci_service' => '1',
    'r_listed_10d' => '1', 'r_listed_stp' => '', 'r_listed_spl' => '', 'r_listed_20d' => '', 'r_listed_30d' => '', 'r_listed_45d' => '', 'r_listed_60d' => '', 'r_listed_tsc' => '',
    'r_listed_10d_seva' => '', 'r_listed_lc_seva' => '', 'r_listed_longseva' => '', 'r_special' => '0',
    'r_start' => array('date' => date('Y-m-d')), 'r_end' => array('date' => date('Y-m-d', strtotime('+90 days'))), 'r_reason' => 'zzq reason', 'r_listed_by' => 'zzq',
    'r_teacher' => '', 'r_course_center' => '', 'r_course_type' => '', 'r_course_start' => array('date' => ''));
}

t_in_txn(function () {
  $tables = function () { return array((int) db_query('select count(*) from dh_student')->fetchField(), (int) db_query('select count(*) from dh_student_course_input')->fetchField()); };
  $get = function ($r) { return db_query('select * from dh_referral where r_id = :r', array(':r' => $r))->fetchAssoc(); };
  $log = function ($r, $ev) { return db_query('select rl_msg from dh_referral_log where rl_referral = :r and rl_event = :e order by rl_id', array(':r' => $r, ':e' => $ev))->fetchCol(); };
  $real = t_make_student(array('s_f_name' => 'Zzqtest', 's_l_name' => 'Formone', 's_email' => 'zzqform1@example.invalid'));
  $before = $tables();

  $r = dh_referral_save(t_input(), array(), 0, 'referral', 0);
  $g = $get($r);
  t_eq($tables(), $before, 'A1 adding a referral writes no student row and no course-count row');
  t_eq(array($g['r_f_name'], $g['r_l_name'], $g['r_dob'], $g['r_phone_mobile'], $g['r_cnt_10d'], $g['r_cnt_stp'], $g['r_cnt_service'], $g['r_center'], $g['r_start'], $g['r_course_start']),
    array('Zzqtest', 'Formone', '1980-04-05', '9855555555', 3, 0, 1, 0, date('Y-m-d'), NULL), 'A2 details, counts, dates and centre are on the referral');
  t_ok($g['r_moved'] !== NULL, 'A3 a new referral is born moved');
  t_eq((int) $g['r_student'], $real, 'A4 the typed name + email link it to the real student record');
  t_eq(array($log($r, 'Created'), $log($r, 'Link')), array(array('Referral created'), array("Linked to student $real (typed match)")), 'A5 Created and Link lines');

  $snap = db_query('select * from dh_student where s_id = :s', array(':s' => $real))->fetchAssoc();
  dh_referral_save(t_input(array('s_phone_mobile' => '9866666666', 'r_listed_stp' => '1', 's_city' => '')), array(), $r, 'referral', 0);
  $g2 = $get($r);
  t_eq(db_query('select * from dh_student where s_id = :s', array(':s' => $real))->fetchAssoc(), $snap, 'E1 editing never changes the linked student record');
  t_eq($tables(), $before, 'E2 nor adds student rows');
  t_eq(array($g2['r_phone_mobile'], $g2['r_listed_stp'], (int) $g2['r_student']), array('9866666666', 1, $real), 'E3 the edit is saved; the link is kept');
  t_eq($log($r, 'Edited'), array("Mobile: '9855555555' -> '9866666666'", 'Listed for STP: No -> Yes'), 'E4 one log line per changed field');

  $other = t_make_student(array('s_f_name' => 'Zzqtest', 's_l_name' => 'Formtwo', 's_email' => 'zzqform2@example.invalid'));
  dh_referral_save(t_input(array('s_l_name' => 'formtwo', 's_email' => 'zzqform2@example.invalid')), array(), $r, 'referral', 0);
  t_eq((int) $get($r)['r_student'], $other, 'E5 details that now match another person switch the link');
  $links = $log($r, 'Link');
  t_eq(end($links), "Link switched from student $real to student $other (typed match)", 'E6 the switch is logged');
  dh_referral_save(t_input(array('s_l_name' => 'formtwo', 's_email' => 'zzqnobody@example.invalid')), array(), $r, 'referral', 0);
  t_eq((int) $get($r)['r_student'], $other, 'E7 details that match nobody keep the link');

  $course = t_make_course();
  $app = t_make_applicant($course, array('a_f_name' => 'Zzqtest', 'a_l_name' => 'Formone', 'a_email' => 'zzqform1@example.invalid', 'a_photo' => 'private://zzq/photo.jpg'));
  $r2 = dh_referral_save(t_input(), array('ref_aid' => $app), 0, 'referral', 0);
  t_eq(array($get($r2)['r_photo'], (int) $get($r2)['r_student']), array('private://zzq/photo.jpg', $real), 'P1 added from an application: photo snapshot, linked by the application');
  t_eq($log($r2, 'Link'), array("Linked to student $real (application $app)"), 'P2 the link says it came from the application');

  $centre = (int) db_query('select min(c_id) from dh_center where c_id > 0')->fetchField();
  $r3 = dh_referral_save(t_input(array('s_l_name' => 'formnobody', 's_email' => 'zzqnobody3@example.invalid')), array(), 0, 'center-referral', $centre);
  t_eq(array((int) $get($r3)['r_center'], (int) $get($r3)['r_student']), array($centre, 0), 'C1 a centre referral: its centre, no link');

  global $user;
  $keep = $user;
  $t = db_query("select t_id, concat(t_code, '.', t_gender) as login from dh_teacher where t_status = 'Active' order by t_id limit 1")->fetchAssoc();
  $user = (object) array('uid' => 999999995, 'name' => $t['login'], 'roles' => array(2 => 'authenticated user'));
  $r4 = dh_referral_save(t_input(array('r_teacher' => '1', 's_l_name' => 'format', 's_email' => 'zzqat@example.invalid')), array(), 0, 'at-portal', 0);
  $user = $keep;
  t_eq(array((int) $get($r4)['r_teacher'], (int) $get($r4)['r_center']), array((int) $t['t_id'], 0), 'T1 the AT Portal form: the teacher is always the logged-in AT');
  t_eq($tables(), array($before[0] + 1, $before[1]), 'Z1 after every save above, the student tables are unchanged (the only new student row is the test'."'".'s own $other)');
});
t_done();
