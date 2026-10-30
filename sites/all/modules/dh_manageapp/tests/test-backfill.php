<?php
require __DIR__ . '/lib.php';
echo "Back-fill\n";
t_in_txn(function () {
  $none = t_make_course(array('c_finalized' => 1));            // finalized but no records, like a migrated course
  t_make_applicant($none); t_make_applicant($none);
  $copies = t_make_course(array('c_finalized' => 1));
  t_make_applicant($copies);
  dh_course_sync($copies, array('mail' => FALSE, 'files' => 'none'));
  $xs = db_query("select sc_student from dh_student_course where sc_course = :c", array(':c' => $copies))->fetchField();
  db_insert('dh_student_course')->fields(array('sc_student' => $xs, 'sc_center' => 1, 'sc_course' => $copies))->execute();
  $centre = (int) db_query("select c_center from dh_course where c_id = :c", array(':c' => $none))->fetchField();
  $b = in_array($centre, array(335, 343, 344)) ? 3 : 4;
  t_ok(in_array($none, dh_backfill_courses($b)), "course with no records is in batch $b");
  t_ok(in_array($copies, dh_backfill_courses(2)), 'course with a copy is in batch 2');

  $before = db_query("select count(*) from dh_student_course")->fetchField();
  $s = dh_backfill_dry_run($none);
  t_eq($s['added'], 2, 'dry run: would add 2');
  t_eq(db_query("select count(*) from dh_student_course")->fetchField(), $before, 'dry run writes nothing');

  $GLOBALS['t_mail'] = array();
  $s = dh_course_sync($none, array('mail' => FALSE, 'mail_fn' => 't_record_mail', 'files' => 'none', 'batch' => 'test-bf'));
  t_eq(array($s['added'], count($GLOBALS['t_mail'])), array(2, 0), 'apply: 2 added, no email');
  t_ok(!in_array($none, dh_backfill_courses($b)), 'a done course leaves its batch (resume skips it)');
  $s = dh_course_sync($copies, array('mail' => FALSE, 'files' => 'none', 'batch' => 'test-bf'));
  t_eq($s['copies'], 1, 'copy removed');

  $n = dh_backfill_undo('test-bf', FALSE);
  t_ok(!empty($n['course-added']) && !empty($n['copy-removed']), 'undo preview lists the changes');
  t_eq(db_query("select count(*) from dh_student_course where sc_course = :c", array(':c' => $none))->fetchField(), 2, 'undo preview changes nothing');
  dh_backfill_undo('test-bf', TRUE);
  t_eq(db_query("select count(*) from dh_student_course where sc_course = :c", array(':c' => $none))->fetchField(), 0, 'undo: added records gone');
  t_eq(db_query("select count(*) from dh_student_course where sc_course = :c", array(':c' => $copies))->fetchField(), 2, 'undo: copy back');
  t_eq(db_query("select count(*) from dh_student s join dh_student_log l on l.sl_student = s.s_id and l.sl_action = 'created' and l.sl_batch = 'test-bf'")->fetchField(), 0, 'undo: created students gone');
});

// Batch 1 (gaps), the courses no batch may pick, and an unknown batch.
t_in_txn(function () {
  $gap = t_make_course(array('c_finalized' => 1));
  t_make_applicant($gap); t_make_applicant($gap);
  dh_course_sync($gap, array('mail' => FALSE, 'files' => 'none'));
  t_ok(!in_array($gap, dh_backfill_courses(1)), 'batch 1: a course with a record for every attended student is not listed');
  t_make_applicant($gap);                                        // attended 3, records for 2
  t_ok(in_array($gap, dh_backfill_courses(1)), 'batch 1: a course with fewer students with records than attended is listed');
  t_ok(!in_array($gap, dh_backfill_courses(3)) && !in_array($gap, dh_backfill_courses(4)), 'batches 3 and 4 only take courses with no records at all');
  dh_course_sync($gap, array('mail' => FALSE, 'files' => 'none'));
  t_ok(!in_array($gap, dh_backfill_courses(1)), 'batch 1: the gap is gone after the sync (resume skips it)');

  $open = t_make_course(array('c_finalized' => 0)); t_make_applicant($open);
  $gone = t_make_course(array('c_finalized' => 1, 'c_deleted' => 1)); t_make_applicant($gone);
  $oneday = (int) db_query("select td_id from dh_type_detail where td_key = '1-DayOSC' limit 1")->fetchField();
  $one = t_make_course(array('c_finalized' => 1, 'c_course_type' => $oneday)); t_make_applicant($one);
  $listed = array();
  foreach (array(1, 2, 3, 4) as $x) $listed = array_merge($listed, dh_backfill_courses($x));
  t_ok(!in_array($open, $listed), 'a course that is not finalized is in no batch');
  t_ok(!in_array($gone, $listed), 'a deleted course is in no batch');
  t_ok(!in_array($one, $listed), 'a 1-Day course is in no batch');
  t_eq(dh_backfill_courses(0), array(), 'an unknown batch lists nothing');
  t_eq(dh_backfill_courses(5), array(), 'batch 5 does not exist');
  // Batches 3 and 4 split on the migrated-centre list, which is a parameter.
  $fresh = t_make_course(array('c_finalized' => 1)); t_make_applicant($fresh);
  $centre = (int) db_query("select c_center from dh_course where c_id = :c", array(':c' => $fresh))->fetchField();
  t_ok(in_array($fresh, dh_backfill_courses(3, array($centre))) && !in_array($fresh, dh_backfill_courses(4, array($centre))), 'batch 3: a course of a migrated centre with no records (not in batch 4)');
  t_ok(in_array($fresh, dh_backfill_courses(4, array(-1))) && !in_array($fresh, dh_backfill_courses(3, array(-1))), 'batch 4: a course of another centre with no records (not in batch 3)');
});

