<?php
require __DIR__ . '/lib.php';
echo "Back-fill undo and the batch log of in-place changes\n";
// Reads or sets the session SQL mode (returns the mode it found). Used to make legacy zero dates and to see them come back.
function t_sql_mode($set = NULL) {
  $was = db_query('select @@session.sql_mode')->fetchField();
  if ($set !== NULL) db_query('SET SESSION sql_mode = :m', array(':m' => $set));
  return $was;
}
function t_row($table, $key, $id) { return db_query("select * from $table where $key = :i", array(':i' => $id))->fetchAssoc(); }
function t_n($sql, $args = array()) { return (int) db_query($sql, $args)->fetchField(); }
$RELAXED = 'STRICT_ALL_TABLES,NO_ENGINE_SUBSTITUTION';

// A1-A6: every kind of removed row comes back, also a student with legacy zero dates; a note per course.
t_in_txn(function () use ($RELAXED) {
  $c = t_make_course(array('c_finalized' => 1));
  $d = t_make_course(array('c_finalized' => 1));
  $centre = (int) t_n("select c_center from dh_course where c_id = :c", array(':c' => $c));
  $keep = t_make_applicant($c, array('a_l_name' => 'Keeper'));
  dh_course_sync($c, array('mail' => FALSE, 'files' => 'none'));                       // the keeper's records, not in the batch
  $keep_sid = (int) t_n("select sc_student from dh_student_course where sc_course = :c", array(':c' => $c));
  $dup = (int) db_insert('dh_student_course')->fields(array('sc_student' => $keep_sid, 'sc_center' => $centre, 'sc_course' => $c, 'sc_comment' => 'copy'))->execute();   // copy-removed
  $s2 = t_make_student(array('s_l_name' => 'Stale2'));                                    // course-removed (the student has another course, so stays)
  $stale2 = (int) db_insert('dh_student_course')->fields(array('sc_student' => $s2, 'sc_center' => $centre, 'sc_course' => $c, 'sc_comment' => 'stale'))->execute();
  db_insert('dh_student_course')->fields(array('sc_student' => $s2, 'sc_center' => $centre, 'sc_course' => $d))->execute();
  $mode = t_sql_mode($RELAXED);                                                          // student-removed + sci-removed, with legacy zero dates
  $s3 = t_make_student(array('s_l_name' => 'Stale3', 's_updated' => '0000-00-00 00:00:00', 's_dob' => '0000-00-00'));
  t_sql_mode($mode);
  $stale3 = (int) db_insert('dh_student_course')->fields(array('sc_student' => $s3, 'sc_center' => $centre, 'sc_course' => $c))->execute();
  $sci3 = (int) db_insert('dh_student_course_input')->fields(array('sci_student' => $s3, 'sci_10d' => 2, 'sci_service' => 4))->execute();
  $s4 = t_make_student(array('s_l_name' => 'Before', 's_passport' => 'ZQ5550001', 's_dob' => '1966-06-06'));      // name-change (strong match by passport + date of birth)
  t_make_applicant($c, array('a_l_name' => 'After', 'a_passport' => 'ZQ5550001', 'a_dob' => '1966-06-06'));

  $b4 = array('dup' => t_row('dh_student_course', 'sc_id', $dup), 'stale2' => t_row('dh_student_course', 'sc_id', $stale2), 'stale3' => t_row('dh_student_course', 'sc_id', $stale3),
    's3' => t_row('dh_student', 's_id', $s3), 'sci3' => t_row('dh_student_course_input', 'sci_id', $sci3));
  $s = dh_course_sync($c, array('mail' => FALSE, 'files' => 'none', 'batch' => 'test-bf-u1'));
  t_eq(array($s['copies'], $s['removed'], $s['students_removed']), array(1, 2, 1), 'sync removed a copy, two course records and one student');
  t_eq(array(t_n("select count(*) from dh_student where s_id = :s", array(':s' => $s3)), t_row('dh_student', 's_id', $s4)['s_l_name']), array(0, 'After'), 'the student is gone, the name changed');

  $mode = t_sql_mode();
  $n = dh_backfill_undo('test-bf-u1', TRUE);
  t_eq(t_sql_mode(), $mode, 'A1: the SQL mode is back as it was after the undo');
  t_eq(array($n['_failed'], $n['_failed_ids']), array(0, array()), 'A2: nothing failed');
  t_ok(!empty($n['copy-removed']) && !empty($n['course-removed']) && !empty($n['student-removed']) && !empty($n['sci-removed']) && !empty($n['name-change']), 'A4: the log has every kind of row');
  t_eq(t_row('dh_student_course', 'sc_id', $dup), $b4['dup'], 'copy-removed: the copy is back, equal to before');
  t_eq(t_row('dh_student_course', 'sc_id', $stale2), $b4['stale2'], 'course-removed: the course record is back, equal to before');
  t_eq(t_row('dh_student_course', 'sc_id', $stale3), $b4['stale3'], 'the course record of the removed student is back');
  t_eq(t_row('dh_student', 's_id', $s3), $b4['s3'], 'student-removed: the student is back with its zero dates, equal to before');
  t_eq(array($b4['s3']['s_dob'], $b4['s3']['s_updated']), array('0000-00-00', '0000-00-00 00:00:00'), 'the removed student really had zero dates');
  t_eq(t_row('dh_student_course_input', 'sci_id', $sci3), $b4['sci3'], 'sci-removed: the numbers are back, equal to before');
  t_eq(t_row('dh_student', 's_id', $s4)['s_l_name'], 'Before', 'name-change: the old name is back');
  t_eq(t_n("select count(*) from dh_course_log where crl_course = :c and crl_field = 'finalize_summary' and crl_new = 'Back-fill batch test-bf-u1 undone'", array(':c' => $c)), 1, 'A5: one Change Log note for the course');
  t_eq(t_n("select count(*) from dh_course_log where crl_course = :c and crl_new like 'Back-fill batch%'", array(':c' => $d)), 0, 'A5: no note for a course the batch did not touch');
  t_eq(array_values($n['_courses']), array($c), 'A5: the touched course is listed');

  $n = dh_backfill_undo('test-bf-u1', TRUE);                                              // a second undo: everything is already back
  t_eq(array($n['_undone'], $n['_failed']), array(0, 0), 'A4: a second undo undoes nothing and fails nothing');
  t_ok($n['_skipped'] > 0, 'A4: and counts the rows it skipped');
  t_eq(t_n("select count(*) from dh_course_log where crl_course = :c and crl_new = 'Back-fill batch test-bf-u1 undone'", array(':c' => $c)), 1, 'A5: no second note');
});

