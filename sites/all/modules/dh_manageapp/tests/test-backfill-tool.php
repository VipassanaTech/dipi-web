<?php
require __DIR__ . '/lib.php';
echo "Back-fill tool (command line)\n";
// Runs the tool as a child process from the Drupal root (this test is run from there too). Returns array(exit code, output).
function t_tool($args) {
  exec(escapeshellarg(PHP_BINARY) . ' sites/all/modules/dh_manageapp/deploy/2026-11-backfill-student-records.php ' . $args . ' 2>&1', $out, $code);
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
$c = 0; $apps = array();
try {
  $c = t_make_course(array('c_finalized' => 1));
  foreach (array(1, 2) as $i) $apps[] = t_make_applicant($c, array('a_l_name' => 'Cli' . mt_rand(100000, 999999) . $i));
  $made = $counts();

  list($code, $out) = t_tool("--courses=$c --no-files");
  t_eq($code, 0, 'dry run of the made-up course: exit 0');
  t_ok(strpos($out, 'DRY RUN: 1 courses, email OFF, files none') !== FALSE && strpos($out, '2 students, +2,') !== FALSE, 'dry run output: one course, email off, +2');
  t_eq($counts(), $made, 'dry run wrote nothing');

  list($code, $out) = t_tool("--courses=$c --apply --no-files");
  t_eq($code, 0, 'apply of the made-up course: exit 0');
  $id = preg_match('/APPLY, batch id (bf-list-\d{8}-\d{6}):/', $out, $m) ? $m[1] : '';
  t_ok($id !== '', 'apply prints a batch id with seconds');
  t_eq(array(t_n('select count(*) from dh_student_course where sc_course = :c', array(':c' => $c)), t_n('select count(*) from dh_student_log where sl_batch = :b and sl_action = \'created\'', array(':b' => $id))), array(2, 2), 'apply: 2 course records, 2 students created');
  t_eq(t_n("select count(*) from dh_course_log where crl_course = :c and crl_new like :t", array(':c' => $c, ':t' => "Back-fill $id (no emails)%")), 1, 'apply: one Change Log row on the course');

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
}
finally {
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
