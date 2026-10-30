<?php
require __DIR__ . '/lib.php';
echo "finalize_course()\n";
t_in_txn(function () {
  $future = t_make_course(array('c_end' => date('Y-m-d', strtotime('+3 days'))));
  t_make_applicant($future);
  ob_start(); $r = finalize_course($future); ob_end_clean();
  t_eq($r['status'], FALSE, 'a course that has not ended is refused');

  $course = t_make_course(array('c_finalize_mail' => 0, 'c_finalized' => 1));      // the cron runs finalize only on a finalized course
  $a = t_make_applicant($course); $b = t_make_applicant($course);
  ob_start(); $r = finalize_course($course); ob_end_clean();
  t_eq($r['status'], TRUE, 'finalize runs');
  t_eq($r['summary']['added'], 2, '2 course records'); t_eq($r['summary']['emails'], 0, 'email choice 0: no emails');
  t_eq(db_query("select count(*) from dh_applicant where a_course = :c and a_status = (select td_val1 from dh_type_detail where td_type = 'COURSE-SYSTEM-STATUS' and td_key = 'STATUS-ATTENDED')", array(':c' => $course))->fetchField(), 2, 'status set to Attended');
  t_eq(db_query("select count(*) from dh_applicant where a_id in (:ids) and a_updated_by = :u", array(':ids' => array($a, $b), ':u' => dh_system_uid()))->fetchField(), 2, 'the status change is credited to the System user');
  $log = db_query("select crl_user, crl_new from dh_course_log where crl_course = :c and crl_field = 'finalize_summary'", array(':c' => $course))->fetchAssoc();
  t_ok((bool) $log, 'summary in the Change Log');
  t_eq($log['crl_user'], dh_system_uid(), 'Change Log summary is credited to the System user');
  t_ok(strpos((string) $log['crl_new'], '2 students:') === 0, 'Change Log summary starts with "2 students:"');
  // Emails come back as 0 for these applicants either way (no email, no mobile), so prove choice 0 never tried a letter at all.
  t_eq(db_query("select count(*) from dh_log where l_module = 'Applicant' and l_identifier in (:ids) and l_msg like 'No letter sent%'", array(':ids' => array($a, $b)))->fetchField(), 0, 'email choice 0: no letter was even tried');
  t_eq(db_query("select count(*) from dh_applicant where a_id in (:ids) and a_attended_mail is null", array(':ids' => array($a, $b)))->fetchField(), 2, 'email choice 0: no email date stored');

  db_update('dh_course')->fields(array('c_finalize_mail' => 1))->condition('c_id', $course)->execute();
  ob_start(); $r = finalize_course($course); ob_end_clean();
  t_eq(array($r['summary']['added'], $r['summary']['emails']), array(0, 0), 'second run: no copies; no letter could go out');
  t_eq(db_query("select count(*) from dh_log where l_module = 'Applicant' and l_identifier in (:ids) and l_msg like 'No letter sent%'", array(':ids' => array($a, $b)))->fetchField(), 2, 'no letter could be sent (no email or mobile)');
  ob_start(); $r = finalize_course($course); ob_end_clean();
  t_eq(db_query("select count(*) from dh_log where l_module = 'Applicant' and l_identifier in (:ids) and l_msg like 'No letter sent%'", array(':ids' => array($a, $b)))->fetchField(), 2, 'third run: nobody is tried again');
});
t_done();
