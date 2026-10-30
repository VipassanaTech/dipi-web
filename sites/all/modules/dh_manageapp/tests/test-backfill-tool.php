<?php
require __DIR__ . '/lib.php';
echo "Back-fill tool (command line)\n";
// Runs the tool as a child process from the Drupal root (this test is run from there too). Returns array(exit code, output).
$TOOL = 'sites/all/modules/dh_manageapp/deploy/2026-11-backfill-student-records.php';
function t_tool($args, $script = NULL) {
  global $TOOL;
  exec(escapeshellarg(PHP_BINARY) . ' ' . escapeshellarg($script ? $script : $TOOL) . ' ' . $args . ' 2>&1', $out, $code);
  return array($code, implode("\n", $out));
}
function t_n($sql, $args = array()) { return (int) db_query($sql, $args)->fetchField(); }
$tables = array('dh_course', 'dh_applicant', 'dh_applicant_attended', 'dh_student', 'dh_student_course', 'dh_student_course_input', 'dh_student_log', 'dh_course_log', 'dh_course_stat', 'dh_log');
$counts = function () use ($tables) { $o = array(); foreach ($tables as $t) $o[$t] = t_n("select count(*) from $t"); return $o; };

// Bad options: exit 1 and nothing written. (A mistyped option must never start a run.)
$logs = t_n('select count(*) from dh_student_log');
foreach (array('--bogus', '--mail', '--batch=7', '--limit=0', '--batch=1 --courses=1', '--batch=4 --apply --limit 5', '--apply') as $args) {
  list($code, $out) = t_tool($args);
  t_eq($code, 1, "exit code 1 for: $args");
  t_ok(strpos($out, 'DRY RUN') === FALSE && strpos($out, 'APPLY') === FALSE, "nothing started for: $args");
}
t_eq(t_n('select count(*) from dh_student_log'), $logs, 'the bad options wrote no student log row');

