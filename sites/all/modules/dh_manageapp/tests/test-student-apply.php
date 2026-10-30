<?php
require __DIR__ . '/lib.php';
echo "Updating a matched student\n";
t_in_txn(function () {
  $course = t_make_course();
  $ctx = array('course' => $course, 'applicant' => 0, 'batch' => NULL, 'uid' => 0);
  $row = function ($sid) { return db_query("select * from dh_student where s_id = :s", array(':s' => $sid))->fetchAssoc(); };

  // 1. New student.
  $a = db_query("select * from dh_applicant where a_id = :a", array(':a' => t_make_applicant($course, array('a_l_name' => 'Brandnew'))))->fetchAssoc();
  $sid = dh_student_apply(0, $a, FALSE, array('applicant' => $a['a_id']) + $ctx);
  t_ok($sid > 0, 'new student created');
  t_eq($row($sid)['s_l_name'], 'Brandnew', 'new student has the applicant name');
  t_eq($row($sid)['s_first'], 1, 'first course marked');
  t_eq(db_query("select count(*) from dh_student_log where sl_student = :s and sl_action = 'created'", array(':s' => $sid))->fetchField(), 1, 'creation logged');

  // 2. Returning student (has a course record for another course), weak match.
  $old = t_make_student(array('s_l_name' => 'Old', 's_dob' => '1980-01-01', 's_gender' => 'F', 's_email' => 'old@example.invalid', 's_company' => '', 's_address' => 'Old address', 's_aadhar' => '999941057058'));
  db_insert('dh_student_course')->fields(array('sc_student' => $old, 'sc_center' => 1, 'sc_course' => $course + 1000000))->execute();
  $a2 = db_query("select * from dh_applicant where a_id = :a", array(':a' => t_make_applicant($course, array('a_l_name' => 'New', 'a_gender' => 'M', 'a_dob' => '1985-05-05',
    'a_email' => '', 'a_company' => 'Acme', 'a_address' => 'New address', 'a_aadhar' => '999971658847'))))->fetchAssoc();
  dh_student_apply($old, $a2, FALSE, array('applicant' => $a2['a_id']) + $ctx);
  $r = $row($old);
  t_eq($r['s_l_name'], 'Old', 'weak match keeps the name');
  t_eq($r['s_gender'], 'F', 'gender never changes');
  t_eq($r['s_dob'], '1980-01-01', 'date of birth never changes');
  t_eq($r['s_email'], 'old@example.invalid', 'a blank does not wipe the email');
  t_eq($r['s_company'], 'Acme', 'empty company filled');
  t_eq($r['s_address'], 'New address', 'address updated');
  t_eq($r['s_aadhar'], '999941057058', 'a stored ID is never replaced');

  // 3. Strong match changes the name and logs it.
  dh_student_apply($old, $a2, TRUE, array('applicant' => $a2['a_id']) + $ctx);
  t_eq($row($old)['s_l_name'], 'New', 'strong match changes the name');
  t_eq(db_query("select concat(sl_old, '>', sl_new) from dh_student_log where sl_student = :s and sl_action = 'name-change' and sl_field = 's_l_name'", array(':s' => $old))->fetchField(), 'Old>New', 'name change logged old to new');
  t_eq($row($old)['s_gender'], 'F', 'strong match still keeps gender');

  // 4. Own record (no other course): everything corrected even on a weak match.
  $own = t_make_student(array('s_l_name' => 'Typo', 's_gender' => 'F'));
  $a3 = db_query("select * from dh_applicant where a_id = :a", array(':a' => t_make_applicant($course, array('a_l_name' => 'Fixed', 'a_gender' => 'M'))))->fetchAssoc();
  dh_student_apply($own, $a3, FALSE, array('applicant' => $a3['a_id']) + $ctx);
  t_eq($row($own)['s_l_name'], 'Fixed', 'own record: name corrected');
  t_eq($row($own)['s_gender'], 'M', 'own record: gender corrected');

  // 5. In a back-fill batch every changed field is logged for undo.
  dh_student_apply($own, array_merge($a3, array('a_address' => 'Batch address')), FALSE, array('applicant' => $a3['a_id'], 'batch' => 'test-batch') + $ctx);
  t_eq(db_query("select sl_old from dh_student_log where sl_student = :s and sl_action = 'updated' and sl_field = 's_address' and sl_batch = 'test-batch'", array(':s' => $own))->fetchField() !== FALSE, TRUE, 'batch update logged with the old value');

  // 6. A zero or unknown date counts as empty (controller ruling). Zero dates need a lenient session mode to be inserted.
  $mode = db_query('select @@sql_mode')->fetchField();
  db_query("SET SESSION sql_mode = 'STRICT_ALL_TABLES,NO_ENGINE_SUBSTITUTION'");
  $zero_stored = t_make_student(array('s_l_name' => 'Zerodob', 's_dob' => '0000-00-00'));
  db_insert('dh_student_course')->fields(array('sc_student' => $zero_stored, 'sc_center' => 1, 'sc_course' => $course + 1000000))->execute();
  $real_stored = t_make_student(array('s_l_name' => 'Realdob', 's_dob' => '1975-07-07'));
  db_insert('dh_student_course')->fields(array('sc_student' => $real_stored, 'sc_center' => 1, 'sc_course' => $course + 1000000))->execute();
  $own_real = t_make_student(array('s_l_name' => 'Ownreal', 's_dob' => '1976-06-06'));
  $a_real = db_query("select * from dh_applicant where a_id = :a", array(':a' => t_make_applicant($course, array('a_l_name' => 'Zerodob', 'a_dob' => '1990-09-09'))))->fetchAssoc();
  $a_zero = db_query("select * from dh_applicant where a_id = :a", array(':a' => t_make_applicant($course, array('a_l_name' => 'Realdob', 'a_dob' => '0000-00-00'))))->fetchAssoc();
  $a_zero_new = db_query("select * from dh_applicant where a_id = :a", array(':a' => t_make_applicant($course, array('a_l_name' => 'Newzero', 'a_dob' => '0000-00-00'))))->fetchAssoc();
  db_query("SET SESSION sql_mode = :m", array(':m' => $mode));
  t_eq($a_zero['a_dob'], '0000-00-00', 'test applicant really has a zero date of birth');

  dh_student_apply($zero_stored, $a_real, FALSE, array('applicant' => $a_real['a_id']) + $ctx);
  t_eq($row($zero_stored)['s_dob'], '1990-09-09', 'returning student with a zero stored date of birth gets the real one filled');
  dh_student_apply($real_stored, $a_zero, FALSE, array('applicant' => $a_zero['a_id']) + $ctx);
  t_eq($row($real_stored)['s_dob'], '1975-07-07', 'a zero applicant date of birth never overwrites a stored one (returning)');
  dh_student_apply($own_real, array_merge($a_zero, array('a_l_name' => 'Ownreal')), FALSE, array('applicant' => $a_zero['a_id']) + $ctx);
  t_eq($row($own_real)['s_dob'], '1976-06-06', 'a zero applicant date of birth never overwrites a stored one (own record)');
  $nz = dh_student_apply(0, $a_zero_new, FALSE, array('applicant' => $a_zero_new['a_id']) + $ctx);
  t_ok($nz > 0, 'new student from an applicant with a zero date of birth is created');
  t_eq($row($nz)['s_dob'], NULL, 'a zero date of birth is not copied to a new student (left empty)');
});
t_done();