// A2/A4: a row that cannot be restored is counted as failed and the next row is still undone.
t_in_txn(function () {
  $c = t_make_course(array('c_finalized' => 1));
  $s = t_make_student(array('s_l_name' => 'Failing'));
  // Two ids that are free (made by inserting a row and removing it again, so no auto-increment counter is pushed up).
  $free = array();
  for ($i = 0; $i < 2; $i++) { $free[] = $x = (int) db_insert('dh_student_course')->fields(array('sc_student' => $s, 'sc_center' => 1, 'sc_course' => $c))->execute(); db_delete('dh_student_course')->condition('sc_id', $x)->execute(); }
  dh_student_log(array('sl_student' => $s, 'sl_course' => $c, 'sl_action' => 'copy-removed', 'sl_batch' => 'test-bf-u2',
    'sl_old' => json_encode(array('sc_id' => $free[0], 'sc_student' => $s, 'sc_center' => 1, 'sc_course' => $c))));                       // older: can be restored
  dh_student_log(array('sl_student' => $s, 'sl_course' => $c, 'sl_action' => 'course-removed', 'sl_batch' => 'test-bf-u2',
    'sl_old' => json_encode(array('sc_id' => $free[1], 'sc_student' => $s, 'sc_center' => 1, 'sc_course' => $c, 'no_such_column' => 1))));  // newer: fails
  dh_student_log(array('sl_student' => $s, 'sl_course' => $c, 'sl_action' => 'sci-removed', 'sl_batch' => 'test-bf-u2', 'sl_old' => 'not json'));  // newest: unreadable
  $ids = db_query("select sl_id from dh_student_log where sl_batch = 'test-bf-u2' and sl_action <> 'copy-removed' order by sl_id desc")->fetchCol();
  $n = dh_backfill_undo('test-bf-u2', TRUE);
  t_eq(array($n['_undone'], $n['_failed']), array(1, 2), 'one row undone, two failed');
  t_eq($n['_failed_ids'], array_map('intval', $ids), 'the failed rows are listed by sl_id, newest first');
  t_eq(t_n("select count(*) from dh_student_course where sc_id = :i", array(':i' => $free[0])), 1, 'the row after the failures was still undone');
  $n = dh_backfill_undo('test-bf-u2', TRUE);
  t_eq(array($n['_undone'], $n['_skipped'], $n['_failed']), array(0, 1, 2), 'run again: the restored row is skipped, the two bad rows fail again');
  $n = dh_backfill_undo('test-bf-u2', FALSE);
  t_eq(array_keys($n), array('sci-removed', 'course-removed', 'copy-removed'), 'the preview lists the actions and counts only');
});

