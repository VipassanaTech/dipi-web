<?php
require __DIR__ . '/lib.php';
echo "An applicant with no match claims this course's own unlinked record (no second student for one person)\n";
function t_n($sql, $args = array()) { return (int) db_query($sql, $args)->fetchField(); }
function t_row($table, $key, $id) { return db_query("select * from $table where $key = :i", array(':i' => $id))->fetchAssoc(); }
function t_sc_of($course, $a) { return t_n('select sc_student from dh_student_course where sc_course = :c and sc_applicant = :a', array(':c' => $course, ':a' => $a)); }
// An old course record of this course, with no applicant noted (as the old finalize wrote them).
function t_old_record($course, $sid) {
  $centre = (int) db_query('select c_center from dh_course where c_id = :c', array(':c' => $course))->fetchField();
  return (int) db_insert('dh_student_course')->fields(array('sc_student' => $sid, 'sc_center' => $centre, 'sc_course' => $course))->execute();
}
$opts = array('mail' => FALSE, 'files' => 'none');

// Same first and last name (any case, spaces around): the record and its student are taken, as a weak match.
t_in_txn(function () use ($opts) {
  $course = t_make_course(array('c_finalized' => 1));
  $s = t_make_student(array('s_f_name' => 'zzqtest ', 's_l_name' => ' CLAIMNAME', 's_dob' => '1950-05-05', 's_gender' => 'M'));
  $sc = t_old_record($course, $s);
  $a = t_make_applicant($course, array('a_l_name' => 'Claimname', 'a_dob' => '1981-01-01', 'a_gender' => 'F'));
  $r = dh_course_sync($course, $opts);
  t_eq(array($r['added'], $r['updated'], $r['removed'], $r['errors']), array(0, 1, 0, 0), 'name: the old record is claimed (nothing added, nothing removed)');
  t_eq(t_row('dh_student_course', 'sc_id', $sc)['sc_applicant'], $a, 'name: the old record now names the applicant');
  t_eq(t_n("select count(*) from dh_student_log where sl_course = :c and sl_action = 'created'", array(':c' => $course)), 0, 'name: no new student');
  $st = t_row('dh_student', 's_id', $s);
  t_eq(array($st['s_f_name'], $st['s_l_name'], $st['s_dob'], $st['s_gender']), array('zzqtest ', ' CLAIMNAME', '1950-05-05', 'M'), 'name: weak match, so name, date of birth and gender are not changed');
});

// Same first name and the same known date of birth (the last name differs): claimed too, the last name is not changed.
t_in_txn(function () use ($opts) {
  $course = t_make_course(array('c_finalized' => 1));
  $s = t_make_student(array('s_l_name' => 'Oldlast', 's_dob' => '1952-02-02'));
  $sc = t_old_record($course, $s);
  $a = t_make_applicant($course, array('a_l_name' => 'Newlast', 'a_dob' => '1952-02-02'));
  $r = dh_course_sync($course, $opts);
  t_eq(array($r['added'], $r['updated']), array(0, 1), 'first name + date of birth: claimed');
  t_eq(t_sc_of($course, $a), $s, 'first name + date of birth: the applicant is on the old student');
  t_eq(t_row('dh_student', 's_id', $s)['s_l_name'], 'Oldlast', 'first name + date of birth: the last name is not changed (weak match)');
  // Same first name but a different (or unknown) date of birth and another last name: not a candidate.
  $c2 = t_make_course(array('c_finalized' => 1));
  $s2 = t_make_student(array('s_l_name' => 'Otherlast', 's_dob' => '1953-03-03'));
  t_old_record($c2, $s2);
  $b = t_make_applicant($c2, array('a_l_name' => 'Notsame', 'a_dob' => '1954-04-04'));
  $r = dh_course_sync($c2, $opts);
  t_eq(array($r['added'], $r['removed']), array(1, 1), 'first name only: no claim (new student, old record removed)');
  t_ok(t_sc_of($c2, $b) != $s2, 'first name only: the applicant is not put on that student');
});

// Two candidates: unclear who is who, so a new student is made as before.
t_in_txn(function () use ($opts) {
  $course = t_make_course(array('c_finalized' => 1));
  $s1 = t_make_student(array('s_l_name' => 'Twin')); $s2 = t_make_student(array('s_l_name' => 'Twin'));
  t_old_record($course, $s1); t_old_record($course, $s2);
  $a = t_make_applicant($course, array('a_l_name' => 'Twin', 'a_dob' => '1983-03-03'));
  $r = dh_course_sync($course, $opts);
  t_eq(array($r['added'], $r['removed']), array(1, 2), 'two candidates: a new student, both old records removed');
  t_ok(!in_array(t_sc_of($course, $a), array($s1, $s2)), 'two candidates: neither old student is used');
  // Two old records of the SAME student are one candidate: the first is claimed, the other is a copy.
  $c2 = t_make_course(array('c_finalized' => 1));
  $s3 = t_make_student(array('s_l_name' => 'Doubled'));
  $first = t_old_record($c2, $s3); t_old_record($c2, $s3);
  $b = t_make_applicant($c2, array('a_l_name' => 'Doubled', 'a_dob' => '1986-06-06'));
  $r = dh_course_sync($c2, $opts);
  t_eq(array($r['added'], $r['updated'], $r['copies']), array(0, 1, 1), 'one student with two old records: claimed once, the other record is a copy');
  t_eq(t_row('dh_student_course', 'sc_id', $first)['sc_applicant'], $b, 'one student with two old records: the first record is claimed');
});

