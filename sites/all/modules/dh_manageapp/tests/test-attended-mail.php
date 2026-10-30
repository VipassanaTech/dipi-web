<?php
require __DIR__ . '/lib.php';
echo "The Attended email is sent at most once\n";
function t_date($a) { return db_query("select a_attended_mail from dh_applicant where a_id = :a", array(':a' => $a))->fetchField(); }
function t_n($sql, $args = array()) { return (int) db_query($sql, $args)->fetchField(); }
// A mail function that throws, as dh_send_letter() does when Mailgun refuses the request.
function t_throw_mail($type, $a_id, $event) { $GLOBALS['t_mail'][] = (int) $a_id; throw new Exception('stand-in mail failure'); }
// A mail function that, while it "sends" to one applicant, lets another finalize of the same course claim the next applicant first.
function t_race_mail($type, $a_id, $event) {
  $GLOBALS['t_mail'][] = (int) $a_id;
  if (!empty($GLOBALS['t_race_next']))
    db_update('dh_applicant')->fields(array('a_attended_mail' => '2026-01-01 01:01:01'))->condition('a_id', $GLOBALS['t_race_next'])->execute();
  $GLOBALS['t_race_next'] = 0;
  return array('email_res_id' => 'test');
}

// C2.1: the claim. A stored date means no send and no change; a NULL date is claimed and sent once.
t_in_txn(function () {
  $GLOBALS['t_mail'] = array();
  $course = t_make_course();
  $done = t_make_applicant($course, array('a_attended_mail' => '2025-05-05 05:05:05'));
  $new = t_make_applicant($course);
  $s = dh_course_sync($course, array('mail_fn' => 't_record_mail', 'files' => 'none'));
  t_eq($GLOBALS['t_mail'], array($new), 'claim: only the applicant with no date is sent the email');
  t_eq(t_date($done), '2025-05-05 05:05:05', 'claim: a stored date is not changed');
  t_ok((bool) t_date($new), 'claim: the claimed date is stored');
  t_eq($s['emails'], 1, 'claim: 1 email counted');
  dh_course_sync($course, array('mail_fn' => 't_record_mail', 'files' => 'none'));
  t_eq($GLOBALS['t_mail'], array($new), 'claim: a second run sends nothing');
});

// C2.1: two finalize runs at once. The rows were read before the other run claimed one: the claim decides, not the stale read.
t_in_txn(function () {
  $GLOBALS['t_mail'] = array();
  $course = t_make_course();
  $first = t_make_applicant($course); $second = t_make_applicant($course);
  $GLOBALS['t_race_next'] = $second;
  $s = dh_course_sync($course, array('mail_fn' => 't_race_mail', 'files' => 'none'));
  t_eq($GLOBALS['t_mail'], array($first), 'race: an applicant claimed by the other run is not sent the email again');
  t_eq(t_date($second), '2026-01-01 01:01:01', "race: the other run's date stays");
  t_eq($s['emails'], 1, 'race: 1 email counted');
});

// I1: a mail failure is not a sync error: the date stays claimed and removals still happen.
t_in_txn(function () {
  $GLOBALS['t_mail'] = array();
  $course = t_make_course();
  $left = t_make_applicant($course, array('a_l_name' => 'Leftlater'));
  dh_course_sync($course, array('mail' => FALSE, 'files' => 'none'));
  db_update('dh_applicant')->fields(array('a_attended' => 0))->condition('a_id', $left)->execute();
  $m = t_make_applicant($course, array('a_l_name' => 'Mailfails'));
  $wid = t_n('select max(wid) from watchdog');
  $s = dh_course_sync($course, array('mail_fn' => 't_throw_mail', 'files' => 'none'));
  t_eq(array($s['errors'], $s['mail_failed'], $s['emails']), array(0, 1, 0), 'mail failure: errors 0, mail_failed 1, emails 0');
  t_eq(array($s['added'], $s['removed']), array(1, 1), 'mail failure: the new student is added and the Left student is still removed');
  t_eq(t_n('select count(*) from dh_student_course where sc_course = :c and sc_applicant = :a', array(':c' => $course, ':a' => $left)), 0, 'mail failure: the Left course record is gone');
  t_ok((bool) t_date($m), 'mail failure: the claimed date stays (nobody gets it twice)');
  $w = db_query("select variables from watchdog where wid > :w and type = 'Finalization Error' order by wid desc limit 1", array(':w' => $wid))->fetchField();
  t_ok($w && strpos($w, "i:$m;") !== FALSE && strpos($w, "i:$course;") !== FALSE, 'mail failure: logged as Finalization Error with the applicant and course ids');
  dh_course_sync($course, array('mail_fn' => 't_throw_mail', 'files' => 'none'));
  t_eq($GLOBALS['t_mail'], array($m), 'mail failure: a later run does not try again');
});