// A3: chunks. More log rows than one chunk: all are seen, newest first.
t_in_txn(function () {
  $c = t_make_course(array('c_finalized' => 1));
  $s = t_make_student(array('s_l_name' => 'Chunky'));
  for ($i = 0; $i < 2005; $i++)
    db_insert('dh_student_log')->fields(array('sl_student' => $s, 'sl_course' => $c, 'sl_action' => 'sci-added', 'sl_new' => (string) (990100000 + $i), 'sl_batch' => 'test-bf-u3', 'sl_user' => 0, 'sl_tstamp' => date('Y-m-d H:i:s')))->execute();
  $n = dh_backfill_undo('test-bf-u3', FALSE);
  t_eq($n, array('sci-added' => 2005), 'A3: the preview counts rows from both chunks');
  $n = dh_backfill_undo('test-bf-u3', TRUE);
  t_eq(array($n['_undone'], $n['_skipped'], $n['_failed']), array(0, 2005, 0), 'A3: all 2005 rows were visited (their rows are not there, so skipped)');
});

// B: a batch sync logs what it changes in place; the undo puts it back.
t_in_txn(function () {
  $c = t_make_course(array('c_finalized' => 1));
  $centre = (int) t_n("select c_center from dh_course where c_id = :c", array(':c' => $c));
  t_make_applicant($c, array('a_l_name' => 'Claimer', 'a_dob' => '1961-01-01'));
  $s1 = t_make_student(array('s_l_name' => 'Claimer', 's_dob' => '1961-01-01', 's_first' => 0));
  $sc1 = (int) db_insert('dh_student_course')->fields(array('sc_student' => $s1, 'sc_center' => $centre, 'sc_course' => $c, 'sc_comment' => 'typed long ago'))->execute();    // no applicant noted: gets claimed
  $sci1 = (int) db_insert('dh_student_course_input')->fields(array('sci_student' => $s1, 'sci_10d' => 5, 'sci_service' => 2))->execute();
  t_make_applicant($c, array('a_l_name' => 'NoNumbers', 'a_dob' => '1962-02-02'));
  $s2 = t_make_student(array('s_l_name' => 'NoNumbers', 's_dob' => '1962-02-02'));                                                                                           // has no numbers row
  $before = array('sc' => t_row('dh_student_course', 'sc_id', $sc1), 'sci' => t_row('dh_student_course_input', 'sci_id', $sci1), 's1' => t_row('dh_student', 's_id', $s1));

  dh_course_sync($c, array('mail' => FALSE, 'files' => 'none', 'batch' => 'test-bf-b1'));
  $log = function ($a) { return t_n("select count(*) from dh_student_log where sl_batch = 'test-bf-b1' and sl_action = :a", array(':a' => $a)); };
  t_eq(array($log('course-updated'), $log('sci-updated'), $log('sci-added')), array(1, 1, 1), 'B1-B3: one course-updated, one sci-updated, one sci-added row');
  t_ok(t_row('dh_student_course', 'sc_id', $sc1)['sc_applicant'] > 0, 'the old course record was claimed by the applicant');
  t_eq(t_row('dh_student_course_input', 'sci_id', $sci1)['sci_10d'], 1, 'the numbers row was overwritten');
  t_eq(t_n("select count(*) from dh_student_course_input where sci_student = :s", array(':s' => $s2)), 1, 'a numbers row was made for the student who had none');
  t_eq(t_row('dh_student', 's_id', $s1)['s_first'], 1, 'first course marked');
  t_eq(t_n("select count(*) from dh_student_log where sl_batch = 'test-bf-b1' and sl_action = 'updated' and sl_field = 's_first' and sl_old = '0' and sl_new = '1'"), 2, 'B4: s_first is logged like the other fields (two students)');
  $l = db_query("select sl_old, sl_new from dh_student_log where sl_batch = 'test-bf-b1' and sl_action = 'course-updated'")->fetchAssoc();
  t_eq(array(json_decode($l['sl_old'], TRUE)['sc_comment'], isset(json_decode($l['sl_new'], TRUE)['sc_applicant'])), array('typed long ago', TRUE), 'the log holds the old row and the changed values');

  dh_backfill_undo('test-bf-b1', TRUE);
  t_eq(t_row('dh_student_course', 'sc_id', $sc1), $before['sc'], 'B1: undo puts the course record back, equal to before');
  t_eq(t_row('dh_student_course_input', 'sci_id', $sci1), $before['sci'], 'B2: undo puts the numbers row back, equal to before');
  t_eq(t_n("select count(*) from dh_student_course_input where sci_student = :s", array(':s' => $s2)), 0, 'B3: undo removes the new numbers row');
  $after = t_row('dh_student', 's_id', $s1);
  t_eq($after['s_first'], 0, 'B4: undo puts s_first back');
  t_eq(array_values(array_diff(array_keys(array_diff_assoc($before['s1'], $after)), array('s_updated', 's_updated_by'))), array(), 'B6: no column differs from before except the audit stamps (they are not logged and not reversed)');
});

