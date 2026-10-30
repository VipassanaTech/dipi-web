<?php
require __DIR__ . '/lib.php';
echo "Reopen for correction\n";
$manager = (object) array('uid' => 900000001, 'roles' => array(DRUPAL_AUTHENTICATED_RID => 'authenticated user', 4 => 'Centre Admin'));
$admin = (object) array('uid' => 900000002, 'roles' => array(DRUPAL_AUTHENTICATED_RID => 'authenticated user', 3 => 'administrator'));
t_in_txn(function () use ($manager, $admin) {
  $recent = t_make_course(array('c_finalized' => 1, 'c_finalized_tstamp' => date('Y-m-d H:i:s', strtotime('-10 days'))));
  $old = t_make_course(array('c_finalized' => 1, 'c_finalized_tstamp' => date('Y-m-d H:i:s', strtotime('-40 days'))));
  $open = t_make_course();
  $waiting = t_make_course(array('c_finalized' => 1, 'c_finalized_tstamp' => date('Y-m-d H:i:s'), 'c_finalize_requested' => 1, 'c_processed' => 0));
  t_ok(dh_course_can_reopen($recent, $manager) === TRUE, 'manager can reopen within 30 days');
  t_ok(dh_course_can_reopen($old, $manager) !== TRUE, 'manager cannot reopen after 30 days');
  t_ok(dh_course_can_reopen($old, $admin) === TRUE, 'admin can reopen after 30 days');
  t_ok(dh_course_can_reopen($open, $admin) !== TRUE, 'a course that is not finalized cannot be reopened');
  t_ok(dh_course_can_reopen($waiting, $admin) !== TRUE, 'not while a finalize is waiting');

  dh_course_reopen($recent, 'Teacher was not assigned', 1);
  $c = db_query("select c_finalized, c_reopened, c_processed from dh_course where c_id = :c", array(':c' => $recent))->fetchAssoc();
  t_eq($c['c_finalized'], 0, 'reopen clears finalized'); t_ok((bool) $c['c_reopened'], 'reopen time stored'); t_eq($c['c_processed'], 1, 'c_processed untouched (no dhamma.org resend)');
  t_eq(db_query("select crl_new from dh_course_log where crl_course = :c and crl_field = 'reopen_reason'", array(':c' => $recent))->fetchField(), 'Teacher was not assigned', 'reason in the Change Log');
  // A stale request flag (left by the old cron: flag 1, c_processed 1) is cleared by the reopen, c_processed still untouched.
  $stale = t_make_course(array('c_finalized' => 1, 'c_finalized_tstamp' => date('Y-m-d H:i:s'), 'c_finalize_requested' => 1, 'c_processed' => 1));
  t_ok(dh_course_can_reopen($stale, $admin) === TRUE, 'a stale flag does not block the reopen');
  dh_course_reopen($stale, 'Stale flag test', 1);
  t_eq(array_values(db_query("select c_finalize_requested, c_processed from dh_course where c_id = :c", array(':c' => $stale))->fetchAssoc()), array(0, 1), 'reopen clears a stale request flag and leaves c_processed alone');

  // The whole correction cycle.
  $opts = array('mail_fn' => 't_record_mail', 'files' => 'none');
  $course = t_make_course();
  $a = t_make_applicant($course); $b = t_make_applicant($course); t_make_applicant($course);
  dh_course_sync($course, $opts);
  $mailed = count($GLOBALS['t_mail']);
  $sb = db_query("select sc_student from dh_student_course where sc_course = :c and sc_applicant = :a", array(':c' => $course, ':a' => $b))->fetchField();
  db_update('dh_applicant')->fields(array('a_attended' => 0))->condition('a_id', $b)->execute();   // B actually left
  $d = t_make_applicant($course);                                                                    // D was missed
  $t = t_add_teacher($course, 'M', 1);                                                               // teacher was missed
  $s = dh_course_sync($course, $opts);
  t_eq(array($s['added'], $s['removed']), array(1, 1), 'correction: D added, B removed');
  t_eq(db_query("select count(*) from dh_student where s_id = :s", array(':s' => $sb))->fetchField(), 0, "B's student record removed (only course)");
  t_eq(count($GLOBALS['t_mail']) - $mailed, 1, 'only D gets the Attended email');
  t_eq(db_query("select count(*) from dh_student_course where sc_course = :c and sc_teacher = :t", array(':c' => $course, ':t' => $t))->fetchField(), 3, 'teacher on every record');
  $e = t_make_applicant($course);
  $mailed = count($GLOBALS['t_mail']);
  dh_course_sync($course, $opts + array('mail' => FALSE));
  t_eq(count($GLOBALS['t_mail']) - $mailed, 0, 'tick box off: nobody emailed');
});

echo "Course page\n";
// A manager who cannot reopen sees why, in one line, instead of the button.
t_in_txn(function () use ($manager) {
  global $user;
  $saved = $user; $user = $manager; drupal_static_reset('user_access');
  if (!isset($_SERVER['REQUEST_URI'])) $_SERVER['REQUEST_URI'] = '/';
  $page = function ($ago) {
    $c = t_make_course(array('c_finalized' => 1, 'c_finalized_tstamp' => date('Y-m-d H:i:s', strtotime($ago))));
    $_GET['q'] = 'course/' . db_query('select c_center from dh_course where c_id = :c', array(':c' => $c))->fetchField() . '/' . $c;
    drupal_static_reset('arg');
    return dh_course_dashboard();
  };
  $out = $page('-40 days');
  t_ok(strpos($out, '<p class="course-reopen-na">Reopen for correction is not available: This course was finalized more than 30 days ago. Ask a DIPI administrator to reopen it.</p>') !== FALSE
    && strpos($out, 'id="course-reopen"') === FALSE, 'course page: after 30 days the manager sees the reason, not the button');
  $out = $page('-5 days');
  t_ok(strpos($out, 'id="course-reopen"') !== FALSE && strpos($out, 'course-reopen-na') === FALSE, 'course page: within 30 days the manager sees the button');
  $user = $saved; drupal_static_reset('user_access');
});
t_done();
