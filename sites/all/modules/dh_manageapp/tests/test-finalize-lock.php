<?php
require __DIR__ . '/lib.php';
echo "One finalize per course at a time\n";
// Another process holds the course's finalize lock (a semaphore row with someone else's lock id).
function t_hold($course) {
  db_insert('semaphore')->fields(array('name' => 'dh_finalize_' . $course, 'value' => 'test-other-process', 'expire' => microtime(TRUE) + 600))->execute();
}
function t_n($sql, $args = array()) { return (int) db_query($sql, $args)->fetchField(); }
$admin = (object) array('uid' => 900000002, 'roles' => array(DRUPAL_AUTHENTICATED_RID => 'authenticated user', 3 => 'administrator'));

t_in_txn(function () {
  t_eq(dh_finalize_lock_name(123), 'dh_finalize_123', 'the lock is named after the course');

  // The lock is held: finalize gives up, asks the cron to try again, and changes nothing.
  $course = t_make_course(array('c_finalize_mail' => 0));
  $a = t_make_applicant($course, array('a_status' => 'Confirmed'));
  t_hold($course);
  $wid = t_n('select max(wid) from watchdog');
  ob_start(); $r = finalize_course($course, 0); ob_end_clean();
  t_eq($r['status'], FALSE, 'lock held, wait 0: finalize returns a failed status');
  $c = db_query('select c_finalize_requested, c_processed from dh_course where c_id = :c', array(':c' => $course))->fetchAssoc();
  t_eq(array($c['c_finalize_requested'], $c['c_processed']), array(1, 0), 'lock held: the request is set again for the cron (flag 1, c_processed 0)');
  t_eq(t_n('select count(*) from dh_student_course where sc_course = :c', array(':c' => $course)), 0, 'lock held: no student record written');
  t_eq(db_query('select a_status from dh_applicant where a_id = :a', array(':a' => $a))->fetchField(), 'Confirmed', 'lock held: the status is not changed');
  t_eq(t_n("select count(*) from watchdog where wid > :w and type = 'Finalization Error' and severity = :s and variables like :v",
    array(':w' => $wid, ':s' => WATCHDOG_ERROR, ':v' => '%i:' . $course . ';%')), 1, 'lock held: an error is logged for the course');
  t_eq(db_query('select value from semaphore where name = :n', array(':n' => 'dh_finalize_' . $course))->fetchField(), 'test-other-process', 'lock held: the other lock is left alone');

  // A short wait: it really waits, then gives up.
  $t0 = microtime(TRUE);
  ob_start(); $r = finalize_course($course, 1); ob_end_clean();
  $took = microtime(TRUE) - $t0;
  t_ok($r['status'] === FALSE && $took >= 0.9 && $took < 10, 'lock held, wait 1 s: it waits about a second, then fails');

  // The lock is free: finalize runs and lets the lock go afterwards.
  db_delete('semaphore')->condition('name', 'dh_finalize_' . $course)->execute();
  ob_start(); $r = finalize_course($course, 0); ob_end_clean();
  t_eq(array($r['status'], $r['summary']['added']), array(TRUE, 1), 'lock free: finalize runs');
  t_ok(lock_may_be_available('dh_finalize_' . $course), 'lock free: the lock is released after the finalize');
});

t_in_txn(function () use ($admin) {
  $now = date('Y-m-d H:i:s');
  $held = t_make_course(array('c_finalized' => 1, 'c_finalized_tstamp' => $now));
  t_hold($held);
  t_eq(dh_course_can_reopen($held, $admin), t('Finalize is still running for this course. Try again in 10 minutes.'), 'reopen: refused while the finalize lock is held');
  $waiting = t_make_course(array('c_finalized' => 1, 'c_finalized_tstamp' => $now, 'c_finalize_requested' => 1, 'c_processed' => 0));
  t_eq(dh_course_can_reopen($waiting, $admin), t('Finalize is still running for this course. Try again in 10 minutes.'), 'reopen: refused while a request waits for the cron (flag 1, c_processed 0)');
  $stuck = t_make_course(array('c_finalized' => 1, 'c_finalized_tstamp' => $now, 'c_finalize_requested' => 1, 'c_processed' => 1));
  t_ok(dh_course_can_reopen($stuck, $admin) === TRUE, 'reopen: a flag left at 1 with c_processed 1 (old cron) does not block');
  $free = t_make_course(array('c_finalized' => 1, 'c_finalized_tstamp' => $now));
  t_ok(dh_course_can_reopen($free, $admin) === TRUE, 'reopen: allowed when nothing runs');
});
t_done();