// Files mode: a made-up mode is refused before anything is written, in the sync and in the dry run.
t_in_txn(function () {
  $c = t_make_course(array('c_finalized' => 1)); t_make_applicant($c);
  $stat = db_query("select count(*) from dh_course_stat where cs_course = :c", array(':c' => $c))->fetchField();
  $threw = FALSE;
  try { dh_course_sync($c, array('mail' => FALSE, 'files' => 'bogus', 'batch' => 'test-bf3')); }
  catch (Exception $e) { $threw = (strpos($e->getMessage(), 'Unknown files mode: bogus') === 0); }
  t_ok($threw, 'sync: an unknown files mode throws');
  t_eq(array(db_query("select count(*) from dh_student_course where sc_course = :c", array(':c' => $c))->fetchField(),
    db_query("select count(*) from dh_student_log where sl_course = :c", array(':c' => $c))->fetchField(),
    db_query("select count(*) from dh_course_stat where cs_course = :c", array(':c' => $c))->fetchField()), array(0, 0, $stat), 'sync: an unknown files mode writes nothing');
  foreach (array('bogus', 's3copy', 'copy') as $mode)
  {
    $threw = FALSE;
    try { dh_backfill_dry_run($c, $mode); }
    catch (Exception $e) { $threw = (strpos($e->getMessage(), 'Dry run files mode must be none or count') === 0); }
    t_ok($threw, "dry run: files mode '$mode' throws (a dry run only counts files; not hidden as an error count)");
  }
  $s = dh_backfill_dry_run($c, 'none');
  t_eq(array($s['added'], $s['files_failed'], $s['errors']), array(1, 0, 0), 'dry run with files none: adds 1, no failures, no errors');
  t_eq(db_query("select count(*) from dh_student_course where sc_course = :c", array(':c' => $c))->fetchField(), 0, 'dry run with files none writes nothing');
});

// Undo: an unknown date comes back as NULL (a date column refuses ''), and a value someone changed since stays.
t_in_txn(function () {
  $s1 = t_make_student(array('s_f_name' => 'Zzqtest', 's_l_name' => 'Nodob', 's_passport' => 'ZQ1234567'));
  $s2 = t_make_student(array('s_f_name' => 'Zzqtest', 's_l_name' => 'Nodobtwo', 's_passport' => 'ZQ7654321'));
  $c = t_make_course(array('c_finalized' => 1));
  t_make_applicant($c, array('a_l_name' => 'Nodob', 'a_dob' => '1969-09-09', 'a_passport' => 'ZQ1234567'));
  t_make_applicant($c, array('a_l_name' => 'Nodobtwo', 'a_dob' => '1968-08-08', 'a_passport' => 'ZQ7654321'));
  t_eq(array(db_query("select s_dob from dh_student where s_id = :s", array(':s' => $s1))->fetchField(),
    db_query("select s_dob from dh_student where s_id = :s", array(':s' => $s2))->fetchField()), array(NULL, NULL), 'both made-up students start with no date of birth');
  dh_course_sync($c, array('mail' => FALSE, 'files' => 'none', 'batch' => 'test-bf4'));
  t_eq(db_query("select s_dob from dh_student where s_id = :s", array(':s' => $s1))->fetchField(), '1969-09-09', 'the sync filled the date of birth');
  $l = db_query("select sl_action, sl_old, sl_new from dh_student_log where sl_batch = 'test-bf4' and sl_student = :s and sl_field = 's_dob'", array(':s' => $s1))->fetchAssoc();
  t_eq($l, array('sl_action' => 'updated', 'sl_old' => '', 'sl_new' => '1969-09-09'), 'the log holds the cleaned old value (empty) and the new date');
  db_update('dh_student')->fields(array('s_dob' => '1970-01-01'))->condition('s_id', $s2)->execute();     // somebody edits it after the sync
  $n = dh_backfill_undo('test-bf4', TRUE);
  t_ok(!empty($n['updated']) && !empty($n['course-added']), 'undo counts updated and course-added rows');
  t_eq(db_query("select s_dob from dh_student where s_id = :s", array(':s' => $s1))->fetchField(), NULL, 'undo: an unknown old date comes back as NULL');
  t_eq(db_query("select s_dob from dh_student where s_id = :s", array(':s' => $s2))->fetchField(), '1970-01-01', 'undo: a date changed since the sync is left alone');
  t_eq(db_query("select count(*) from dh_student_course where sc_course = :c", array(':c' => $c))->fetchField(), 0, 'undo: course records gone');
});
t_done();
