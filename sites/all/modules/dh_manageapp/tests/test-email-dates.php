<?php
require __DIR__ . '/lib.php';
echo "Past Attended-email dates\n";
t_in_txn(function () {
  $course = t_make_course();
  $got = t_make_applicant($course); $none = t_make_applicant($course);
  $kept = t_make_applicant($course, array('a_attended_mail' => '2025-05-05 05:05:05'));
  $twice = t_make_applicant($course); $far = t_make_applicant($course);
  $line = function ($id, $at, $extra = array()) {
    db_insert('dh_log')->fields($extra + array('l_user' => 0, 'l_center' => 1, 'l_module' => 'Applicant', 'l_event' => 'Letter', 'l_tstamp' => $at,
      'l_identifier' => $id, 'l_msg' => 'Letter sent (Email: zzq@example.invalid) - Attended ( Response: <test> )'))->execute();
  };
  $line($got, '2026-01-02 03:04:05');
  db_insert('dh_log')->fields(array('l_user' => 0, 'l_center' => 1, 'l_module' => 'Applicant', 'l_event' => 'Letter', 'l_tstamp' => '2026-01-02 03:04:05',
    'l_identifier' => $none, 'l_msg' => 'No letter sent, No email or Mobile!'))->execute();
  $line($kept, '2026-01-02 03:04:05');
  $line($twice, '2026-03-03 03:03:03'); $line($twice, '2026-02-02 02:02:02');
  // The log is read in chunks of l_id; two lines for one applicant in different chunks must still give the earlier date.
  $line($far, '2026-03-03 03:03:03');
  $low = (int) db_query('select min(l_id) from dh_log')->fetchField() - 300000;
  $line($far, '2026-02-02 02:02:02', array('l_id' => $low ? $low : -1));
  t_ok(dh_fill_attended_mail_dates() >= 1, 'applicants marked');
  $date = function ($id) { return db_query("select a_attended_mail from dh_applicant where a_id = :a", array(':a' => $id))->fetchField(); };
  t_eq($date($got), '2026-01-02 03:04:05', 'date taken from the letter log');
  t_ok($date($none) === NULL, '"No letter sent" is not an email');
  t_eq($date($kept), '2025-05-05 05:05:05', 'an existing date is never overwritten');
  t_eq($date($twice), '2026-02-02 02:02:02', 'two Attended lines: the earlier date wins');
  t_eq($date($far), '2026-02-02 02:02:02', 'two Attended lines in different log chunks: the earlier date wins');
  t_eq(dh_fill_attended_mail_dates(), 0, 'second run marks nothing new');
});
t_done();