// B5: s3copy writes the photo twice (the applicant's path, then the final path): both steps are logged, undo walks both back.
function t_s3_head($bucket, $key) { return $key === 'photos/zzq/b5.jpg' ? 1000 : FALSE; }
function t_s3_copy($bucket, $from, $to) { return array('success' => TRUE, 'message' => ''); }
t_in_txn(function () {
  $c = t_make_course(array('c_finalized' => 1));
  t_make_applicant($c, array('a_l_name' => 'Photo', 'a_dob' => '1963-03-03', 'a_photo' => 'private://photos/zzq/b5.jpg'));
  $s = t_make_student(array('s_l_name' => 'Photo', 's_dob' => '1963-03-03', 's_photo' => 'private://old/photo.jpg'));
  dh_course_sync($c, array('mail' => FALSE, 'files' => 's3copy', 's3_head' => 't_s3_head', 's3_copy' => 't_s3_copy', 'batch' => 'test-bf-b5'));
  $final = t_row('dh_student', 's_id', $s)['s_photo'];
  t_ok(strpos($final, 'private://final/photo-id/') === 0, 'the final photo path is stored');
  $rows = db_query("select sl_old, sl_new from dh_student_log where sl_batch = 'test-bf-b5' and sl_field = 's_photo' order by sl_id")->fetchAll(PDO::FETCH_NUM);
  t_eq($rows, array(array('private://old/photo.jpg', 'private://photos/zzq/b5.jpg'), array('private://photos/zzq/b5.jpg', $final)), 'B5: two photo rows: old to applicant path, applicant path to final path');
  dh_backfill_undo('test-bf-b5', TRUE);
  t_eq(t_row('dh_student', 's_id', $s)['s_photo'], 'private://old/photo.jpg', 'B5: undo walks both steps back');
});

