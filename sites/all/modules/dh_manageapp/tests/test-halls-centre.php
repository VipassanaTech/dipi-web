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

// Edge cases on fresh made-up centres: a refused save writes nothing, usual-hall rename, code reuse, foreign hall.
t_in_txn(function () {
  // C7: a refused save changes nothing
  $c = t_make_centre();
  dh_halls_centre_save($c, array(hc_row(0, 'DH1', 'Dhamma Hall 1'), hc_row(0, 'DH2', 'Dhamma Hall 2')), array(), 1);
  dh_halls_reset();
  $h1 = hc_id($c, 'DH1'); $h2 = hc_id($c, 'DH2');
  $logs_before = count(hc_log($c));
  t_eq(dh_halls_centre_save($c, array(hc_row($h1, 'DH1', 'Renamed'), hc_row($h2, 'DH 9', 'B')), array(), 1), array('Hall code must be 1 to 6 letters or digits: DH 9'), 'C7 bad code refused');
  t_eq((string) db_query("select ch_name from dh_center_hall where ch_id = :h", array(':h' => $h1))->fetchField(), 'Dhamma Hall 1', 'C7 refused save: hall name unchanged');
  t_eq(count(hc_log($c)), $logs_before, 'C7 refused save: no Change Log lines');

  // C8: renaming a group's usual hall writes one hall line, no usual-hall line
  $c = t_make_centre();
  dh_halls_centre_save($c, array(hc_row(0, 'DH1', 'Dhamma Hall 1'), hc_row(0, 'DH2', 'Dhamma Hall 2')), array(), 1);
  dh_halls_reset();
  $h1 = hc_id($c, 'DH1'); $h2 = hc_id($c, 'DH2');
  dh_halls_centre_save($c, array(hc_row($h1, 'DH1', 'Dhamma Hall 1'), hc_row($h2, 'DH2', 'Dhamma Hall 2')), array(2 => 'DH2'), 1);
  dh_halls_reset();
  $logs_before = count(hc_log($c));
  t_eq(dh_halls_centre_save($c, array(hc_row($h1, 'DH1', 'Dhamma Hall 1'), hc_row($h2, 'DH2X', 'Second')), array(2 => 'DH2X'), 1), array(), 'C8 usual hall renamed: saved');
  dh_halls_reset();
  $new = array_slice(hc_log($c), $logs_before);
  t_eq(count($new), 1, 'C8 renaming the usual hall writes exactly one line');
  t_eq(array($new[0]['cl_field'], $new[0]['cl_old'], $new[0]['cl_new']), array('hall', 'DH2 Dhamma Hall 2', 'DH2X Second'), 'C8 the one line is the hall rename');
  t_eq(dh_halls_usual($c)[2], $h2, 'C8 group 2 still on the same hall');

  // C9: a new hall may reuse a removed hall's code
  $c = t_make_centre();
  dh_halls_centre_save($c, array(hc_row(0, 'DH1', 'Dhamma Hall 1'), hc_row(0, 'MH', 'Mini Hall')), array(), 1);
  dh_halls_reset();
  $h1 = hc_id($c, 'DH1'); $hm = hc_id($c, 'MH');
  t_eq(dh_halls_centre_save($c, array(hc_row($h1, 'DH1', 'Dhamma Hall 1'), hc_row($hm, 'MH', 'Mini Hall', TRUE)), array(), 1), array(), 'C9 MH removed');
  dh_halls_reset();
  t_eq(dh_halls_centre_save($c, array(hc_row($h1, 'DH1', 'Dhamma Hall 1'), hc_row(0, 'MH', 'New Mini Hall')), array(), 1), array(), 'C9 new hall reuses code MH');
  dh_halls_reset();
  t_eq(count(dh_halls_list($c)), 2, 'C9 the centre has 2 halls again');

  // C10: a real hall of another centre is refused
  $c = t_make_centre();
  dh_halls_centre_save($c, array(hc_row(0, 'DH1', 'Dhamma Hall 1')), array(), 1);
  dh_halls_reset();
  $other = t_make_centre();
  $foreign = t_make_hall($other, 'XX', 'Other Centre Hall');
  $h1 = hc_id($c, 'DH1');
  t_eq(dh_halls_centre_validate($c, array(hc_row($h1, 'DH1', 'Dhamma Hall 1'), hc_row($foreign, 'XX', 'Other Centre Hall')), array()), array('This hall is not part of this centre.'), 'C10 hall of another centre refused');
  t_eq(dh_halls_centre_save($c, array(hc_row($foreign, 'XX', 'Other Centre Hall')), array(), 1), array('This hall is not part of this centre.'), 'C10 save refused, nothing saved');

  // C11-C13: the usual selects post hall ids
  $c = t_make_centre();
  dh_halls_centre_save($c, array(hc_row(0, 'DH1', 'Dhamma Hall 1'), hc_row(0, 'DH2', 'Dhamma Hall 2')), array(), 1);
  dh_halls_reset();
  $h1 = hc_id($c, 'DH1'); $h2 = hc_id($c, 'DH2');
  dh_halls_centre_save($c, array(hc_row($h1, 'DH1', 'Dhamma Hall 1'), hc_row($h2, 'DH2', 'Dhamma Hall 2')), array(2 => 'DH2'), 1);
  dh_halls_reset();
  $post = array('dhh' => array(array('id' => $h1, 'code' => 'DH1', 'name' => 'Dhamma Hall 1'), array('id' => $h2, 'code' => 'DX2', 'name' => 'Dhamma Hall 2')), 'dhh_usual' => array(2 => (string) $h2));
  $u = dh_halls_form_usual($post);
  t_eq($u, array(2 => 'DX2'), 'C11 usual hall follows the posted (renamed) code');
  t_eq(dh_halls_centre_save($c, dh_halls_form_rows($post), $u, 1), array(), 'C11 rename of a usual hall saves');
  dh_halls_reset();
  t_eq(array(dh_halls_usual($c)[2], (string) db_query("select ch_code from dh_center_hall where ch_id = :h", array(':h' => $h2))->fetchField()), array($h2, 'DX2'), 'C11 group 2 keeps the hall, code is DX2');
  $post = array('dhh' => array(array('id' => $h1, 'code' => 'DH1', 'name' => 'Dhamma Hall 1'), array('id' => $h2, 'code' => 'DX2', 'name' => 'Dhamma Hall 2', 'remove' => 1)), 'dhh_usual' => array(2 => (string) $h2));
  t_eq(dh_halls_centre_validate($c, dh_halls_form_rows($post), dh_halls_form_usual($post)), array('Hall DX2 is still the usual hall of Group 2. Move that group first.'), 'C12 removing a usual hall gives the clear message');
  $post['dhh_usual'] = array(3 => (string) ($h2 + 99999));
  $post['dhh'][1]['remove'] = 0;
  t_eq(dh_halls_centre_validate($c, dh_halls_form_rows($post), dh_halls_form_usual($post)), array('Group 3: choose one of this centre\'s halls.'), 'C13 usual id not among the rows is refused');
});
t_done();
