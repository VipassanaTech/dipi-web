<?php
// dh_render_seat_grid() with a label start (opts['label_start']), all three styles.
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-seat-label-render.php
require __DIR__ . '/lib.php';

$row = array('a_id' => 101, 'a_old' => 0, 'aa_chowky' => 0, 'aa_chair' => 0, 'aa_backrest' => 0, 'aa_seat_aid' => '',
  'aa_section' => 'A', 'aa_acco' => '1', 'aa_cell' => '', 'Full Name' => 'Zzqtest One', 'Age' => 40);
$grid = array(0 => array(0 => $row));   // one student at the front-left stored seat; 2 rows x 3 columns drawn
// Labels of the draggable list in page order: the back row first, each row from the highest stored column down.
function lr_labels($html) {
  preg_match('/<ul class="sortable-m".*?<\/ul>/s', $html, $ul);
  preg_match_all('/<span class="seatnumber">([^<]*)<\/span>/', $ul[0], $m);
  return $m[1];
}
t_eq(lr_labels(dh_render_seat_grid($grid, 3, 1, LEFT_TO_RIGHT, 'main-div', 1)), array('C2', 'B2', 'A2', 'C1', 'B1', 'A1'), 'style 1, no start');
t_eq(lr_labels(dh_render_seat_grid($grid, 3, 1, LEFT_TO_RIGHT, 'main-div', 1, array('label_start' => 8))), array('K2', 'J2', 'I2', 'K1', 'J1', 'I1'), 'style 1 from I');
t_eq(lr_labels(dh_render_seat_grid($grid, 3, 1, LEFT_TO_RIGHT, 'main-div', 2, array('label_start' => 6))), array('B9', 'B8', 'B7', 'A9', 'A8', 'A7'), 'style 2 from 7');
t_eq(lr_labels(dh_render_seat_grid($grid, 3, 1, LEFT_TO_RIGHT, 'main-div', 0)), array('6', '5', '4', '3', '2', '1'), 'numbered, no start');
t_eq(lr_labels(dh_render_seat_grid($grid, 3, 1, LEFT_TO_RIGHT, 'main-div', 0, array('label_start' => 100))), array('106', '105', '104', '103', '102', '101'), 'numbered from 101');

foreach (array(0, 1, 2) as $conv) {
  $plain = dh_render_seat_grid($grid, 3, 1, LEFT_TO_RIGHT, 'main-div', $conv, array(), 'M', 'Male');
  t_eq(dh_render_seat_grid($grid, 3, 1, LEFT_TO_RIGHT, 'main-div', $conv, array('label_start' => 0), 'M', 'Male'), $plain, "style $conv: start 0 gives the same HTML as no start");
  t_ok(strpos($plain, 'data-label-start') === FALSE, "style $conv: no data-label-start without a start");
}
t_ok(strpos(dh_render_seat_grid($grid, 3, 1, LEFT_TO_RIGHT, 'main-div', 1, array('label_start' => 8), 'M', 'Male'),
  '<div class="main-div" data-section="M" data-section-label="Male" data-label-start="8">') !== FALSE, 'the grid carries its start for the column tools');

// A chowky student: the CW- mark stays outside the label.
$cw = array(0 => array(0 => array_merge($row, array('aa_chowky' => 1))));
t_ok(strpos(dh_render_seat_grid($cw, 1, 0, LEFT_TO_RIGHT, 'chowky-div', 1, array('label_start' => 8)),
  '<span class="cwch">CW-</span><span class="seatnumber">I1</span>') !== FALSE, 'chowky seat: CW- then I1');

// The saved label (made-up student, rolled back).
t_in_txn(function () use ($row) {
  $c = t_make_course(array('c_combined_seat_course' => 0));
  $a = t_make_applicant($c);
  $g = array(0 => array(0 => array_merge($row, array('a_id' => $a))));
  dh_render_seat_grid($g, 2, 0, LEFT_TO_RIGHT, 'main-div', 1, array('write_seat' => TRUE, 'label_start' => 8));
  t_eq(db_query("select aa_seat from dh_applicant_attended where aa_applicant = :a", array(':a' => $a))->fetchField(), 'I1', 'aa_seat saved as I1');
  dh_render_seat_grid($g, 2, 0, LEFT_TO_RIGHT, 'main-div', 1, array('write_seat' => TRUE, 'write_col' => 'aa_group_seat', 'label_start' => 2));
  t_eq(db_query("select aa_group_seat from dh_applicant_attended where aa_applicant = :a", array(':a' => $a))->fetchField(), 'C1', 'aa_group_seat saved as C1');
  dh_render_seat_grid($g, 2, 0, LEFT_TO_RIGHT, 'main-div', 0, array('write_seat' => TRUE, 'label_start' => 100));
  t_eq(db_query("select aa_seat from dh_applicant_attended where aa_applicant = :a", array(':a' => $a))->fetchField(), '101', 'numbered: aa_seat saved as 101');
});
t_done();