// D3/D4: the 'created' undo keeps a student that a legacy applicant link or a patrika subscription points to.
t_in_txn(function () {
  $c = t_make_course(array('c_finalized' => 1));
  $a = array();
  foreach (array('A', 'B', 'C', 'D') as $k) $a[$k] = t_make_applicant($c, array('a_l_name' => "Pointed$k"));
  dh_course_sync($c, array('mail' => FALSE, 'files' => 'none', 'batch' => 'test-bf-d3'));
  $sid = function ($k) use ($a) { return (int) t_n("select sl_student from dh_student_log where sl_batch = 'test-bf-d3' and sl_action = 'created' and sl_applicant = :a", array(':a' => $a[$k])); };
  db_update('dh_applicant')->fields(array('a_student' => $sid('A'), 'a_updated_by' => 0))->condition('a_id', $a['A'])->execute();
  db_update('dh_applicant')->fields(array('a_old_student' => $sid('B'), 'a_updated_by' => 0))->condition('a_id', $a['B'])->execute();
  db_insert('dh_patrika')->fields(array('p_student' => $sid('C'), 'p_updated' => date('Y-m-d H:i:s')))->execute();
  $n = dh_backfill_undo('test-bf-d3', TRUE);
  $there = function ($k) use ($sid) { return t_n("select count(*) from dh_student where s_id = :s", array(':s' => $sid($k))); };
  t_eq(array($there('A'), $there('B'), $there('C'), $there('D')), array(1, 1, 1, 0), 'D3: students pointed to by a_student, a_old_student or a patrika row stay; the other one goes');
  t_eq(array($n['_undone'], $n['_skipped'], $n['_failed']), array(5, 3, 0), 'D3: 4 course records and 1 student undone; 3 students counted as skipped');
});

// D4: a student is not removed while an applicant points to it; when it is removed the patrika subscription stays with its link set to 0, and the undo relinks it.
t_in_txn(function () {
  $c = t_make_course(array('c_finalized' => 1));
  $centre = (int) t_n("select c_center from dh_course where c_id = :c", array(':c' => $c));
  $linked = t_make_student(array('s_l_name' => 'Linked'));
  $gone = t_make_student(array('s_l_name' => 'Gone'));
  foreach (array($linked, $gone) as $s) db_insert('dh_student_course')->fields(array('sc_student' => $s, 'sc_center' => $centre, 'sc_course' => $c))->execute();
  $other = t_make_applicant(t_make_course(), array('a_l_name' => 'Pointer'));
  db_update('dh_applicant')->fields(array('a_student' => $linked, 'a_updated_by' => 0))->condition('a_id', $other)->execute();
  $pid = (int) db_insert('dh_patrika')->fields(array('p_student' => $gone, 'p_updated' => date('Y-m-d H:i:s')))->execute();
  $s = dh_course_sync($c, array('mail' => FALSE, 'files' => 'none', 'batch' => 'test-bf-d4'));
  t_eq(array($s['removed'], $s['students_removed']), array(2, 1), 'D4: both course records are removed, only one student');
  t_eq(t_n("select count(*) from dh_student where s_id = :s", array(':s' => $linked)), 1, 'D4: a student an applicant points to (a_student) is kept');
  t_eq(array(t_n("select count(*) from dh_student where s_id = :s", array(':s' => $gone)), t_n("select p_student from dh_patrika where p_id = :p", array(':p' => $pid))), array(0, 0), 'D4: the other student is removed and the patrika link is set to 0');
  t_eq(t_n("select count(*) from dh_patrika where p_id = :p", array(':p' => $pid)), 1, 'D4: the patrika subscription itself is kept');
  dh_backfill_undo('test-bf-d4', TRUE);
  t_eq(array(t_n("select count(*) from dh_student where s_id = :s", array(':s' => $gone)), t_n("select p_student from dh_patrika where p_id = :p", array(':p' => $pid))), array(1, $gone), 'D4: undo brings the student back and relinks the patrika row');
});
t_done();