echo "Summary text\n";
$sum = array('students' => 5, 'added' => 0, 'updated' => 0, 'removed' => 0, 'copies' => 0, 'students_removed' => 0, 'emails' => 1, 'errors' => 0, 'mail_failed' => 0);
t_eq(dh_course_sync_summary_text($sum), '5 students: 0 added, 1 email sent', 'summary: no failed emails, nothing extra');
t_eq(dh_course_sync_summary_text(array('mail_failed' => 1) + $sum), '5 students: 0 added, 1 email sent, 1 email failed', 'summary: 1 email failed');
t_eq(dh_course_sync_summary_text(array('mail_failed' => 3) + $sum), '5 students: 0 added, 1 email sent, 3 emails failed', 'summary: 3 emails failed');

// C1.2: mark_mailed (the back-fill apply) dates the attended students as emailed without sending anything.
t_in_txn(function () {
  $GLOBALS['t_mail'] = array();
  $ts = t_make_course(array('c_finalized' => 1, 'c_finalized_tstamp' => '2025-03-04 05:06:07'));
  $a = t_make_applicant($ts); $kept = t_make_applicant($ts, array('a_attended_mail' => '2024-04-04 04:04:04'));
  $notatt = t_make_applicant($ts, array('a_attended' => 0));
  dh_course_sync($ts, array('mail' => FALSE, 'mark_mailed' => TRUE, 'mail_fn' => 't_record_mail', 'files' => 'none'));
  t_eq(t_date($a), '2025-03-04 05:06:07', 'mark_mailed: the finalize time is stored');
  t_eq(t_date($kept), '2024-04-04 04:04:04', 'mark_mailed: a stored date is kept');
  t_eq(t_date($notatt), NULL, 'mark_mailed: an applicant who did not attend is not marked');
  t_eq($GLOBALS['t_mail'], array(), 'mark_mailed: the mail function is never called');
  $end = t_make_course(array('c_finalized' => 1, 'c_finalized_tstamp' => NULL));
  $b = t_make_applicant($end);
  dh_course_sync($end, array('mail' => FALSE, 'mark_mailed' => TRUE, 'files' => 'none'));
  t_eq(t_date($b), db_query('select c_end from dh_course where c_id = :c', array(':c' => $end))->fetchField() . ' 00:00:00', 'mark_mailed: no finalize time, so the course end date');
  $off = t_make_course(array('c_finalized' => 1));
  $c = t_make_applicant($off);
  dh_course_sync($off, array('mail' => FALSE, 'files' => 'none'));
  t_eq(t_date($c), NULL, 'without mark_mailed (and no email) the date stays empty');
});

