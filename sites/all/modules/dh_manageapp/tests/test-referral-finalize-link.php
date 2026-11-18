<?php
// Course finalize links a flagged, unlinked referral to the student it chose (dh_referral_link_from_finalize),
// and never matches an open referral copy.
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-referral-finalize-link.php
require __DIR__ . '/lib.php';

t_in_txn(function () {
  $course = t_make_course();
  $ref = function ($over) {
    return (int) db_insert('dh_referral')->fields($over + array('r_student' => 0, 'r_center' => 0, 'r_reason' => 'zzq', 'r_listed_by' => 'zzq',
      'r_start' => date('Y-m-d', strtotime('-30 days')), 'r_end' => date('Y-m-d', strtotime('+30 days')), 'r_listed_10d' => 1, 'r_created_by' => 0, 'r_updated_by' => 0, 'r_moved' => date('Y-m-d H:i:s')))->execute();
  };
  $r1 = $ref(array('r_f_name' => 'Zzqtest', 'r_l_name' => 'Finone', 'r_email' => 'zzqfin1@example.invalid'));
  $a1 = t_make_applicant($course, array('a_f_name' => 'Zzqtest', 'a_l_name' => 'Finone', 'a_email' => 'zzqfin1@example.invalid', 'a_referral' => $r1));
  $s2 = t_make_student(array('s_f_name' => 'Zzqtest', 's_l_name' => 'Fintwo-other'));
  $r2 = $ref(array('r_f_name' => 'Zzqtest', 'r_l_name' => 'Fintwo', 'r_email' => 'zzqfin2@example.invalid', 'r_student' => $s2));
  $a2 = t_make_applicant($course, array('a_f_name' => 'Zzqtest', 'a_l_name' => 'Fintwo', 'a_email' => 'zzqfin2@example.invalid', 'a_referral' => $r2));
  $r3 = $ref(array('r_f_name' => 'Zzqtest', 'r_l_name' => 'Finthree', 'r_email' => 'zzqfin3-old@example.invalid'));
  $a3 = t_make_applicant($course, array('a_f_name' => 'Zzqtest', 'a_l_name' => 'Finthree', 'a_email' => 'zzqfin3@example.invalid', 'a_referral' => $r3));
  $copy = t_make_student(array('s_f_name' => 'Zzqtest', 's_l_name' => 'Finfour', 's_email' => 'zzqfin4@example.invalid'));
  db_insert('dh_referral_copy')->fields(array('rc_student' => $copy, 'rc_referral' => 0, 'rc_group' => 'none', 'rc_status' => 'open', 'rc_created' => date('Y-m-d H:i:s')))->execute();
  $a4 = t_make_applicant($course, array('a_f_name' => 'Zzqtest', 'a_l_name' => 'Finfour', 'a_email' => 'zzqfin4@example.invalid'));

  $sum = dh_course_sync($course, array('mail_fn' => 't_record_mail', 'files' => 'none'));
  t_eq($sum['errors'], 0, 'F0 the sync ran without errors');
  $student = function ($a) { return (int) db_query('select sc_student from dh_student_course where sc_applicant = :a', array(':a' => $a))->fetchField(); };
  $link = function ($r) { return (int) db_query('select r_student from dh_referral where r_id = :r', array(':r' => $r))->fetchField(); };
  t_ok($student($a1) > 0 && $link($r1) === $student($a1), 'F1 an unlinked referral is linked to the student finalize chose');
  t_eq(db_query("select rl_msg from dh_referral_log where rl_referral = :r and rl_event = 'Link'", array(':r' => $r1))->fetchField(),
    'Linked to student ' . $student($a1) . " (finalize of course $course)", 'F2 the link is logged');
  t_eq($link($r2), $s2, 'F3 a linked referral keeps its link');
  t_eq($link($r3), 0, 'F4 a referral whose typed details no longer match the application is not linked');
  t_ok($student($a4) > 0 && $student($a4) !== $copy, 'F5 finalize never matches an open referral copy');
  // Back-fill batch: no referral linking (link writes are not in the student log, so an undo could not reverse them).
  $course2 = t_make_course();
  $r7 = $ref(array('r_f_name' => 'Zzqtest', 'r_l_name' => 'Finseven', 'r_email' => 'zzqfin7@example.invalid'));
  $a7 = t_make_applicant($course2, array('a_f_name' => 'Zzqtest', 'a_l_name' => 'Finseven', 'a_email' => 'zzqfin7@example.invalid', 'a_referral' => $r7));
  $sum2 = dh_course_sync($course2, array('mail_fn' => 't_record_mail', 'files' => 'none', 'batch' => 'zzq-batch-link', 'uid' => 0));
  t_eq(array($sum2['errors'], $link($r7)), array(0, 0), 'F7 inside a back-fill batch an unlinked flagged referral stays unlinked');
  t_eq(dh_referral_link_from_finalize(999999999, 1, array('a_id' => 1, 'a_course' => 1)), FALSE, 'F6 an unknown referral: FALSE, no exception');
});
t_done();
