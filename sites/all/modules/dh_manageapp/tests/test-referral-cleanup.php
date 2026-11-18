<?php
// Step 2 cleanup of old referral copies (inc/referral-cleanup.inc): merge into the real record, remove non-students, keep
// the only record, and undo of the whole batch.
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-referral-cleanup.php
require __DIR__ . '/lib.php';
require_once DRUPAL_ROOT . '/sites/all/modules/dh_manageapp/inc/referral-cleanup.inc';

t_in_txn(function () {
  $cA = t_make_course(); $cB = t_make_course();
  $centre = db_query('select c_center from dh_course where c_id = :c', array(':c' => $cA))->fetchField();
  $course = function ($s, $c) use ($centre) { return (int) db_insert('dh_student_course')->fields(array('sc_student' => $s, 'sc_course' => $c, 'sc_center' => $centre))->execute(); };
  $open = function ($s, $r = 0, $g = 'twin') { db_insert('dh_referral_copy')->fields(array('rc_student' => $s, 'rc_referral' => $r, 'rc_group' => $g, 'rc_status' => 'open', 'rc_created' => date('Y-m-d H:i:s')))->execute(); };
  $real = t_make_student(array('s_f_name' => 'Zzqtest', 's_l_name' => 'Cleanreal', 's_email' => 'zzqclean@example.invalid', 's_phone_home' => ''));
  $course($real, $cA);
  $c1 = t_make_student(array('s_f_name' => 'Zzqtest', 's_l_name' => 'Cleanreal', 's_email' => 'zzqclean@example.invalid', 's_phone_home' => '0221234567'));
  $sc1 = $course($c1, $cB);
  $dup = $course($c1, $cA);                                  // the same course is already on the real record
  db_insert('dh_student_course_input')->fields(array('sci_student' => $c1, 'sci_10d' => 5))->execute();
  $ref = (int) db_insert('dh_referral')->fields(array('r_student' => $real, 'r_center' => 0, 'r_reason' => 'zzq', 'r_listed_by' => 'zzq', 'r_start' => date('Y-m-d'),
    'r_end' => date('Y-m-d', strtotime('+30 days')), 'r_created_by' => 0, 'r_updated_by' => 0, 'r_moved' => date('Y-m-d H:i:s')))->execute();
  $open($c1, $ref);
  $c2 = t_make_student(array('s_f_name' => 'Zzqtest', 's_l_name' => 'Cleannone'));
  $open($c2, 0, 'none');
  $c3 = t_make_student(array('s_f_name' => 'Zzqtest', 's_l_name' => 'Cleanonly', 's_email' => 'zzqonly@example.invalid'));
  $course($c3, $cB);
  $open($c3, 0, 'none');
  $only = array($c1, $c2, $c3);

  t_eq(array(dh_refclean_plan($c1)['do'], dh_refclean_plan($c1)['into'], dh_refclean_plan($c2)['do'], dh_refclean_plan($c3)['do']),
    array('merge', $real, 'remove', 'keep'), 'P1 plans: merge into the real record, remove a non-student, keep an only record');
  $dry = dh_refclean_run(FALSE, $only);
  t_eq(array($dry['merge'], $dry['remove'], $dry['keep'], $dry['batch']), array(1, 1, 1, ''), 'P2 the dry run counts and writes nothing');
  t_ok((bool) db_query('select 1 from dh_student where s_id = :s', array(':s' => $c2))->fetchField(), 'P3 nothing removed by the dry run');

  $sum = dh_refclean_run(TRUE, $only);
  $b = $sum['batch'];
  t_eq(array($sum['merged'], $sum['removed'], $sum['errors']), array(1, 1, 0), 'A1 one merged, one removed, no errors');
  t_eq(db_query('select sc_student from dh_student_course where sc_id = :i', array(':i' => $sc1))->fetchField(), $real, 'A2 the copy\'s course moved to the real record');
  t_eq(db_query('select count(*) from dh_student_course where sc_id = :i', array(':i' => $dup))->fetchField(), 0, 'A3 a duplicate of a course the real record has is dropped');
  t_eq(db_query('select s_phone_home from dh_student where s_id = :s', array(':s' => $real))->fetchField(), '0221234567', 'A4 an empty field of the real record is filled');
  t_eq(db_query('select sci_10d from dh_student_course_input where sci_student = :s', array(':s' => $real))->fetchField(), 5, 'A5 the real record had no course counts: it takes the copy\'s');
  t_eq(array((int) db_query('select count(*) from dh_student where s_id in (:s)', array(':s' => array($c1, $c2)))->fetchField(), (int) db_query('select count(*) from dh_student where s_id = :s', array(':s' => $c3))->fetchField()),
    array(0, 1), 'A6 the merged and the non-student copies are gone; the only record stays');
  t_eq(db_query('select rc_student, rc_status from dh_referral_copy where rc_student in (:s) order by rc_student', array(':s' => $only))->fetchAllKeyed(),
    array($c1 => 'merged', $c2 => 'removed', $c3 => 'kept'), 'A7 the copy list records the outcome');

  $u = dh_refclean_undo($b);
  t_eq($u['_failed'], 0, 'U1 undo without failures');
  t_eq(array((int) db_query('select count(*) from dh_student where s_id in (:s)', array(':s' => array($c1, $c2)))->fetchField(),
    db_query('select sc_student from dh_student_course where sc_id = :i', array(':i' => $sc1))->fetchField(),
    (int) db_query('select count(*) from dh_student_course where sc_id = :i', array(':i' => $dup))->fetchField(),
    db_query('select s_phone_home from dh_student where s_id = :s', array(':s' => $real))->fetchField()),
    array(2, $c1, 1, ''), 'U2 copies, courses and the filled field are back');
  t_eq(db_query("select count(*) from dh_referral_copy where rc_student in (:s) and rc_status = 'open'", array(':s' => $only))->fetchField(), 3, 'U3 the copies are open again');
});
t_done();
