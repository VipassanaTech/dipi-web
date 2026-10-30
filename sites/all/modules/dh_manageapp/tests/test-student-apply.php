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

  // 4. Own record (course record of this course from this applicant, none for another course): everything corrected even on a weak match.
  $own = t_make_student(array('s_l_name' => 'Typo', 's_gender' => 'F'));
  $a3 = db_query("select * from dh_applicant where a_id = :a", array(':a' => t_make_applicant($course, array('a_l_name' => 'Fixed', 'a_gender' => 'M'))))->fetchAssoc();
  db_insert('dh_student_course')->fields(array('sc_student' => $own, 'sc_center' => 1, 'sc_course' => $course, 'sc_applicant' => $a3['a_id']))->execute();
  dh_student_apply($own, $a3, FALSE, array('applicant' => $a3['a_id']) + $ctx);
  t_eq($row($own)['s_l_name'], 'Fixed', 'own record: name corrected');
  t_eq($row($own)['s_gender'], 'M', 'own record: gender corrected');

  // 5. In a back-fill batch every changed field is logged for undo, with the old and the new value.
  db_update('dh_student')->fields(array('s_address' => 'Before batch address'))->condition('s_id', $own)->execute();
  dh_student_apply($own, array_merge($a3, array('a_address' => 'Batch address')), FALSE, array('applicant' => $a3['a_id'], 'batch' => 'test-batch') + $ctx);
  $bl = db_query("select sl_old, sl_new, sl_applicant, sl_course from dh_student_log where sl_student = :s and sl_action = 'updated' and sl_field = 's_address' and sl_batch = 'test-batch'", array(':s' => $own))->fetchAssoc();
  t_eq($bl['sl_old'], 'Before batch address', 'batch update logged with the old value');
  t_eq($bl['sl_new'], 'Batch address', 'batch update logged with the new value');
  t_eq($bl['sl_applicant'], $a3['a_id'], 'batch update logged with the applicant');
  t_eq($bl['sl_course'], $course, 'batch update logged with the course');
  t_eq(db_query("select count(*) from dh_student_log where sl_student = :s and sl_action = 'updated' and sl_field = 's_l_name'", array(':s' => $own))->fetchField(), 0, 'a name change is logged as name-change, not as updated');

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
  db_insert('dh_student_course')->fields(array('sc_student' => $own_real, 'sc_center' => 1, 'sc_course' => $course, 'sc_applicant' => $a_zero['a_id']))->execute();
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

  // 7. An orphan student (no course record at all, not created from this applicant) is a returning student: protected.
  $orphan = t_make_student(array('s_l_name' => 'Orphan', 's_gender' => 'F', 's_dob' => '1970-01-01'));
  $a7 = db_query("select * from dh_applicant where a_id = :a", array(':a' => t_make_applicant($course, array('a_l_name' => 'Changed', 'a_gender' => 'M', 'a_dob' => '1985-05-05', 'a_company' => 'Orphan Co'))))->fetchAssoc();
  dh_student_apply($orphan, $a7, FALSE, array('applicant' => $a7['a_id']) + $ctx);
  $r = $row($orphan);
  t_eq($r['s_l_name'], 'Orphan', 'orphan student: weak match keeps the name');
  t_eq($r['s_gender'], 'F', 'orphan student: gender stays');
  t_eq($r['s_dob'], '1970-01-01', 'orphan student: date of birth stays');
  t_eq($r['s_company'], 'Orphan Co', 'orphan student: an empty company is still filled');

  // 8. A student created from THIS applicant is its own record: corrected on the next apply (own via the created log row).
  $a8 = db_query("select * from dh_applicant where a_id = :a", array(':a' => t_make_applicant($course, array('a_l_name' => 'Firsttyping', 'a_gender' => 'M'))))->fetchAssoc();
  $ctx8 = array('applicant' => $a8['a_id']) + $ctx;
  $s8 = dh_student_apply(0, $a8, FALSE, $ctx8);
  dh_student_apply($s8, array_merge($a8, array('a_l_name' => 'Corrected', 'a_gender' => 'F')), FALSE, $ctx8);
  t_eq($row($s8)['s_l_name'], 'Corrected', 'student created from this applicant: name corrected on a weak match');
  t_eq($row($s8)['s_gender'], 'F', 'student created from this applicant: gender corrected');

  // 9. A second applicant of the same course that resolves to the student the first one created is NOT its owner.
  db_insert('dh_student_course')->fields(array('sc_student' => $s8, 'sc_center' => 1, 'sc_course' => $course, 'sc_applicant' => $a8['a_id']))->execute();
  $a9 = db_query("select * from dh_applicant where a_id = :a", array(':a' => t_make_applicant($course, array('a_l_name' => 'Secondperson', 'a_gender' => 'M', 'a_company' => 'Second Co'))))->fetchAssoc();
  dh_student_apply($s8, $a9, FALSE, array('applicant' => $a9['a_id']) + $ctx);
  t_eq($row($s8)['s_l_name'], 'Corrected', 'second applicant of the course: weak match keeps the name');
  t_eq($row($s8)['s_gender'], 'F', 'second applicant of the course: gender stays');
  t_eq($row($s8)['s_company'], 'Second Co', 'second applicant of the course: an empty box is still filled');
  // A course record of this course with no applicant noted (older data) still counts as the student's own.
  $s9 = t_make_student(array('s_l_name' => 'Nullapp', 's_gender' => 'F'));
  db_insert('dh_student_course')->fields(array('sc_student' => $s9, 'sc_center' => 1, 'sc_course' => $course))->execute();
  dh_student_apply($s9, $a9, FALSE, array('applicant' => $a9['a_id']) + $ctx);
  t_eq($row($s9)['s_l_name'], 'Secondperson', 'course record of this course without an applicant: own, name corrected');

  // 10. Whitespace: a blank is blank, and padding is not a change.
  $ws = t_make_student(array('s_f_name' => 'Zzqtest', 's_l_name' => 'Old', 's_company' => 'Stored Co', 's_address' => 'Stored street'));
  $a10 = db_query("select * from dh_applicant where a_id = :a", array(':a' => t_make_applicant($course, array('a_l_name' => 'Old ', 'a_company' => '   ', 'a_address' => '  New street  '))))->fetchAssoc();
  dh_student_apply($ws, $a10, TRUE, array('applicant' => $a10['a_id'], 'batch' => 'test-batch-ws') + $ctx);
  t_eq($row($ws)['s_company'], 'Stored Co', 'a whitespace-only value does not wipe a stored value');
  t_eq($row($ws)['s_l_name'], 'Old', 'a padded name equal to the stored one leaves it as it was');
  t_eq(db_query("select count(*) from dh_student_log where sl_student = :s and sl_action = 'name-change'", array(':s' => $ws))->fetchField(), 0, 'a padded name writes no false name-change row');
  t_eq(db_query("select count(*) from dh_student_log where sl_student = :s and sl_field = 's_company'", array(':s' => $ws))->fetchField(), 0, 'a whitespace-only value writes no false batch row');
  t_eq($row($ws)['s_address'], 'New street', 'a changed value is stored trimmed');

  // 11. Junk IDs: a stored junk ID counts as empty; a junk new ID is never written.
  $j1 = t_make_student(array('s_l_name' => 'Junkstored', 's_aadhar' => '0'));
  db_insert('dh_student_course')->fields(array('sc_student' => $j1, 'sc_center' => 1, 'sc_course' => $course + 1000000))->execute();   // a returning student, so the ID rule decides
  $j2 = t_make_student(array('s_l_name' => 'Junknew'));
  $aj1 = db_query("select * from dh_applicant where a_id = :a", array(':a' => t_make_applicant($course, array('a_l_name' => 'Junkstored', 'a_aadhar' => '999941057058'))))->fetchAssoc();
  $aj2 = db_query("select * from dh_applicant where a_id = :a", array(':a' => t_make_applicant($course, array('a_l_name' => 'Junknew', 'a_aadhar' => '1111'))))->fetchAssoc();
  dh_student_apply($j1, $aj1, FALSE, array('applicant' => $aj1['a_id']) + $ctx);
  t_eq($row($j1)['s_aadhar'], '999941057058', 'a stored junk ID is filled by a valid one');
  dh_student_apply($j2, $aj2, FALSE, array('applicant' => $aj2['a_id']) + $ctx);
  t_ok($row($j2)['s_aadhar'] === NULL || $row($j2)['s_aadhar'] === '', 'a junk new ID is never written');
  $j3 = t_make_student(array('s_l_name' => 'Validstored', 's_aadhar' => '999941057058'));
  dh_student_apply($j3, array_merge($aj2, array('a_aadhar' => '999971658847')), FALSE, array('applicant' => $aj2['a_id']) + $ctx);
  t_eq($row($j3)['s_aadhar'], '999941057058', 'a stored valid ID is still never replaced');

  // 12. A student row that does not exist is an error for the caller to log, never a silent insert.
  $missing = (int) db_query('select max(s_id) from dh_student')->fetchField() + 1000000;
  $msg = NULL;
  try { dh_student_apply($missing, $a7, FALSE, array('applicant' => $a7['a_id']) + $ctx); } catch (Exception $e) { $msg = $e->getMessage(); }
  t_eq($msg, "Student $missing not found", 'updating a student that does not exist throws');
});
t_done();