// C1.3: dh_send_letter() dates an Attended letter to an applicant (sent by hand from search or the application, or by finalize).
t_in_txn(function () {
  $course = t_make_course();
  $a = t_make_applicant($course);
  t_eq(dh_attended_mail_note('applicant', $a, 'Confirmed'), 0, 'letter note: another letter does not count');
  t_eq(dh_attended_mail_note('r-teacher', $a, 'Attended'), 0, 'letter note: a letter to a teacher does not count');
  t_eq(t_date($a), NULL, 'letter note: still no date');
  t_eq(dh_attended_mail_note('applicant', $a, 'Attended'), 1, 'letter note: the Attended letter dates the applicant');
  $d = t_date($a);
  t_ok((bool) $d, 'letter note: date stored');
  db_update('dh_applicant')->fields(array('a_attended_mail' => '2025-01-01 00:00:00'))->condition('a_id', $a)->execute();
  t_eq(dh_attended_mail_note('applicant', $a, 'Attended'), 0, 'letter note: a stored date is never moved');
  t_eq(t_date($a), '2025-01-01 00:00:00', 'letter note: the first date stays');
});
$f = new ReflectionFunction('dh_send_letter');
$src = implode('', array_slice(file($f->getFileName()), $f->getStartLine() - 1, $f->getEndLine() - $f->getStartLine() + 1));
t_ok(preg_match('/dh_attended_mail_note\(\$type, \$id, \$event\);\s*return \$ret_val;\s*\}\s*$/', $src) === 1, 'dh_send_letter() calls the note after it sent, just before its last return');

// C1.1: the deploy step dates the attended students of courses finalized before this change.
t_in_txn(function () {
  $start = t_make_course(array('c_finalized' => 1, 'c_finalized_tstamp' => '2025-06-07 08:09:10'));
  $first = t_make_applicant($start);                                                   // every made-up applicant from here on has a higher a_id
  $sevak = t_make_applicant($start, array('a_type' => 'Sevak'));
  $teacher = t_make_applicant($start, array('a_type' => 'Teacher'));
  $notatt = t_make_applicant($start, array('a_attended' => 0));
  $kept = t_make_applicant($start, array('a_attended_mail' => '2024-02-02 02:02:02'));
  $old = t_make_course(array('c_finalized' => 1, 'c_finalized_tstamp' => NULL));        // finalized long ago: no finalize time, has records
  $o = t_make_applicant($old);
  db_insert('dh_student_course')->fields(array('sc_student' => t_make_student(), 'sc_center' => 1, 'sc_course' => $old))->execute();
  $pending = t_make_course(array('c_finalized' => 1, 'c_processed' => 0, 'c_finalized_tstamp' => NULL, 'c_finalize_requested' => 1));   // first finalize still waiting
  $p = t_make_applicant($pending);
  $again = t_make_course(array('c_finalized' => 1, 'c_processed' => 0, 'c_finalized_tstamp' => NULL));    // waiting, but it ran before (has records)
  $g = t_make_applicant($again);
  db_insert('dh_student_course')->fields(array('sc_student' => t_make_student(), 'sc_center' => 1, 'sc_course' => $again))->execute();
  $open = t_make_course(array('c_finalized' => 0));
  $x = t_make_applicant($open);
  $n = dh_stamp_finalized_attended_mail($first);
  t_eq($n, 4, 'stamp: 4 applicants marked');
  t_eq(array(t_date($first), t_date($sevak)), array('2025-06-07 08:09:10', '2025-06-07 08:09:10'), 'stamp: students and servers get the finalize time');
  t_eq(array(t_date($teacher), t_date($notatt)), array(NULL, NULL), 'stamp: other types and applicants who did not attend are left alone');
  t_eq(t_date($kept), '2024-02-02 02:02:02', 'stamp: a stored date is kept');
  t_eq(t_date($o), db_query('select c_end from dh_course where c_id = :c', array(':c' => $old))->fetchField() . ' 00:00:00', 'stamp: no finalize time, so the course end date');
  t_eq(t_date($p), NULL, 'stamp: a first finalize that is still waiting is left alone (its students get the email)');
  t_eq(t_date($g), db_query('select c_end from dh_course where c_id = :c', array(':c' => $again))->fetchField() . ' 00:00:00', 'stamp: a waiting course that has records is marked');
  t_eq(t_date($x), NULL, 'stamp: a course that is not finalized is left alone');
  t_eq(dh_stamp_finalized_attended_mail($first), 0, 'stamp: a second run marks nothing');
});
t_done();
