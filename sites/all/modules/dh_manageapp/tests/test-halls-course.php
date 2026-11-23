<?php
// A course's own halls (inc/halls.inc): save, reset, blocks, the course line, Change Log lines.
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-halls-course.php
require __DIR__ . '/lib.php';

function hk_log($k) { return db_query("select crl_old, crl_new from dh_course_log where crl_course = :k and crl_field = 'halls' order by crl_id", array(':k' => $k))->fetchAll(PDO::FETCH_NUM); }

t_in_txn(function () {
  $c = t_make_centre();
  $k = t_make_course(array('c_center' => $c, 'c_combined_seat_course' => 0, 'c_start' => date('Y-m-d', strtotime('+5 days')), 'c_end' => date('Y-m-d', strtotime('+15 days'))));
  dh_halls_reset();
  t_eq(dh_halls_course_block($k), 'This centre has only one Dhamma Hall.', 'K1 no halls: blocked');
  t_make_hall($c, 'DH1', 'Dhamma Hall 1', 0);
  $h2 = t_make_hall($c, 'DH2', 'Dhamma Hall 2', 1);
  t_make_hall($c, 'DH3', 'Dhamma Hall 3', 2);
  db_insert('dh_center_hall_group')->fields(array('chg_center' => $c, 'chg_group' => 2, 'chg_hall' => $h2))->execute();
  dh_halls_reset();
  t_eq(dh_halls_course_block($k), '', 'K2 three halls: can change');
  t_eq(dh_halls_course_line($k), 'DH1 — Group 1, 3, 4, 5, 6, 7, 8, 9 · DH2 — Group 2 · DH3 — none', 'K2 the course line, hall by hall');

  t_eq(dh_halls_course_save($k, array(2 => 'DH3', 5 => 'DH2'), 1), '', 'K3 save two changes');
  dh_halls_reset();
  t_eq(dh_halls_tail($k, 2, 'code'), ' · DH3', 'K3 group 2 now DH3');
  t_eq(dh_halls_tail($k, 5, 'code'), ' · DH2', 'K3 group 5 now DH2');
  t_eq(hk_log($k), array(array('Group 2: DH2', 'Group 2: DH3'), array('Group 5: DH1', 'Group 5: DH2')), 'K3 two Change Log lines');

  t_eq(dh_halls_course_save($k, array(2 => 'DH2'), 1), '', 'K4 a change back to the usual hall');
  t_eq((int) db_query("select count(*) from dh_course_hall_group where cg_course = :k and cg_group = 2", array(':k' => $k))->fetchField(), 0, 'K4 no row kept for a usual hall');
  t_eq(dh_halls_course_save($k, array(4 => 'XX'), 1), 'Group 4: choose one of this centre\'s halls.', 'K5 unknown hall refused');

  t_eq(dh_halls_course_reset($k, 1), '', 'K6 back to the usual halls');
  dh_halls_reset();
  t_eq(dh_halls_tail($k, 5, 'code'), ' · DH1', 'K6 group 5 back to DH1');
  $log = hk_log($k);
  t_eq(end($log), array('Group 5: DH2', 'Group 5: DH1 (usual)'), 'K6 reset log line');

  db_update('dh_course')->fields(array('c_finalized' => 1))->condition('c_id', $k)->execute();
  t_eq(dh_halls_course_block($k), 'This course is finalized. Its halls cannot be changed.', 'K7 finalized: blocked');
  t_eq(dh_halls_course_save($k, array(1 => 'DH2'), 1), 'This course is finalized. Its halls cannot be changed.', 'K7 finalized: save refused');
  db_update('dh_course')->fields(array('c_finalized' => 0))->condition('c_id', $k)->execute();

  $kb = t_make_course(array('c_center' => $c, 'c_combined_seat_course' => $k, 'c_start' => date('Y-m-d', strtotime('+5 days')), 'c_end' => date('Y-m-d', strtotime('+15 days'))));
  dh_halls_reset();
  t_eq(dh_halls_course_block($kb), 'Halls follow course ' . $k . ' (shared seating plan).', 'K8 shared plan: the other course is blocked');
});
t_done();
