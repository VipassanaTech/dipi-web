<?php
// Referral lists read the referral's own details (dh_referral_list_sql), one row per referral; tab counts count referrals;
// the expiry cron no longer reads dh_student.
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-referral-lists.php
require __DIR__ . '/lib.php';

t_in_txn(function () {
  $centre = (int) db_query('select min(c_id) from dh_center where c_id > 0')->fetchField();
  $course = t_make_course();
  $ref = function ($over) {
    return (int) db_insert('dh_referral')->fields($over + array('r_student' => 0, 'r_center' => 0, 'r_reason' => 'zzq', 'r_listed_by' => 'zzq',
      'r_start' => date('Y-m-d'), 'r_end' => date('Y-m-d', strtotime('+30 days')), 'r_listed_10d' => 1, 'r_created_by' => 0, 'r_updated_by' => 0, 'r_moved' => date('Y-m-d H:i:s')))->execute();
  };
  $s = t_make_student(array('s_photo' => 'private://zzq/linked.jpg'));
  foreach (array(1, 2) as $i) {
    db_insert('dh_student_course')->fields(array('sc_student' => $s, 'sc_course' => $course, 'sc_center' => $centre))->execute();
    db_insert('dh_student_course_input')->fields(array('sci_student' => $s, 'sci_10d' => 10 * $i))->execute();
  }
  $r1 = $ref(array('r_center' => $centre, 'r_f_name' => 'Zzqtest', 'r_l_name' => 'Listone', 'r_cnt_10d' => 2, 'r_photo' => 'private://zzq/own.jpg', 'r_student' => $s));
  $r2 = $ref(array('r_center' => $centre, 'r_f_name' => 'Zzqtest', 'r_l_name' => 'Listtwo', 'r_photo' => 'private://zzq/own2.jpg', 'r_student' => $s));
  $r3 = $ref(array('r_center' => $centre, 'r_f_name' => 'Zzqtest', 'r_l_name' => 'Listthree', 'r_photo' => 'private://zzq/own3.jpg'));
  $rows = db_query(dh_referral_list_sql('r.r_id in (:ids)'), array(':ids' => array($r1, $r2, $r3)))->fetchAllAssoc('r_id', PDO::FETCH_ASSOC);
  t_eq(count($rows), 3, 'L1 one row per referral, even with a linked student that has 2 courses and 2 count rows');
  t_eq(array($rows[$r1]['Full Name'], $rows[$r1]['sci_10d']), array('Zzqtest Listone', 2), 'L2 name and counts come from the referral');
  t_eq(array($rows[$r1]['s_photo'], $rows[$r3]['s_photo']), array('private://zzq/linked.jpg', 'private://zzq/own3.jpg'), 'L3 photo: the linked student\'s, else the referral\'s own');
  t_eq((int) $rows[$r2]['r_student'], $s, 'L4 rows carry the link');
  $n = dh_centre_referral_counts($centre);
  t_ok($n['active'] >= 3, 'C1 the counts include referrals that share one linked student');
  $before = $n['active'];
  $ref(array('r_center' => $centre, 'r_f_name' => 'Zzqtest', 'r_l_name' => 'Listfour', 'r_student' => $s));
  t_eq(dh_centre_referral_counts($centre)['active'], $before + 1, 'C2 a referral is counted, not a person record');
});
t_ok(strpos(file_get_contents(DRUPAL_ROOT . '/cron-referral.php'), 'dh_student') === FALSE, 'X1 the expiry cron no longer reads dh_student');
t_done();
