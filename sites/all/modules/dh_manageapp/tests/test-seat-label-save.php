<?php
// Saving and reading seat label starts (inc/seat-label-start.inc).
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-seat-label-save.php
require __DIR__ . '/lib.php';

function ls_rows($course) {
  return db_query("select sls_plan, sls_section, sls_grid, sls_conv, sls_start from dh_seat_label_start where sls_course = :c
    order by sls_plan, sls_section, sls_grid", array(':c' => $course))->fetchAll(PDO::FETCH_ASSOC);
}
function ls_conv($centre, $conv) {
  db_query("update dh_center_setting set cs_seat_naming_conv = :v where cs_center = :c", array(':v' => $conv, ':c' => $centre));
}
$centre = (int) db_query("select cs_center from dh_center_setting where cs_center > 0 order by cs_center limit 1")->fetchField();

// Save, read back, update, remove; the group plan; a style change.
t_in_txn(function () use ($centre) {
  ls_conv($centre, 1);
  $c = t_make_course(array('c_center' => $centre, 'c_combined_seat_course' => 0));
  $r = dh_seat_label_start_save($centre, $c, 'main', 'M', array('main' => 'I', 'chowky' => ''), 77);
  t_eq($r['status'], 'OK', 'style 1: main grid from I saved');
  t_eq(ls_rows($c), array(array('sls_plan' => 'main', 'sls_section' => 'M', 'sls_grid' => 'main', 'sls_conv' => 1, 'sls_start' => 8)),
    'one row: main grid offset 8; the empty chowky field saves no row');
  t_eq(dh_seat_label_starts($c, 'main'), array('M' => array('main' => 8, 'chowky' => 0)), 'read back for the main plan');
  t_eq(dh_seat_label_starts($c, 'group'), array(), 'the group plan has its own starts');
  t_eq(dh_seat_label_start_of(dh_seat_label_starts($c, 'main'), 'M', 'main'), 8, 'one value: Male main grid');
  t_eq(dh_seat_label_start_of(dh_seat_label_starts($c, 'main'), 'F', 'main'), 0, 'one value: another section is normal');
  $log = db_query("select l_module, l_event, l_identifier, l_msg, l_user from dh_log order by l_id desc limit 1")->fetchAssoc();
  t_eq($log, array('l_module' => 'Course', 'l_event' => 'Seat Labels', 'l_identifier' => $c,
    'l_msg' => 'Seat labels for Male: main grid starts at I, chowky grid normal', 'l_user' => 77), 'Activity Log line');

  dh_seat_label_start_save($centre, $c, 'main', 'M', array('main' => 'K', 'chowky' => 'B'), 77);
  t_eq(dh_seat_label_starts($c, 'main'), array('M' => array('main' => 10, 'chowky' => 1)), 'both grids updated');
  dh_seat_label_start_save($centre, $c, 'main', 'M', array('main' => '', 'chowky' => 'B'), 77);
  t_eq(dh_seat_label_starts($c, 'main'), array('M' => array('main' => 0, 'chowky' => 1)), 'main grid back to normal');
  t_eq(count(ls_rows($c)), 1, 'its row is removed; only the chowky row is left');

  t_eq(dh_seat_label_start_save($centre, $c, 'group', 'g2_F', array('main' => 'C', 'chowky' => ''), 77)['status'], 'OK', 'group plan, Group 2 - Female');
  t_eq(dh_seat_label_start_save($centre, $c, 'group', 'gUnassigned_M', array('main' => 'D', 'chowky' => ''), 77)['status'], 'OK', 'group plan, no group');
  t_eq(dh_seat_label_starts($c, 'group'), array('g2_F' => array('main' => 2, 'chowky' => 0), 'gUnassigned_M' => array('main' => 3, 'chowky' => 0)), 'group starts read back');

  ls_conv($centre, 2);
  t_eq(dh_seat_label_starts($c, 'main'), array(), 'the centre changed its style: old starts are ignored');
  t_eq(dh_seat_label_start_save($centre, $c, 'main', 'M', array('main' => '7', 'chowky' => ''), 77)['status'], 'OK', 'style 2: column 7 saved');
  t_eq(dh_seat_label_starts($c, 'main'), array('M' => array('main' => 6, 'chowky' => 0)), 'style 2 start read back');
});

// Refusals save nothing.
t_in_txn(function () use ($centre) {
  ls_conv($centre, 1);
  $c = t_make_course(array('c_center' => $centre, 'c_combined_seat_course' => 0));
  $other = (int) db_query("select c_id from dh_center where c_id <> :c and c_id > 0 order by c_id limit 1", array(':c' => $centre))->fetchField();
  foreach (array(
    array($centre, 'main', 'M', array('main' => '7', 'chowky' => ''), 'Main grid: type a letter, for example I. Leave it empty for the normal start.'),
    array($centre, 'main', 'X', array('main' => 'I', 'chowky' => ''), 'Unknown section.'),
    array($centre, 'other', 'M', array('main' => 'I', 'chowky' => ''), 'Unknown section.'),
    array($other, 'main', 'M', array('main' => 'I', 'chowky' => ''), 'This course is not in this centre.'),
  ) as $b)
    t_eq(dh_seat_label_start_save($b[0], $c, $b[1], $b[2], $b[3], 77), array('status' => 'Failed', 'msg' => $b[4]), 'refused: ' . $b[4]);
  t_eq(ls_rows($c), array(), 'nothing saved');
  ls_conv($centre, 0);
  t_eq(dh_seat_label_start_save($centre, $c, 'main', 'F', array('main' => '', 'chowky' => '0'), 77)['msg'],
    'Chowky grid: type a seat number of 1 or more, for example 101. Leave it empty for the normal start.', 'numbered: 0 refused');
});

// Combined seating: one save sets every course drawn on the plan.
t_in_txn(function () use ($centre) {
  ls_conv($centre, 1);
  $b = t_make_course(array('c_center' => $centre, 'c_combined_seat_course' => 0));
  $a = t_make_course(array('c_center' => $centre, 'c_combined_seat_course' => $b));
  dh_seat_label_start_save($centre, $a, 'main', 'F', array('main' => 'I', 'chowky' => ''), 77);
  t_eq(dh_seat_label_starts($a, 'main'), array('F' => array('main' => 8, 'chowky' => 0)), 'combined: the opened course has the start');
  t_eq(dh_seat_label_starts($b, 'main'), array('F' => array('main' => 8, 'chowky' => 0)), 'combined: the course drawn with it has the same start');
  t_eq(dh_seat_label_start_save($centre, $b, 'main', 'F', array('main' => 'K', 'chowky' => ''), 77),
    array('status' => 'Failed', 'msg' => 'This seating plan is shared with course '.$a.'. Change its seat labels from the seating plan of course '.$a.'.'),
    'combined: a save from the course that does not draw both is refused');
  t_eq(array(dh_seat_label_starts($a, 'main'), dh_seat_label_starts($b, 'main')),
    array(array('F' => array('main' => 8, 'chowky' => 0)), array('F' => array('main' => 8, 'chowky' => 0))), 'combined: the refused save changed nothing');
  dh_seat_label_start_save($centre, $a, 'main', 'F', array('main' => '', 'chowky' => ''), 77);
  t_eq(array(dh_seat_label_starts($a, 'main'), dh_seat_label_starts($b, 'main')), array(array(), array()), 'a reset from the course that draws both clears both');
});

// The JSON route: token first.
t_in_txn(function () use ($centre) {
  ls_conv($centre, 1);
  $c = t_make_course(array('c_center' => $centre, 'c_combined_seat_course' => 0));
  t_eq(dh_seat_label_start_request($centre, $c, array('plan' => 'main', 'sec' => 'M', 'main' => 'I', 'chowky' => '')),
    array('status' => 'Failed', 'msg' => 'Session expired. Reload the page and try again.'), 'no token: refused');
  t_eq(dh_seat_label_start_request($centre, $c, array('token' => 'x', 'plan' => 'main', 'sec' => 'M', 'main' => 'I', 'chowky' => '')),
    array('status' => 'Failed', 'msg' => 'Session expired. Reload the page and try again.'), 'wrong token: refused');
  t_eq(ls_rows($c), array(), 'nothing saved without the token');
  $r = dh_seat_label_start_request($centre, $c, array('token' => drupal_get_token('seat-label-start'), 'plan' => 'main', 'sec' => 'M', 'main' => 'I', 'chowky' => ''));
  t_eq($r['status'], 'OK', 'with the page token: saved');
});

$items = dh_manageapp_menu();
$route = isset($items['seat-label-start/%centre_id/%course_id']) ? $items['seat-label-start/%centre_id/%course_id'] : array();
t_eq(isset($route['page callback']) ? $route['page callback'] : '', 'dh_seat_label_start_update', 'route registered');
t_eq(isset($route['access arguments']) ? $route['access arguments'] : array(), array('access zero day'), 'same access as the other seat tools');
t_done();
