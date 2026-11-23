<?php
// Dhamma Halls data helpers (inc/halls.inc): one-hall rule, fallbacks, shared seating plan.
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-halls-data.php
require __DIR__ . '/lib.php';

t_in_txn(function () {
  $c = t_make_centre();
  $k = t_make_course(array('c_center' => $c, 'c_combined_seat_course' => 0));
  dh_halls_reset();
  t_eq(dh_halls_on($c), FALSE, 'D1 no halls: feature off');
  t_eq(dh_halls_tail($k, 2, 'code'), '', 'D1 no halls: no tail');
  t_eq(dh_halls_js_codes($k), array(), 'D1 no halls: no codes for JS');

  t_make_hall($c, 'DH1', 'Dhamma Hall 1', 0);
  dh_halls_reset();
  t_eq(dh_halls_on($c), FALSE, 'D2 one hall: feature still off');
  t_eq(dh_halls_tail($k, 2, 'name'), '', 'D2 one hall: no tail');
  t_eq(dh_halls_course_map($k), array(), 'D2 one hall: empty map');

  $h2 = t_make_hall($c, 'DH2', 'Dhamma Hall 2', 1);
  dh_halls_reset();
  t_eq(dh_halls_on($c), TRUE, 'D3 two halls: feature on');
  t_eq(dh_halls_tail($k, 2, 'code'), ' · DH1', 'D3 no usual hall set: first hall (code)');
  t_eq(dh_halls_tail($k, '2', 'name'), ' · Dhamma Hall 1', 'D3 no usual hall set: first hall (name)');
  t_eq(dh_halls_tail($k, ' 2 ', 'code'), ' · DH1', 'D3 group with spaces still found');
  t_eq(dh_halls_tail($k, '', 'code'), '', 'D3 No Group: no tail');
  t_eq(dh_halls_tail($k, 0, 'code'), '', 'D3 group 0: no tail');
  t_eq(dh_halls_tail($k, 10, 'code'), '', 'D3 group 10: no tail');

  db_insert('dh_center_hall_group')->fields(array('chg_center' => $c, 'chg_group' => 2, 'chg_hall' => $h2))->execute();
  dh_halls_reset();
  t_eq(dh_halls_tail($k, 2, 'code'), ' · DH2', 'D4 usual hall used');
  t_eq(dh_halls_tail($k, 3, 'code'), ' · DH1', 'D4 other groups: first hall');

  $h1 = (int) db_query("select ch_id from dh_center_hall where ch_center = :c and ch_code = 'DH1'", array(':c' => $c))->fetchField();
  db_insert('dh_course_hall_group')->fields(array('cg_course' => $k, 'cg_group' => 2, 'cg_hall' => $h1))->execute();
  dh_halls_reset();
  t_eq(dh_halls_tail($k, 2, 'code'), ' · DH1', 'D5 the course own change wins');
  t_eq(dh_halls_js_codes($k), array('1' => 'DH1', '2' => 'DH1', '3' => 'DH1', '4' => 'DH1', '5' => 'DH1', '6' => 'DH1', '7' => 'DH1', '8' => 'DH1', '9' => 'DH1'), 'D5 codes for JS, all nine groups');

  $h3 = t_make_hall($c, 'DH3', 'Old Hall', 2, 1);   // removed hall
  db_update('dh_course_hall_group')->fields(array('cg_hall' => $h3))->condition('cg_course', $k)->condition('cg_group', 2)->execute();
  dh_halls_reset();
  t_eq(dh_halls_tail($k, 2, 'code'), ' · DH2', 'D6 change to a removed hall: back to the usual hall');
  db_update('dh_center_hall_group')->fields(array('chg_hall' => $h3))->condition('chg_center', $c)->condition('chg_group', 2)->execute();
  dh_halls_reset();
  t_eq(dh_halls_tail($k, 2, 'code'), ' · DH1', 'D7 usual hall removed: first hall');

  // Shared seating plan: course B points to course A; B follows A's halls.
  $kb = t_make_course(array('c_center' => $c, 'c_combined_seat_course' => $k));
  db_update('dh_course_hall_group')->fields(array('cg_hall' => $h2))->condition('cg_course', $k)->condition('cg_group', 2)->execute();
  dh_halls_reset();
  t_eq(dh_halls_owner_course($kb), $k, 'D8 shared plan: owner is the course the other points to');
  t_eq(dh_halls_owner_course($k), $k, 'D8 owner course is its own owner');
  t_eq(dh_halls_tail($kb, 2, 'code'), ' · DH2', 'D8 shared plan: the other course follows the owner');
});
t_done();
