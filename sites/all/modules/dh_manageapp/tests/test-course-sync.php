<?php
require __DIR__ . '/lib.php';
echo "Sync step\n";
// A mail function that records each call and reports that the letter could not go out.
function t_bounce_mail($type, $a_id, $event) { $GLOBALS['t_bounce'][] = (int) $a_id; return array('error' => 'No Email or Mobile'); }
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

echo "Safety rule\n";
t_in_txn(function () {
  $opts = array('mail' => FALSE, 'files' => 'none');
  $course = t_make_course();
  $x = t_make_applicant($course, array('a_l_name' => 'Failing'));
  $y = t_make_applicant($course, array('a_l_name' => 'Leaving'));
  $s = dh_course_sync($course, $opts);
  t_eq(array($s['added'], $s['errors']), array(2, 0), 'safety: both applicants synced first');
  $sx = db_query("select sc_student from dh_student_course where sc_course = :c and sc_applicant = :a", array(':c' => $course, ':a' => $x))->fetchField();
  db_delete('dh_student')->condition('s_id', $sx)->execute();   // x's course record now points to a student that does not exist
  db_update('dh_applicant')->fields(array('a_attended' => 0))->condition('a_id', $y)->execute();   // y left, so y's record would normally go
  $s = dh_course_sync($course, $opts);
  t_ok($s['errors'] >= 1, 'safety: the applicant with a missing student is counted as failed');
  t_eq(array($s['removed'], $s['copies'], $s['students_removed']), array(0, 0, 0), 'safety: nothing removed in a run with a failure');
  t_eq(db_query("select count(*) from dh_student_course where sc_course = :c and sc_applicant = :a", array(':c' => $course, ':a' => $y))->fetchField(), 1, 'safety: the record of the applicant who left is kept');
});

echo "Bounced letter\n";
t_in_txn(function () {
  $GLOBALS['t_bounce'] = array();
  $opts = array('mail_fn' => 't_bounce_mail', 'files' => 'none');
  $course = t_make_course();
  $a = t_make_applicant($course, array('a_l_name' => 'Bounced'));
  $s = dh_course_sync($course, $opts);
  t_eq($s['emails'], 0, 'bounce: a letter that could not go out is not counted');
  t_ok((bool) db_query("select a_attended_mail from dh_applicant where a_id = :a", array(':a' => $a))->fetchField(), 'bounce: the email date is stored anyway');
  t_eq($GLOBALS['t_bounce'], array($a), 'bounce: the mail function was called once');
  $s = dh_course_sync($course, $opts);
  t_eq($GLOBALS['t_bounce'], array($a), 'bounce: a second run does not call the mail function again');
  t_eq($s['emails'], 0, 'bounce: still no email counted');
});

echo "Student course input\n";
t_in_txn(function () {
  $opts = array('mail' => FALSE, 'files' => 'none');
  $course = t_make_course();
  $a = t_make_applicant($course, array('a_l_name' => 'Hasinput'));
  db_insert('dh_applicant_course')->fields(array('ac_applicant' => $a, 'ac_created_by' => 0, 'ac_updated_by' => 0,
    'ac_first_teacher_str' => 'Teacher text', 'ac_first_location_str' => 'Location text'))->execute();
  for ($i = 0; $i < 3; $i++)
    dh_course_sync($course, $opts);
  $sid = db_query("select sc_student from dh_student_course where sc_course = :c and sc_applicant = :a", array(':c' => $course, ':a' => $a))->fetchField();
  t_eq(db_query("select count(*) from dh_student_course_input where sci_student = :s", array(':s' => $sid))->fetchField(), 1, 'sci: exactly one course input row after three syncs');
  $sci = db_query("select sci_first_location_str, sci_first_teacher_str from dh_student_course_input where sci_student = :s", array(':s' => $sid))->fetchAssoc();
  t_eq($sci['sci_first_location_str'], 'Location text', 'sci: first course location comes from the location field');
  t_eq($sci['sci_first_teacher_str'], 'Teacher text', 'sci: first course teacher comes from the teacher field');
});

echo "Summary text\n";
$sum = array('students' => 5, 'added' => 0, 'updated' => 0, 'removed' => 0, 'copies' => 0, 'students_removed' => 0, 'emails' => 1, 'errors' => 0);
t_eq(dh_course_sync_summary_text($sum), '5 students: 0 added, 1 email sent', 'summary: plain run');
t_eq(dh_course_sync_summary_text(array('copies' => 1) + $sum), '5 students: 0 added, 1 copy removed, 1 email sent', 'summary: one copy removed');
t_eq(dh_course_sync_summary_text(array('copies' => 2) + $sum), '5 students: 0 added, 2 copies removed, 1 email sent', 'summary: two copies removed');
t_eq(dh_course_sync_summary_text(array('removed' => 3, 'students_removed' => 1) + $sum), '5 students: 0 added, 3 removed, 1 student record removed, 1 email sent', 'summary: one student record removed');
t_eq(dh_course_sync_summary_text(array('removed' => 3, 'students_removed' => 2) + $sum), '5 students: 0 added, 3 removed, 2 student records removed, 1 email sent', 'summary: two student records removed');

echo "Newsletter once per student\n";
t_in_txn(function () {
  $opts = array('mail' => FALSE, 'files' => 'none');
  $course = t_make_course(array('c_center' => 1));
  // The same person applied twice: two attended applications, one student, two course records.
  $a1 = t_make_applicant($course, array('a_l_name' => 'Twice', 'a_dob' => '1971-02-02', 'a_country' => 'IN'));
  $a2 = t_make_applicant($course, array('a_l_name' => 'Twice', 'a_dob' => '1971-02-02', 'a_country' => 'IN'));
  dh_course_sync($course, $opts);
  $students = db_query("select distinct sc_student from dh_student_course where sc_course = :c", array(':c' => $course))->fetchCol();
  t_eq(count($students), 1, 'newsletter: both applications joined one student');
  t_eq(db_query("select count(*) from dh_student_course where sc_course = :c", array(':c' => $course))->fetchField(), 2, 'newsletter: two course records for that student');
  dh_patrika_add_course_students($course);
  t_eq(db_query("select count(*) from dh_patrika where p_student = :s", array(':s' => $students[0]))->fetchField(), 1, 'newsletter: one subscription even with two course records');
  dh_patrika_add_course_students($course);
  t_eq(db_query("select count(*) from dh_patrika where p_student = :s", array(':s' => $students[0]))->fetchField(), 1, 'newsletter: a second run adds none');
});
t_done();
