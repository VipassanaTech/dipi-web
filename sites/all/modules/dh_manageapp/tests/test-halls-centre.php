<?php
// Saving a centre's Dhamma Halls (inc/halls.inc dh_halls_centre_save) and its Change Log lines.
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-halls-centre.php
require __DIR__ . '/lib.php';

function hc_row($id, $code, $name, $remove = FALSE) { return array('id' => $id, 'code' => $code, 'name' => $name, 'remove' => $remove); }
function hc_log($c) { return db_query("select cl_action, cl_field, cl_old, cl_new from dh_center_log where cl_center = :c and cl_area = 'halls' order by cl_id", array(':c' => $c))->fetchAll(PDO::FETCH_ASSOC); }
function hc_id($c, $code) { return (int) db_query("select ch_id from dh_center_hall where ch_center = :c and ch_code = :k and ch_deleted = 0", array(':c' => $c, ':k' => $code))->fetchField(); }

t_in_txn(function () {
  $c = t_make_centre();
  t_eq(dh_halls_centre_save($c, array(hc_row(0, 'DH1', 'Dhamma Hall 1'), hc_row(0, 'DH2', 'Dhamma Hall 2'), hc_row(0, '', '')), array(), 1), array(), 'C1 two halls added');
  dh_halls_reset();
  t_eq(count(dh_halls_list($c)), 2, 'C1 two halls saved, empty row skipped');
  $log = hc_log($c);
  t_eq(count($log), 2, 'C1 two Change Log lines');
  t_eq(array($log[0]['cl_action'], $log[0]['cl_field'], $log[0]['cl_old'], $log[0]['cl_new']), array('create', 'hall', NULL, 'DH1 Dhamma Hall 1'), 'C1 log line: hall added');

  $h1 = hc_id($c, 'DH1'); $h2 = hc_id($c, 'DH2');
  t_eq(dh_halls_centre_validate($c, array(hc_row($h1, 'DH 1', 'X'), hc_row($h2, 'DH2', 'Y')), array()), array('Hall code must be 1 to 6 letters or digits: DH 1'), 'C2 code with a space refused');
  t_eq(dh_halls_centre_validate($c, array(hc_row($h1, 'DH1', str_repeat('a', 41)), hc_row($h2, 'DH2', 'Y')), array()), array('Hall name must be 1 to 40 characters: DH1'), 'C2 name too long refused');
  t_eq(dh_halls_centre_validate($c, array(hc_row($h1, 'DH1', 'A'), hc_row($h2, 'dh1', 'B')), array()), array('Hall code DH1 is used twice.'), 'C2 repeated code refused (any case)');
  t_eq(dh_halls_centre_validate($c, array(hc_row($h1, 'DH1', 'A'), hc_row($h2, 'DH2', 'B')), array(3 => 'XX')), array('Group 3: choose one of this centre\'s halls.'), 'C2 unknown usual hall refused');
  t_eq(dh_halls_centre_validate($c, array(hc_row(999999999, 'DH9', 'Z')), array()), array('This hall is not part of this centre.'), 'C2 a hall id of another centre refused');

  t_eq(dh_halls_centre_save($c, array(hc_row($h1, 'DH1', 'Dhamma Hall 1'), hc_row($h2, 'MH', 'Mini Hall')), array(3 => 'MH'), 1), array(), 'C3 rename + usual hall saved');
  dh_halls_reset();
  t_eq(dh_halls_usual($c)[3], $h2, 'C3 group 3 usual hall is MH');
  $log = hc_log($c);
  $last2 = array_slice($log, -2);
  t_eq(array($last2[0]['cl_field'], $last2[0]['cl_old'], $last2[0]['cl_new']), array('hall', 'DH2 Dhamma Hall 2', 'MH Mini Hall'), 'C3 log line: hall renamed');
  t_eq(array($last2[1]['cl_field'], $last2[1]['cl_old'], $last2[1]['cl_new']), array('hall_group', 'Group 3: DH1', 'Group 3: MH'), 'C3 log line: usual hall changed');

  t_eq(dh_halls_centre_validate($c, array(hc_row($h1, 'DH1', 'Dhamma Hall 1'), hc_row($h2, 'MH', 'Mini Hall', TRUE)), array()), array('Hall MH is still the usual hall of Group 3. Move that group first.'), 'C4 remove refused while a usual hall');

  $k = t_make_course(array('c_center' => $c, 'c_combined_seat_course' => 0, 'c_start' => date('Y-m-d', strtotime('+5 days')), 'c_end' => date('Y-m-d', strtotime('+15 days'))));
  db_insert('dh_course_hall_group')->fields(array('cg_course' => $k, 'cg_group' => 1, 'cg_hall' => $h2))->execute();
  t_eq(dh_halls_centre_validate($c, array(hc_row($h1, 'DH1', 'Dhamma Hall 1'), hc_row($h2, 'MH', 'Mini Hall', TRUE)), array(3 => 'DH1')), array('Hall MH is used by course ' . $k . '. Change that course first.'), 'C5 remove refused while a coming course uses it');

  db_delete('dh_course_hall_group')->condition('cg_course', $k)->execute();
  t_eq(dh_halls_centre_save($c, array(hc_row($h1, 'DH1', 'Dhamma Hall 1'), hc_row($h2, 'MH', 'Mini Hall', TRUE)), array(3 => 'DH1'), 1), array(), 'C6 remove allowed once unused');
  dh_halls_reset();
  t_eq(count(dh_halls_list($c)), 1, 'C6 one hall left');
  t_eq((int) db_query("select ch_deleted from dh_center_hall where ch_id = :h", array(':h' => $h2))->fetchField(), 1, 'C6 hall kept, marked removed');
  $log = hc_log($c);
  $last = end($log);
  t_eq(array($last['cl_action'], $last['cl_old'], $last['cl_new']), array('delete', 'MH Mini Hall', NULL), 'C6 log line: hall removed');
});
t_done();
