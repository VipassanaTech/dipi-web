<?php
require __DIR__ . '/lib.php';
echo "finalize_course()\n";
t_in_txn(function () {
  $future = t_make_course(array('c_end' => date('Y-m-d', strtotime('+3 days'))));
  t_make_applicant($future);
  ob_start(); $r = finalize_course($future); ob_end_clean();
  t_eq($r['status'], FALSE, 'a course that has not ended is refused');

  $course = t_make_course(array('c_finalize_mail' => 0));
  $a = t_make_applicant($course); $b = t_make_applicant($course);
  ob_start(); $r = finalize_course($course); ob_end_clean();
  t_eq($r['status'], TRUE, 'finalize runs');
  t_eq($r['summary']['added'], 2, '2 course records'); t_eq($r['summary']['emails'], 0, 'email choice 0: no emails');
  t_eq(db_query("select count(*) from dh_applicant where a_course = :c and a_status = (select td_val1 from dh_type_detail where td_type = 'COURSE-SYSTEM-STATUS' and td_key = 'STATUS-ATTENDED')", array(':c' => $course))->fetchField(), 2, 'status set to Attended');
  t_ok((bool) db_query("select 1 from dh_course_log where crl_course = :c and crl_field = 'finalize_summary'", array(':c' => $course))->fetchField(), 'summary in the Change Log');

  db_update('dh_course')->fields(array('c_finalize_mail' => 1))->condition('c_id', $course)->execute();
  ob_start(); $r = finalize_course($course); ob_end_clean();
  t_eq(array($r['summary']['added'], $r['summary']['emails']), array(0, 0), 'second run: no copies; no letter could go out');
  t_eq(db_query("select count(*) from dh_log where l_module = 'Applicant' and l_identifier in (:ids) and l_msg like 'No letter sent%'", array(':ids' => array($a, $b)))->fetchField(), 2, 'no letter could be sent (no email or mobile)');
  ob_start(); $r = finalize_course($course); ob_end_clean();
  t_eq(db_query("select count(*) from dh_log where l_module = 'Applicant' and l_identifier in (:ids) and l_msg like 'No letter sent%'", array(':ids' => array($a, $b)))->fetchField(), 2, 'third run: nobody is tried again');
});
t_done();