// A made-up course that is committed (the tool runs in another process), removed again at the end.
$before = $counts();
$c = 0; $apps = array(); $tmp = '';
try {
  $c = t_make_course(array('c_finalized' => 1));
  foreach (array(1, 2) as $i) $apps[] = t_make_applicant($c, array('a_l_name' => 'Cli' . mt_rand(100000, 999999) . $i));
  $made = $counts();

  list($code, $out) = t_tool("--courses=$c --no-files");
  t_eq($code, 0, 'dry run of the made-up course: exit 0');
  t_ok(strpos($out, 'DRY RUN: 1 courses, email OFF, files none') !== FALSE && strpos($out, '2 students, +2,') !== FALSE, 'dry run output: one course, email off, +2');
  t_eq($counts(), $made, 'dry run wrote nothing');
  t_eq(t_n('select count(*) from dh_applicant where a_id in (:a) and a_attended_mail is not null', array(':a' => $apps)), 0, 'dry run: no applicant is dated as emailed');
  t_ok(lock_may_be_available(dh_finalize_lock_name($c)), "dry run: the course's finalize lock is let go afterwards");

  // A finalize of the course is running (another process holds its lock): the tool skips the course, dry run and apply.
  db_insert('semaphore')->fields(array('name' => dh_finalize_lock_name($c), 'value' => 'test-other-process', 'expire' => microtime(TRUE) + 600))->execute();
  list($code, $out) = t_tool("--courses=$c --no-files");
  t_ok(strpos($out, "course $c busy (finalize running), skipped") !== FALSE && strpos($out, '2 students') === FALSE, 'busy: the dry run skips the course');
  t_ok(strpos($out, "Courses skipped because a finalize was running or waiting (run them again later with --courses): $c") !== FALSE, 'busy: the skipped course is listed at the end');
  list($code, $out) = t_tool("--courses=$c --apply --no-files");
  t_ok(strpos($out, "course $c busy (finalize running), skipped") !== FALSE, 'busy: the apply skips the course');
  t_eq($counts(), $made, 'busy: nothing written, no Change Log note');
  t_eq(db_query('select value from semaphore where name = :n', array(':n' => dh_finalize_lock_name($c)))->fetchField(), 'test-other-process', "busy: the other process's lock is left alone");
  db_delete('semaphore')->condition('name', dh_finalize_lock_name($c))->execute();

  // A finalize of the course waits for the cron (flag 1, c_processed 0): skipped the same way, dry run and apply.
  db_update('dh_course')->fields(array('c_finalize_requested' => 1, 'c_processed' => 0))->condition('c_id', $c)->execute();
  list($code, $out) = t_tool("--courses=$c --no-files");
  t_ok(strpos($out, "course $c busy (finalize waiting), skipped") !== FALSE && strpos($out, '2 students') === FALSE, 'waiting: the dry run skips the course');
  list($code, $out) = t_tool("--courses=$c --apply --no-files");
  t_ok(strpos($out, "course $c busy (finalize waiting), skipped") !== FALSE && strpos($out, "Courses skipped because a finalize was running or waiting (run them again later with --courses): $c") !== FALSE, 'waiting: the apply skips the course and lists it');
  db_update('dh_course')->fields(array('c_finalize_requested' => 0, 'c_processed' => 1))->condition('c_id', $c)->execute();
  t_eq($counts(), $made, 'waiting: nothing written, no Change Log note');

  // A 1-Day course is refused even when named with --courses, dry run and apply (1-Day courses never get student records).
  $type = t_n('select c_course_type from dh_course where c_id = :c', array(':c' => $c));
  $oneday = t_n("select td_id from dh_type_detail where td_type = 'COURSE-TYPE' and td_key = '1-DayOSC' limit 1");
  db_update('dh_course')->fields(array('c_course_type' => $oneday))->condition('c_id', $c)->execute();
  $made1 = $counts();
  list($code, $out) = t_tool("--courses=$c --no-files");
  t_ok(strpos($out, "course $c: a 1-Day course (1-Day courses never get student records), skipped") !== FALSE && strpos($out, '2 students') === FALSE, 'C: the dry run refuses a named 1-Day course');
  list($code, $out) = t_tool("--courses=$c --apply --no-files");
  t_ok(strpos($out, "course $c: a 1-Day course (1-Day courses never get student records), skipped") !== FALSE, 'C: the apply refuses it too');
  t_eq($counts(), $made1, 'C: nothing written for the 1-Day course');
  db_update('dh_course')->fields(array('c_course_type' => $type))->condition('c_id', $c)->execute();
  $made = $counts();                                             // (the two type changes wrote Change Log rows; the cleanup removes them)

  // When a course's sync throws, the screen gets only the course id, the exception class and the SQLSTATE; the message (it can echo an ID or a name) goes to the log.
  // A copy of the tool whose sync call throws a PDOException with a recognisable message stands in for a database error.
  $src = file_get_contents($TOOL);
  $copy = str_replace("dh_course_sync(\$c, array('mail' => FALSE", "t_boom(\$c, array('mail' => FALSE", $src);
  t_ok($copy !== $src, 'the stand-in copy of the tool replaced the sync call');
  $copy = preg_replace('/^<\?php/', "<?php\nfunction t_boom() { \$e = new PDOException('Incorrect value SECRET-ID-123456789 for column x'); \$e->errorInfo = array('22007', 1292, 'x'); throw \$e; }", $copy, 1);
  $tmp = tempnam(sys_get_temp_dir(), 'bf');
  file_put_contents($tmp, $copy);
  list($code, $out) = t_tool("--courses=$c --apply --no-files", $tmp);
  t_eq($code, 1, 'D1: a sync that throws stops the tool with exit 1');
  t_ok(strpos($out, "course $c: STOPPED by an error: PDOException (SQLSTATE 22007)") !== FALSE, 'the screen shows the course id, the exception class and the SQLSTATE');
  t_ok(strpos($out, 'SECRET-ID') === FALSE, 'the exception message is not printed');
  t_ok(strpos($out, "STOP: course $c stopped with an error") !== FALSE && strpos($out, '--undo=bf-list-') !== FALSE, 'and the STOP line names the undo');
  t_ok(strpos($out, "fix the cause, then run again with --courses=$c") !== FALSE, 'and says to fix the cause and run the course again by name');
  $w = db_query("select severity, variables from watchdog where type = 'StudentBackfill' and variables like '%SECRET-ID-123456789%' order by wid desc limit 1")->fetchAssoc();
  t_ok($w && (int) $w['severity'] === WATCHDOG_ERROR && strpos($w['variables'], "i:$c;") !== FALSE, 'the full message is in the log (type StudentBackfill, error) with the course id');
  t_eq($counts(), $made, 'the stopped run wrote nothing');
  // The same, when even the log cannot be written (the database is gone): still the STOP line, exit 1, nothing from the failed log call.
  $copy2 = str_replace("watchdog('StudentBackfill',", "t_wd_boom('StudentBackfill',", $copy);
  t_ok($copy2 !== $copy, 'the second stand-in copy replaced the log call');
  $copy2 = preg_replace('/^<\?php/', "<?php\nfunction t_wd_boom() { throw new Exception('stand-in LOG-GONE failure'); }", $copy2, 1);
  file_put_contents($tmp, $copy2);
  list($code, $out) = t_tool("--courses=$c --apply --no-files", $tmp);
  t_eq($code, 1, 'M4: the log call fails too: still exit 1');
  t_ok(strpos($out, "STOP: course $c stopped with an error") !== FALSE && strpos($out, 'LOG-GONE') === FALSE, 'M4: the STOP line is printed, the failed log call prints nothing');
  t_ok(lock_may_be_available(dh_finalize_lock_name($c)) && lock_may_be_available('dh_student_backfill'), 'M4: the locks are let go');

  list($code, $out) = t_tool("--courses=$c --apply --no-files");
  t_eq($code, 0, 'apply of the made-up course: exit 0');
  $id = preg_match('/APPLY, batch id (bf-list-\d{8}-\d{6}):/', $out, $m) ? $m[1] : '';
  t_ok($id !== '', 'apply prints a batch id with seconds');
  t_eq(array(t_n('select count(*) from dh_student_course where sc_course = :c', array(':c' => $c)), t_n('select count(*) from dh_student_log where sl_batch = :b and sl_action = \'created\'', array(':b' => $id))), array(2, 2), 'apply: 2 course records, 2 students created');
  t_eq(t_n("select count(*) from dh_course_log where crl_course = :c and crl_new like :t", array(':c' => $c, ':t' => "Back-fill $id (no emails)%")), 1, 'apply: one Change Log row on the course');
  $end = db_query('select c_end from dh_course where c_id = :c', array(':c' => $c))->fetchField();
  t_eq(db_query('select distinct a_attended_mail from dh_applicant where a_id in (:a)', array(':a' => $apps))->fetchCol(), array("$end 00:00:00"), 'apply: both applicants are dated as emailed (the course end date: no finalize time), no email sent');

  // The tool's own Change Log row keeps a course out of its batch (checked in a transaction that is rolled back).
  $b = in_array(t_n('select c_center from dh_course where c_id = :c', array(':c' => $c)), array(335, 343, 344)) ? 3 : 4;
  $txn = db_transaction();
  db_delete('dh_student_course')->condition('sc_course', $c)->execute();
  t_ok(!in_array($c, dh_backfill_courses($b)), 'D2: a course with the tool\'s Change Log row is not picked again, even with no records');
  db_delete('dh_course_log')->condition('crl_course', $c)->condition('crl_field', 'finalize_summary')->execute();
  t_ok(in_array($c, dh_backfill_courses($b)), 'D2: without that row the same course would be picked');
  $txn->rollback();

  list($code, $out) = t_tool("--undo=$id");
  t_eq($code, 0, 'undo preview: exit 0');
  t_ok(strpos($out, 'undo would reverse') !== FALSE && strpos($out, '"course-added":2') !== FALSE, 'undo preview lists the changes');
  t_eq(t_n('select count(*) from dh_student_course where sc_course = :c', array(':c' => $c)), 2, 'undo preview changed nothing');

  list($code, $out) = t_tool("--undo=$id --apply");
  t_eq($code, 0, 'undo apply: exit 0');
  t_ok(strpos($out, 'rows undone 4, skipped 0') !== FALSE && strpos($out, 'failed 0') !== FALSE, 'undo apply reports 4 rows undone, 0 skipped, 0 failed');
  t_eq(array(t_n('select count(*) from dh_student_course where sc_course = :c', array(':c' => $c)), t_n('select count(*) from dh_student where s_id in (select sl_student from dh_student_log where sl_batch = :b)', array(':b' => $id))), array(0, 0), 'undo apply: course records and students gone');
  t_eq(t_n("select count(*) from dh_course_log where crl_course = :c and crl_new = :t", array(':c' => $c, ':t' => "Back-fill batch $id undone")), 1, 'undo apply: one Change Log note');
  t_ok(in_array($c, dh_backfill_courses($b)), 'after the undo the course can be picked by its batch again');

  // A course with a failing applicant (its course record points to a student that is not there): listed, with both log names.
  $ghost = 1 + t_n('select max(s_id) from dh_student');
  db_insert('dh_student_course')->fields(array('sc_student' => $ghost, 'sc_applicant' => $apps[0], 'sc_center' => t_n('select c_center from dh_course where c_id = :c', array(':c' => $c)), 'sc_course' => $c))->execute();
  list($code, $out) = t_tool("--courses=$c --no-files");
  t_eq($code, 1, 'a course with errors: exit 1');
  t_ok(strpos($out, "Courses with errors (see the 'Finalization Error' and 'StudentBackfill' logs): $c") !== FALSE, 'M6: the summary names both log types');
  t_eq(t_n("select count(*) from watchdog where type = 'Finalization Error' and message like 'Back-fill dry run:%' and variables like :v", array(':v' => '%i:' . $apps[0] . ';%')), 1, 'M1: the dry run kept the per-applicant error in the log');
}
finally {
  if ($tmp) @unlink($tmp);
  db_delete('watchdog')->condition('type', 'StudentBackfill')->condition('variables', '%SECRET-ID-123456789%', 'LIKE')->execute();
  if ($c) db_delete('semaphore')->condition('name', dh_finalize_lock_name($c))->execute();
  if ($apps) db_delete('watchdog')->condition('type', 'Finalization Error')->condition('message', 'Back-fill dry run:%', 'LIKE')->condition('variables', '%i:' . $apps[0] . ';%', 'LIKE')->execute();
  // Remove everything the test made: students it created (if an undo did not), then the course, its applicants and the rows triggers wrote.
  if ($c) {
    $students = db_query("select distinct sl_student from dh_student_log where sl_course = :c and sl_action = 'created'", array(':c' => $c))->fetchCol();
    db_delete('dh_student_course')->condition('sc_course', $c)->execute();
    if ($students) {
      db_delete('dh_student_course_input')->condition('sci_student', $students, 'IN')->execute();
      db_delete('dh_student')->condition('s_id', $students, 'IN')->execute();
    }
    db_delete('dh_student_log')->condition('sl_course', $c)->execute();
    db_delete('dh_course_log')->condition('crl_course', $c)->execute();
    db_delete('dh_course_stat')->condition('cs_course', $c)->execute();
    if ($apps) {
      db_delete('dh_log')->condition('l_module', 'Applicant')->condition('l_identifier', $apps, 'IN')->execute();
      db_delete('dh_applicant_attended')->condition('aa_applicant', $apps, 'IN')->execute();
      db_delete('dh_applicant')->condition('a_id', $apps, 'IN')->execute();
    }
    db_delete('dh_course')->condition('c_id', $c)->execute();
  }
}
t_eq($counts(), $before, 'the made-up course, applicants and everything written for them are removed');
t_done();
