<?php
require __DIR__ . '/lib.php';
echo "Restore: restore or merge one old student number\n";
dh_restore_source_create();                        // outside any transaction: a CREATE TABLE commits
function t_n($sql, $args = array()) { return (int) db_query($sql, $args)->fetchField(); }
function t_row($table, $key, $id) { return db_query("select * from $table where $key = :i", array(':i' => $id))->fetchAssoc(); }
// A student number nobody uses locally (no student, line or course-counts row).
function t_free_sid() {
  static $n = 250000;
  do { $n++; } while (t_n('select (select count(*) from dh_student where s_id = :a) + (select count(*) from dh_student_course where sc_student = :b)
    + (select count(*) from dh_student_course_input where sci_student = :c)', array(':a' => $n, ':b' => $n, ':c' => $n)) > 0);
  return $n;
}
function t_old_course($start) { return t_make_course(array('c_finalized' => 1, 'c_start' => $start, 'c_end' => date('Y-m-d', strtotime($start . ' +10 days')))); }
// One old line of $sid in $course and its old row ($rs = the person's fields).
function t_old($sid, $course, $conf, $rs = array()) {
  $centre = t_n('select c_center from dh_course where c_id = :c', array(':c' => $course));
  db_insert('dh_restore_source')->fields(array_merge(array('rs_src' => 'b', 'rs_old_id' => 1, 'rs_course' => $course, 'rs_conf_no' => $conf, 'rs_type' => 'OM',
    'rs_f_name' => 'Zzqold', 'rs_l_name' => 'Zzqperson', 'rs_gender' => 'M'), $rs))->execute();
  return (int) db_insert('dh_student_course')->fields(array('sc_student' => $sid, 'sc_center' => $centre, 'sc_course' => $course, 'sc_conf_no' => $conf,
    'from_migration_script' => 'Yes'))->execute();
}
function t_old_sci($sid) { return (int) db_insert('dh_student_course_input')->fields(array('sci_student' => $sid, 'sci_10d' => 1, 'from_migration_script' => 'Yes'))->execute(); }
function t_logs($action, $sid) { return t_n('select count(*) from dh_student_log where sl_batch = :b and sl_action = :a and sl_student = :s', array(':b' => 'test-rs-one', ':a' => $action, ':s' => $sid)); }
function t_sci($sids) { return db_query('select sci_id from dh_student_course_input where sci_student in (:s) order by sci_id', array(':s' => $sids))->fetchCol(); }
$CTX = array('batch' => 'test-rs-one', 'uid' => dh_system_uid());

// R1: restored under its own number, details from the newest row.
t_in_txn(function () use ($CTX) {
  $n = t_free_sid();
  $l1 = t_old($n, t_old_course('2008-01-10'), 'ZQ 11', array('rs_f_name' => 'Zzqrone', 'rs_l_name' => 'Zzqrestore', 'rs_type' => 'NM', 'rs_email' => 'zzq.r1old@example.com'));
  $l2 = t_old($n, t_old_course('2012-03-05'), 'ZQ 12', array('rs_f_name' => 'Zzqrone', 'rs_l_name' => 'Zzqrestore', 'rs_email' => 'zzq.r1new@example.com', 'rs_phone_mobile' => '[91]9876500011'));
  $r = dh_restore_one($n, $CTX);
  t_eq(array($r['result'], $r['sid']), array('restored', $n), 'R1: restored under its own number');
  $s = t_row('dh_student', 's_id', $n);
  t_eq(array($s['s_f_name'], $s['s_email'], $s['s_phone_mobile'], $s['from_migration_script'], $s['s_first'], $s['s_created_by']),
    array('Zzqrone', 'zzq.r1new@example.com', '9876500011', 'Yes', 1, $CTX['uid']), 'R1: newest details, new at the first course, made by System');
  t_eq(array(t_row('dh_student_course', 'sc_id', $l1)['sc_student'], t_row('dh_student_course', 'sc_id', $l2)['sc_student']), array($n, $n), 'R1: the lines still point to it');
  t_eq(t_logs('restored', $n), 1, 'R1: one restored log row');
});

// R2: merged into the current student: line moved, filled box kept, empty box filled, all logged.
t_in_txn(function () use ($CTX) {
  $cur = t_make_student(array('s_f_name' => 'Zzqmerge', 's_l_name' => 'Zzqone', 's_gender' => 'M', 's_email' => 'zzq.m1@example.com', 's_education' => 'Current'));
  $n = t_free_sid();
  $line = t_old($n, t_old_course('2009-02-01'), 'ZQ 21', array('rs_f_name' => 'Zzqmerge', 'rs_l_name' => 'Zzqone', 'rs_email' => 'zzq.m1@example.com', 'rs_education' => 'Old', 'rs_zip' => '411002'));
  $r = dh_restore_one($n, $CTX);
  t_eq(array($r['result'], $r['sid'], $r['how']), array('merged', $cur, 'name+email'), 'R2: merged by name + email');
  t_eq(t_row('dh_student_course', 'sc_id', $line)['sc_student'], $cur, 'R2: the old line moved');
  t_eq(t_n('select count(*) from dh_student where s_id = :s', array(':s' => $n)), 0, 'R2: the old number is not recreated');
  $s = t_row('dh_student', 's_id', $cur);
  t_eq(array($s['s_education'], $s['s_zip']), array('Current', '411002'), 'R2: a filled box is kept, an empty box is filled');
  t_eq(array(t_logs('course-updated', $cur), t_logs('updated', $cur)), array(1, 1), 'R2: the move and the filled box are logged');
  $old = json_decode(db_query("select sl_old from dh_student_log where sl_batch = 'test-rs-one' and sl_action = 'course-updated'")->fetchField(), TRUE);
  t_eq($old['sc_student'], $n, 'R2: the log keeps the old number for the undo');
});

// R3: two numbers the import split (same name and email, no current student) come together on the lower one.
t_in_txn(function () use ($CTX) {
  $n1 = t_free_sid(); $n2 = t_free_sid();
  t_old($n1, t_old_course('2008-05-01'), 'ZQ 31', array('rs_f_name' => 'Zzqsplit', 'rs_l_name' => 'Zzqthree', 'rs_email' => 'zzq.s3@example.com'));
  $l2 = t_old($n2, t_old_course('2011-05-01'), 'ZQ 32', array('rs_f_name' => 'Zzqsplit', 'rs_l_name' => 'Zzqthree', 'rs_email' => 'zzq.s3@example.com'));
  t_eq(dh_restore_one($n1, $CTX)['result'], 'restored', 'R3: the lower number is restored');
  $r = dh_restore_one($n2, $CTX);
  t_eq(array($r['result'], $r['sid'], t_row('dh_student_course', 'sc_id', $l2)['sc_student']), array('merged', $n1, $n1), 'R3: the higher number merges into it');
});

// R4 and R5: rows of two people, or a line with no old row, are unclear and change nothing.
t_in_txn(function () use ($CTX) {
  $n = t_free_sid();
  $before = t_n('select count(*) from dh_student');
  $l1 = t_old($n, t_old_course('2008-06-01'), 'ZQ 41', array('rs_l_name' => 'Zzqfour'));
  t_old($n, t_old_course('2009-06-01'), 'ZQ 42', array('rs_l_name' => 'Zzqother'));
  t_eq(dh_restore_one($n, $CTX), array('result' => 'unclear', 'sid' => 0, 'how' => ''), 'R4: rows of two people are unclear');
  t_eq(array(t_n('select count(*) from dh_student'), t_row('dh_student_course', 'sc_id', $l1)['sc_student'], t_n("select count(*) from dh_student_log where sl_batch = 'test-rs-one'")),
    array($before, $n, 0), 'R4: nothing changed, nothing logged');
  $m = t_free_sid();
  $c = t_old_course('2008-07-01');
  t_old($m, $c, 'ZQ 51', array('rs_l_name' => 'Zzqfive'));
  db_insert('dh_student_course')->fields(array('sc_student' => $m, 'sc_center' => t_n('select c_center from dh_course where c_id = :c', array(':c' => $c)),
    'sc_course' => t_old_course('2010-07-01'), 'sc_conf_no' => 'ZQ 59', 'from_migration_script' => 'Yes'))->execute();
  t_eq(dh_restore_one($m, $CTX)['result'], 'unclear', 'R5: a line with no old row is unclear');
});

// R6: a number that exists is done (its own sid); a number with no old line is done (sid 0).
t_in_txn(function () use ($CTX) {
  $s = t_make_student(array('s_l_name' => 'Zzqsix'));
  t_eq(dh_restore_one($s, $CTX), array('result' => 'done', 'sid' => $s, 'how' => ''), 'R6: an existing number is done');
  t_eq(dh_restore_one(t_free_sid(), $CTX), array('result' => 'done', 'sid' => 0, 'how' => ''), 'R6: a number with no old line is done');
});

// R7: one course-counts row each.
t_in_txn(function () use ($CTX) {
  $n = t_free_sid();
  t_old($n, t_old_course('2008-08-01'), 'ZQ 71', array('rs_l_name' => 'Zzqseven'));
  t_old_sci($n); t_old_sci($n); $newest = t_old_sci($n);
  dh_restore_one($n, $CTX);
  t_eq(array(t_sci(array($n)), t_logs('sci-removed', $n)), array(array($newest), 2), 'R7: a restored student keeps its newest row; two removed and logged');
  $cur = t_make_student(array('s_f_name' => 'Zzqmove', 's_l_name' => 'Zzqseven', 's_gender' => 'M', 's_email' => 'zzq.m7@example.com'));
  $m = t_free_sid();
  t_old($m, t_old_course('2009-08-01'), 'ZQ 72', array('rs_f_name' => 'Zzqmove', 'rs_l_name' => 'Zzqseven', 'rs_email' => 'zzq.m7@example.com'));
  t_old_sci($m); $moved = t_old_sci($m);
  dh_restore_one($m, $CTX);
  t_eq(array(t_sci(array($cur)), t_sci(array($m))), array(array($moved), array()), 'R7: a merged student with no row gets the newest old row');
  $cur2 = t_make_student(array('s_f_name' => 'Zzqkeep', 's_l_name' => 'Zzqseven', 's_gender' => 'M', 's_email' => 'zzq.k7@example.com'));
  $own = t_old_sci($cur2);
  $k = t_free_sid();
  t_old($k, t_old_course('2010-08-01'), 'ZQ 73', array('rs_f_name' => 'Zzqkeep', 'rs_l_name' => 'Zzqseven', 'rs_email' => 'zzq.k7@example.com'));
  t_old_sci($k);
  dh_restore_one($k, $CTX);
  t_eq(t_sci(array($cur2, $k)), array($own), 'R7: a merged student with a row keeps only its own');
});

// R8: an older row's email still finds the current student when the newest row's does not.
t_in_txn(function () use ($CTX) {
  $cur = t_make_student(array('s_f_name' => 'Zzqeight', 's_l_name' => 'Zzqolder', 's_gender' => 'M', 's_email' => 'zzq.e8old@example.com'));
  $n = t_free_sid();
  t_old($n, t_old_course('2008-09-01'), 'ZQ 81', array('rs_f_name' => 'Zzqeight', 'rs_l_name' => 'Zzqolder', 'rs_email' => 'zzq.e8old@example.com'));
  t_old($n, t_old_course('2013-09-01'), 'ZQ 82', array('rs_f_name' => 'Zzqeight', 'rs_l_name' => 'Zzqolder', 'rs_email' => 'zzq.e8new@example.com'));
  t_eq(dh_restore_one($n, $CTX)['sid'], $cur, 'R8: matched through an older row');
});

// R9: a 1 January birth date (mostly a year-only date in the old data) is never a match by itself, with a current student or
// with another old number; the restored student still keeps the date.
t_in_txn(function () use ($CTX) {
  $cur = t_make_student(array('s_f_name' => 'Zzqnine', 's_l_name' => 'Zzqjanone', 's_gender' => 'M', 's_dob' => '1975-01-01'));
  $n1 = t_free_sid(); $n2 = t_free_sid();
  t_old($n1, t_old_course('2010-04-01'), 'ZQ 91', array('rs_f_name' => 'Zzqnine', 'rs_l_name' => 'Zzqjanone', 'rs_dob' => '1975-01-01'));
  t_old($n2, t_old_course('2011-04-01'), 'ZQ 92', array('rs_f_name' => 'Zzqnine', 'rs_l_name' => 'Zzqjanone', 'rs_dob' => '1975-01-01'));
  $r1 = dh_restore_one($n1, $CTX);
  $r2 = dh_restore_one($n2, $CTX);
  t_eq(array($r1['result'], $r1['sid'], $r2['result'], $r2['sid']), array('restored', $n1, 'restored', $n2), 'R9: name + 1 January joins nobody');
  t_eq(t_row('dh_student', 's_id', $n1)['s_dob'], '1975-01-01', 'R9: the restored student keeps the date');
});

// R10: any other birth date still matches by name + date of birth.
t_in_txn(function () use ($CTX) {
  $cur = t_make_student(array('s_f_name' => 'Zzqten', 's_l_name' => 'Zzqrealdob', 's_gender' => 'F', 's_dob' => '1975-06-14'));
  $n = t_free_sid();
  t_old($n, t_old_course('2010-05-01'), 'ZQ 101', array('rs_f_name' => 'Zzqten', 'rs_l_name' => 'Zzqrealdob', 'rs_gender' => 'F', 'rs_dob' => '1975-06-14'));
  $r = dh_restore_one($n, $CTX);
  t_eq(array($r['result'], $r['sid'], $r['how']), array('merged', $cur, 'name+dob'), 'R10: name + a full date still merges');
});

// R11: with a 1 January date, the same email or mobile still merges.
t_in_txn(function () use ($CTX) {
  $cur = t_make_student(array('s_f_name' => 'Zzqeleven', 's_l_name' => 'Zzqjanmail', 's_gender' => 'M', 's_dob' => '1980-01-01', 's_phone_mobile' => '9876500111'));
  $n = t_free_sid();
  t_old($n, t_old_course('2010-06-01'), 'ZQ 111', array('rs_f_name' => 'Zzqeleven', 'rs_l_name' => 'Zzqjanmail', 'rs_dob' => '1980-01-01', 'rs_phone_mobile' => '[91]9876500111'));
  $r = dh_restore_one($n, $CTX);
  t_eq(array($r['result'], $r['sid'], $r['how']), array('merged', $cur, 'name+phone_mobile'), 'R11: 1 January + the same mobile merges');
});

// R12: when the newest row has 1 January, an older row's full date is still tried.
t_in_txn(function () use ($CTX) {
  $cur = t_make_student(array('s_f_name' => 'Zzqtwelve', 's_l_name' => 'Zzqolddob', 's_gender' => 'M', 's_dob' => '1968-09-23'));
  $n = t_free_sid();
  t_old($n, t_old_course('2007-07-01'), 'ZQ 121', array('rs_f_name' => 'Zzqtwelve', 'rs_l_name' => 'Zzqolddob', 'rs_dob' => '1968-09-23'));
  t_old($n, t_old_course('2014-07-01'), 'ZQ 122', array('rs_f_name' => 'Zzqtwelve', 'rs_l_name' => 'Zzqolddob', 'rs_dob' => '1968-01-01'));
  $r = dh_restore_one($n, $CTX);
  t_eq(array($r['result'], $r['sid'], $r['how']), array('merged', $cur, 'name+dob'), 'R12: an older row with a full date still matches');
});
t_done();
