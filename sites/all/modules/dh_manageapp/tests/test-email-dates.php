<?php
require __DIR__ . '/lib.php';
echo "Past Attended-email dates\n";
t_in_txn(function () {
  $course = t_make_course();
  $got = t_make_applicant($course); $none = t_make_applicant($course);
  $log = array('l_user' => 0, 'l_center' => 1, 'l_module' => 'Applicant', 'l_event' => 'Letter', 'l_tstamp' => '2026-01-02 03:04:05');
  db_insert('dh_log')->fields($log + array('l_identifier' => $got, 'l_msg' => 'Letter sent (Email: zzq@example.invalid) - Attended ( Response: <test> )'))->execute();
  db_insert('dh_log')->fields($log + array('l_identifier' => $none, 'l_msg' => 'No letter sent, No email or Mobile!'))->execute();
  t_ok(dh_fill_attended_mail_dates() >= 1, 'applicants marked');
  t_eq(db_query("select a_attended_mail from dh_applicant where a_id = :a", array(':a' => $got))->fetchField(), '2026-01-02 03:04:05', 'date taken from the letter log');
  t_ok(db_query("select a_attended_mail from dh_applicant where a_id = :a", array(':a' => $none))->fetchField() === NULL, '"No letter sent" is not an email');
  t_eq(dh_fill_attended_mail_dates(), 0, 'second run marks nothing new');
});
t_done();