// Order does not matter: when ANOTHER attended applicant of the course could also be the record's person (same first and
// last name, or same first name and known date of birth as its student), nobody claims it by name.
t_in_txn(function () use ($opts) {
  $course = t_make_course(array('c_finalized' => 1));
  $s = t_make_student(array('s_l_name' => 'Same'));                                    // no date of birth
  $sc = t_old_record($course, $s);
  $a1 = t_make_applicant($course, array('a_l_name' => 'Same', 'a_dob' => '1984-04-04'));
  $a2 = t_make_applicant($course, array('a_l_name' => 'Same', 'a_dob' => '1985-05-05'));
  $r = dh_course_sync($course, $opts);
  t_eq(array($r['added'], $r['updated'], $r['removed']), array(2, 0, 1), 'two applicants could claim it: neither does (two new students, the old record removed)');
  t_ok(!in_array($s, array(t_sc_of($course, $a1), t_sc_of($course, $a2))) && t_sc_of($course, $a1) != t_sc_of($course, $a2), 'two applicants: two new students, not the old one');
  // A student another applicant of this course already has is not a candidate either (its spare record is a copy).
  $c2 = t_make_course(array('c_finalized' => 1));
  $b1 = t_make_applicant($c2, array('a_l_name' => 'Kept', 'a_dob' => '1987-07-07'));
  dh_course_sync($c2, $opts);
  $sb = t_sc_of($c2, $b1);
  t_old_record($c2, $sb);
  $b2 = t_make_applicant($c2, array('a_l_name' => 'Kept', 'a_dob' => '1988-08-08'));
  $r = dh_course_sync($c2, $opts);
  t_eq(array($r['added'], $r['copies']), array(1, 1), 'a student already kept in this run is not claimed: new student, spare record removed as a copy');
  t_ok(t_sc_of($c2, $b2) != $sb, 'the second applicant is not put on the first one\'s student');
});

// B1: first run of an old course. A (lower a_id, no ID, no match) shares the name of B, whose old record is on S; B matches S
// strongly by ID. A must not take S's record (that would change S's contact details and leave B a copy or a new student).
t_in_txn(function () use ($opts) {
  $course = t_make_course(array('c_finalized' => 1));
  $s = t_make_student(array('s_l_name' => 'Pairname', 's_dob' => '1955-05-05', 's_passport' => 'ZQ8880001', 's_address' => 'Zzq old address', 's_occupation' => 'Zzq old work'));
  $r1 = t_old_record($course, $s);
  $a = t_make_applicant($course, array('a_l_name' => 'Pairname', 'a_dob' => '1956-06-06', 'a_address' => 'Zzq address of A', 'a_occupation' => 'Zzq work of A'));
  $b = t_make_applicant($course, array('a_l_name' => 'Pairname', 'a_dob' => '1955-05-05', 'a_passport' => 'ZQ8880001'));
  t_ok($a < $b, 'B1: A has the lower a_id (it is synced first)');
  $r = dh_course_sync($course, $opts);
  t_eq(array($r['added'], $r['updated'], $r['removed'], $r['errors']), array(1, 1, 0, 0), 'B1: A gets a new student, B keeps the old record');
  t_ok(t_sc_of($course, $a) > 0 && t_sc_of($course, $a) != $s, 'B1: A is not put on S');
  t_eq(array(t_sc_of($course, $b), t_row('dh_student_course', 'sc_id', $r1)['sc_applicant']), array($s, $b), "B1: B has S and S's old record r1");
  $st = t_row('dh_student', 's_id', $s);
  t_eq(array($st['s_address'], $st['s_occupation']), array('Zzq old address', 'Zzq old work'), "B1: S's contact details are not changed by A");
  t_eq(t_n('select count(*) from dh_student_course where sc_course = :c and sc_student = :s', array(':c' => $course, ':s' => $s)), 1, 'B1: S has one record in the course (no merge of two people)');
});

// removed_kept: course records removed whose student keeps other courses.
t_in_txn(function () use ($opts) {
  $course = t_make_course(array('c_finalized' => 1)); $other = t_make_course(array('c_finalized' => 1));
  $keeps = t_make_student(array('s_l_name' => 'Hasother')); t_old_record($course, $keeps); t_old_record($other, $keeps);
  $only = t_make_student(array('s_l_name' => 'Onlyhere')); t_old_record($course, $only);
  t_make_applicant($course, array('a_l_name' => 'Unrelated', 'a_dob' => '1989-09-09'));
  $r = dh_course_sync($course, $opts);
  t_eq(array($r['removed'], $r['removed_kept'], $r['students_removed']), array(2, 1, 1), 'removed_kept: 2 removed, 1 student keeps another course, 1 student removed');
  t_eq(t_n('select count(*) from dh_student where s_id = :s', array(':s' => $keeps)), 1, 'removed_kept: that student is still there');
});
t_done();
