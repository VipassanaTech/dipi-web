<?php
// Seat labels change only from a page that draws every course of a plan; section ids.
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-seat-label-combined.php
require __DIR__ . '/lib.php';

$centre = (int) db_query("select cs_center from dh_center_setting where cs_center > 0 order by cs_center limit 1")->fetchField();
function lc_course($centre, $other = 0) {
  return t_make_course(array('c_center' => $centre, 'c_combined_seat_course' => $other));
}
function lc_set($c, $other) {
  db_query("update dh_course set c_combined_seat_course = :o where c_id = :c", array(':o' => $other, ':c' => $c));
}

t_in_txn(function () use ($centre) {
  $solo = lc_course($centre);
  t_eq(dh_seat_label_page_courses($solo), array($solo), 'page courses: an uncombined course draws itself');
  t_eq(dh_seat_label_start_blocker($solo), '', 'blocker: uncombined course has none');

  $b = lc_course($centre);
  $a = lc_course($centre, $b);
  t_eq(dh_seat_label_page_courses($a), array($a, $b), 'page courses: A -> B draws both');
  t_eq(dh_seat_label_page_courses($b), array($b), 'page courses: B (not combined itself) draws only itself');
  t_eq(dh_seat_label_start_blocker($a), '', 'blocker: one-way A -> B, A page is fine');
  $say = 'This seating plan is shared with course '.$a.'. Change its seat labels from the seating plan of course '.$a.'.';
  t_eq(dh_seat_label_start_blocker($b), $say, 'blocker: one-way A -> B, B names A');

  lc_set($b, $a);
  t_eq(dh_seat_label_start_blocker($a), '', 'blocker: mutual pair, A fine');
  t_eq(dh_seat_label_start_blocker($b), '', 'blocker: mutual pair, B fine');
});

// Chain C -> A -> B. plan_courses(A) = A, B, C; plan_courses(C) = A, C.
// A's page draws A, B (not C); C's page draws C, A (not B). So no page draws all
// three: A is blocked, but no single course can be named, so the generic sentence
// is returned. C's plan_courses is only A and C, which its page draws: C is fine.
t_in_txn(function () use ($centre) {
  $b = lc_course($centre);
  $a = lc_course($centre, $b);
  $c = lc_course($centre, $a);
  $all = array($a, $b, $c);
  sort($all);
  t_eq(dh_seat_label_start_blocker($a), 'This seating plan is shared by courses '.implode(', ', $all).'. Its seat labels cannot be changed here.',
    'chain: A page draws A, B but not C -> generic sentence');
  t_eq(dh_seat_label_start_blocker($c), '', 'chain: C page draws C, A, which is all that plan_courses(C) holds -> allowed');
  t_ok(dh_seat_label_start_blocker($b) !== '', 'chain: B is blocked');
});

// The button.
$starts = array();
$note = dh_seat_label_start_button('main', 'M', 'Male', 1, $starts, 'Use course 5 here.');
t_ok(strpos($note, 'class="dh-sls-note no-print"') !== FALSE && strpos($note, 'Use course 5 here.') !== FALSE, 'button: blocked shows the note');
t_ok(strpos($note, 'dh-sls-btn') === FALSE, 'button: blocked has no button');
t_ok(strpos(dh_seat_label_start_button('main', 'M', 'Male', 1, $starts, ''), 'class="dh-sls-btn no-print"') !== FALSE, 'button: normal button');
t_ok(strpos(dh_seat_label_start_button('main', 'M', 'Male', 1, $starts), 'class="dh-sls-btn no-print"') !== FALSE, 'button: default is the normal button');

// Section ids.
t_in_txn(function () use ($centre) {
  $c = lc_course($centre);
  foreach (array('M' => 'main', 'g12_M' => 'group', 'gUnassigned_F' => 'group') as $sec => $plan)
    t_eq(dh_seat_label_start_save($centre, $c, $plan, $sec, array('main' => '', 'chowky' => ''), 77)['status'], 'OK', "section $sec accepted");
  foreach (array('gabc_M', 'g1_X', 'g_M', 'g'.str_repeat('1', 40).'_M') as $sec)
    t_eq(dh_seat_label_start_save($centre, $c, 'group', $sec, array('main' => '', 'chowky' => ''), 77),
      array('status' => 'Failed', 'msg' => 'Unknown section.'), 'section refused: '.substr($sec, 0, 12));
});
t_done();
