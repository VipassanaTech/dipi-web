<?php
require __DIR__ . '/lib.php';
echo "Restore: the undo puts everything back\n";
dh_restore_source_create();                        // outside any transaction: a CREATE TABLE commits
function t_n($sql, $args = array()) { return (int) db_query($sql, $args)->fetchField(); }
function t_free_sid() {
  static $n = 260000;
  do { $n++; } while (t_n('select (select count(*) from dh_student where s_id = :a) + (select count(*) from dh_student_course where sc_student = :b)
    + (select count(*) from dh_student_course_input where sci_student = :c)', array(':a' => $n, ':b' => $n, ':c' => $n)) > 0);
  return $n;
}
function t_old_course($start) { return t_make_course(array('c_finalized' => 1, 'c_start' => $start, 'c_end' => date('Y-m-d', strtotime($start . ' +10 days')))); }
function t_old($sid, $course, $conf, $rs = array()) {
  $centre = t_n('select c_center from dh_course where c_id = :c', array(':c' => $course));
  db_insert('dh_restore_source')->fields(array_merge(array('rs_src' => 'b', 'rs_old_id' => 1, 'rs_course' => $course, 'rs_conf_no' => $conf, 'rs_type' => 'OM',
    'rs_f_name' => 'Zzqold', 'rs_l_name' => 'Zzqperson', 'rs_gender' => 'M'), $rs))->execute();
  return (int) db_insert('dh_student_course')->fields(array('sc_student' => $sid, 'sc_center' => $centre, 'sc_course' => $course, 'sc_conf_no' => $conf,
    'from_migration_script' => 'Yes'))->execute();
}
function t_old_sci($sid) { return (int) db_insert('dh_student_course_input')->fields(array('sci_student' => $sid, 'sci_10d' => 1, 'from_migration_script' => 'Yes'))->execute(); }
function t_make_referral($sid) {
  return (int) db_insert('dh_referral')->fields(array('r_student' => $sid, 'r_center' => 0, 'r_reason' => 'Zzq made-up referral',
    'r_start' => date('Y-m-d'), 'r_end' => date('Y-m-d', strtotime('+1 year')), 'r_created_by' => 0))->execute();
}
// Everything a run can change for these students and numbers (not s_updated / s_updated_by: audit stamps, never reversed).
function t_snap($ids) {
  return array(
    'lines' => db_query('select * from dh_student_course where sc_student in (:s) order by sc_id', array(':s' => $ids))->fetchAll(PDO::FETCH_ASSOC),
    'sci' => db_query('select * from dh_student_course_input where sci_student in (:s) order by sci_id', array(':s' => $ids))->fetchAll(PDO::FETCH_ASSOC),
    'students' => db_query('select s_id, s_f_name, s_l_name, s_email, s_zip, s_education, from_migration_script from dh_student where s_id in (:s) order by s_id', array(':s' => $ids))->fetchAll(PDO::FETCH_ASSOC));
}
// The run as the runner does it: each number, then the tidy of the student its lines now belong to.
function t_run($numbers, $batch) {
  $ctx = array('batch' => $batch, 'uid' => dh_system_uid());
  foreach ($numbers as $n) { $r = dh_restore_one($n, $ctx); if ($r['sid']) dh_restore_tidy_lines($r['sid'], array('sl_batch' => $batch, 'sl_user' => $ctx['uid'])); }
}

// U1: a restore, a merge (box filled, course-counts rows removed), a split pair, a copy, a room and a seat: all go back exactly.
t_in_txn(function () {
  $cur = t_make_student(array('s_f_name' => 'Zzqundo', 's_l_name' => 'Zzqmerge', 's_gender' => 'M', 's_email' => 'zzq.u1@example.com', 's_zip' => ''));
  t_old_sci($cur);
  $r = t_free_sid(); $m = t_free_sid(); $s1 = t_free_sid(); $s2 = t_free_sid();
  $c1 = t_old_course('2008-01-01'); $c2 = t_old_course('2010-01-01');
  t_old($r, $c1, 'ZQ 1', array('rs_f_name' => 'Zzqundo', 'rs_l_name' => 'Zzqrestore', 'rs_acco' => 'Block-1', 'rs_seat' => '5'));
  t_old($r, $c1, 'ZQ 1', array('rs_f_name' => 'Zzqundo', 'rs_l_name' => 'Zzqrestore', 'rs_src' => 'c'));        // one application in both tables
  t_old($r, $c2, 'ZQ 2', array('rs_f_name' => 'Zzqundo', 'rs_l_name' => 'Zzqrestore'));
  t_old_sci($r); t_old_sci($r);
  t_old($m, $c2, 'ZQ 3', array('rs_f_name' => 'Zzqundo', 'rs_l_name' => 'Zzqmerge', 'rs_email' => 'zzq.u1@example.com', 'rs_zip' => '411003', 'rs_acco' => 'Block-3'));
  t_old_sci($m);
  t_old($s1, $c1, 'ZQ 4', array('rs_f_name' => 'Zzqundo', 'rs_l_name' => 'Zzqsplit', 'rs_email' => 'zzq.u2@example.com'));
  t_old($s2, $c2, 'ZQ 5', array('rs_f_name' => 'Zzqundo', 'rs_l_name' => 'Zzqsplit', 'rs_email' => 'zzq.u2@example.com'));
  $ids = array($cur, $r, $m, $s1, $s2);
  $before = t_snap($ids);
  t_run(array($r, $m, $s1, $s2), 'test-rs-undo');
  t_eq(t_n('select count(*) from dh_student where s_id in (:s)', array(':s' => array($r, $s1))), 2, 'U1: the run restored two numbers');
  t_eq(t_n('select s_zip from dh_student where s_id = :s', array(':s' => $cur)), 411003, 'U1: the run filled the empty box of the current student');
  $u = dh_backfill_undo('test-rs-undo', TRUE);
  t_eq(array($u['_failed'], $u['_failed_ids']), array(0, array()), 'U1: nothing failed');
  t_ok(!empty($u['restored']) && !empty($u['course-updated']) && !empty($u['copy-removed']) && !empty($u['sci-removed']) && !empty($u['updated']), 'U1: the log had every kind of row');
  t_eq(t_snap($ids), $before, 'U1: lines, course-counts rows and students are exactly as before');
});

// U2 and U3: a restored student is kept when it got a course since, or a referral points to it.
t_in_txn(function () {
  $n = t_free_sid();
  $c = t_old_course('2008-02-01');
  t_old($n, $c, 'ZQ 6', array('rs_l_name' => 'Zzqguard'));
  t_run(array($n), 'test-rs-undo2');
  db_insert('dh_student_course')->fields(array('sc_student' => $n, 'sc_center' => t_n('select c_center from dh_course where c_id = :c', array(':c' => $c)),
    'sc_course' => t_make_course(array('c_finalized' => 1))))->execute();             // a finalize since
  dh_backfill_undo('test-rs-undo2', TRUE);
  t_eq(t_n('select count(*) from dh_student where s_id = :s', array(':s' => $n)), 1, 'U2: kept after a new course');
  $k = t_free_sid();
  t_old($k, t_old_course('2009-02-01'), 'ZQ 7', array('rs_l_name' => 'Zzqreferred'));
  t_run(array($k), 'test-rs-undo3');
  t_make_referral($k);
  dh_backfill_undo('test-rs-undo3', TRUE);
  t_eq(t_n('select count(*) from dh_student where s_id = :s', array(':s' => $k)), 1, 'U3: kept when a referral points to it');
});

// U4: a merged value longer than its box is cut to the box before it is written and logged, so the undo can put the box back.
t_in_txn(function () {
  $cur = t_make_student(array('s_f_name' => 'Zzqundo', 's_l_name' => 'Zzqcut', 's_gender' => 'M', 's_email' => 'zzq.u4@example.com', 's_phone_home' => ''));
  $m = t_free_sid();
  $c = t_old_course('2010-04-01');
  $long = '01234567890123456789ABCDEF';                                       // 26 characters; the box holds 20
  t_old($m, $c, 'ZQ 8', array('rs_f_name' => 'Zzqundo', 'rs_l_name' => 'Zzqcut', 'rs_email' => 'zzq.u4@example.com', 'rs_phone_home' => $long));
  t_run(array($m), 'test-rs-undo4');
  t_eq(db_query('select s_phone_home from dh_student where s_id = :s', array(':s' => $cur))->fetchField(), substr($long, 0, 20), 'U4: the long value was cut to the box');
  t_eq(db_query("select sl_new from dh_student_log where sl_batch = 'test-rs-undo4' and sl_student = :s and sl_field = 's_phone_home'", array(':s' => $cur))->fetchField(),
    substr($long, 0, 20), 'U4: the log holds the value that was stored');
  $u = dh_backfill_undo('test-rs-undo4', TRUE);
  t_eq(array($u['_failed'], $u['_skipped']), array(0, 0), 'U4: nothing failed, nothing skipped');
  t_eq(db_query('select s_phone_home from dh_student where s_id = :s', array(':s' => $cur))->fetchField(), '', 'U4: the box is empty again after the undo');
});
t_done();
