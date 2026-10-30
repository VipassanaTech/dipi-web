<?php
require __DIR__ . '/lib.php';
echo "Sync step\n";
t_in_txn(function () {
  $opts = array('mail_fn' => 't_record_mail', 'files' => 'none');
  $course = t_make_course();
  $ret = t_make_student(array('s_l_name' => 'Returning', 's_dob' => '1975-03-03'));
  db_insert('dh_student_course')->fields(array('sc_student' => $ret, 'sc_center' => 1, 'sc_course' => $course + 1000000))->execute();
  $a = t_make_applicant($course, array('a_l_name' => 'Firsttimer'));
  $b = t_make_applicant($course, array('a_l_name' => 'Returning', 'a_dob' => '1975-03-03'));
  t_make_applicant($course, array('a_l_name' => 'Notattended', 'a_attended' => 0));
  $records = function () use ($course) { return (int) db_query("select count(*) from dh_student_course where sc_course = :c", array(':c' => $course))->fetchField(); };

  $s = dh_course_sync($course, $opts);
  t_eq($s['added'], 2, 'first run adds 2 course records'); t_eq($records(), 2, '2 records in the course');
  t_eq(db_query("select sc_student from dh_student_course where sc_course = :c and sc_applicant = :a", array(':c' => $course, ':a' => $b))->fetchField(), $ret, 'returning applicant joins the existing student');
  t_eq(count($GLOBALS['t_mail']), 2, '2 Attended emails decided');
  t_ok((bool) db_query("select a_attended_mail from dh_applicant where a_id = :a", array(':a' => $a))->fetchField(), 'email date stored');
  t_ok(db_query("select count(*) from dh_course_stat where cs_course = :c", array(':c' => $course))->fetchField() > 0, 'statistics written');

  $s = dh_course_sync($course, $opts);
  t_eq(array($s['added'], $s['updated'], $s['emails']), array(0, 2, 0), 'second run: no new records, no new emails'); t_eq($records(), 2, 'still 2 records');

  db_insert('dh_student_course')->fields(array('sc_student' => $ret, 'sc_center' => 1, 'sc_course' => $course))->execute();
  $s = dh_course_sync($course, $opts);
  t_eq($s['copies'], 1, 'an extra copy is removed'); t_eq($records(), 2, 'back to 2 records');

  $sa = db_query("select sc_student from dh_student_course where sc_course = :c and sc_applicant = :a", array(':c' => $course, ':a' => $a))->fetchField();
  db_update('dh_applicant')->fields(array('a_attended' => 0))->condition('a_id', $a)->execute();
  $s = dh_course_sync($course, $opts);
  t_eq(array($s['removed'], $s['students_removed']), array(1, 1), 'left student: record and student removed'); t_eq($records(), 1, '1 record left');
  t_eq(db_query("select count(*) from dh_student where s_id = :s", array(':s' => $sa))->fetchField(), 0, 'student with no other course is gone');
  t_ok((bool) db_query("select sl_old from dh_student_log where sl_student = :s and sl_action = 'student-removed'", array(':s' => $sa))->fetchField(), 'removed student kept in the log');

  $course2 = t_make_course();
  $old = t_make_student(array('s_l_name' => 'Oldstyle', 's_dob' => '1970-07-07'));
  db_insert('dh_student_course')->fields(array('sc_student' => $old, 'sc_center' => 1, 'sc_course' => $course2))->execute();
  $d = t_make_applicant($course2, array('a_l_name' => 'Oldstyle', 'a_dob' => '1970-07-07'));
  $s = dh_course_sync($course2, $opts + array('mail' => FALSE));
  t_eq(array($s['added'], $s['updated'], $s['emails']), array(0, 1, 0), 'old unlinked record claimed, no email');
  t_eq(db_query("select sc_applicant from dh_student_course where sc_course = :c", array(':c' => $course2))->fetchField(), $d, 'claimed record now linked');
});
t_done();
