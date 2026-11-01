<?php
require __DIR__ . '/lib.php';
echo "Restore guards: the sync keeps old 2020 lines; the back-fill skips courses with no applications\n";
function t_n($sql, $args = array()) { return (int) db_query($sql, $args)->fetchField(); }
// A made-up old line from the 2020 migration (no applicant noted).
function t_old_line($sid, $course, $over = array()) {
  $centre = t_n('select c_center from dh_course where c_id = :c', array(':c' => $course));
  return (int) db_insert('dh_student_course')->fields(array_merge(array('sc_student' => $sid, 'sc_center' => $centre, 'sc_course' => $course,
    'sc_conf_no' => 'ZQ 1', 'from_migration_script' => 'Yes'), $over))->execute();
}

// G1: the old line of a student who is not an attended applicant stays; a normal line of such a student still goes.
t_in_txn(function () {
  $c = t_make_course(array('c_finalized' => 1));
  t_make_applicant($c);
  $old = t_old_line(t_make_student(array('s_l_name' => 'Oldline')), $c);
  $centre = t_n('select c_center from dh_course where c_id = :c', array(':c' => $c));
  db_insert('dh_student_course')->fields(array('sc_student' => t_make_student(array('s_l_name' => 'Normalline')), 'sc_center' => $centre, 'sc_course' => $c))->execute();
  $s = dh_course_sync($c, array('mail' => FALSE, 'files' => 'none'));
  t_eq(t_n('select count(*) from dh_student_course where sc_id = :i', array(':i' => $old)), 1, 'G1: the old line is still there');
  t_eq(array($s['kept_old'], $s['removed']), array(1, 1), 'G1: one old line kept, the normal line removed');
});

// G2: an old line of a student who keeps another line of the same course is a copy and goes.
t_in_txn(function () {
  $c = t_make_course(array('c_finalized' => 1));
  $a = t_make_applicant($c);
  dh_course_sync($c, array('mail' => FALSE, 'files' => 'none'));
  $sid = t_n('select sc_student from dh_student_course where sc_course = :c and sc_applicant = :a', array(':c' => $c, ':a' => $a));
  $old = t_old_line($sid, $c);
  $s = dh_course_sync($c, array('mail' => FALSE, 'files' => 'none'));
  t_eq(t_n('select count(*) from dh_student_course where sc_id = :i', array(':i' => $old)), 0, 'G2: the copy is removed');
  t_eq(array($s['copies'], $s['kept_old']), array(1, 0), 'G2: counted as a copy, not as kept');
});

// G3: a course with no applications keeps its old line and its student; the summary says so.
t_in_txn(function () {
  $c = t_make_course(array('c_finalized' => 1));
  $sid = t_make_student(array('s_l_name' => 'Onlyold'));
  $old = t_old_line($sid, $c);
  $s = dh_course_sync($c, array('mail' => FALSE, 'files' => 'none'));
  t_eq(t_n('select count(*) from dh_student_course where sc_id = :i', array(':i' => $old)), 1, 'G3: the old line stays');
  t_eq(t_n('select count(*) from dh_student where s_id = :s', array(':s' => $sid)), 1, 'G3: its student stays');
  t_ok(strpos(dh_course_sync_summary_text($s), '1 old record kept') !== FALSE, 'G3: the summary names the kept old record');
});

// G4: back-fill batch 2 skips a course with no applications, and still lists one with an application.
t_in_txn(function () {
  $none = t_make_course(array('c_finalized' => 1));
  $sid = t_make_student(array('s_l_name' => 'Twiceold'));
  t_old_line($sid, $none, array('sc_conf_no' => 'ZQ 2'));
  t_old_line($sid, $none, array('sc_conf_no' => 'ZQ 2'));
  $with = t_make_course(array('c_finalized' => 1));
  t_make_applicant($with);
  $sid2 = t_make_student(array('s_l_name' => 'Twicenew'));
  $centre = t_n('select c_center from dh_course where c_id = :c', array(':c' => $with));
  foreach (array(1, 2) as $i) db_insert('dh_student_course')->fields(array('sc_student' => $sid2, 'sc_center' => $centre, 'sc_course' => $with))->execute();
  $b2 = dh_backfill_courses(2);
  t_ok(!in_array($none, $b2), 'G4: batch 2 skips the course with no applications');
  t_ok(in_array($with, $b2), 'G4: batch 2 still lists the course with an application');
});

// G5: the restore is a one-time job: the site does not load its code on page requests (only its tool and tests do).
t_ok(!function_exists('dh_restore_one') && !defined('DH_RESTORE_FIRST'), 'G5: the site does not load the restore code');
t_done();
