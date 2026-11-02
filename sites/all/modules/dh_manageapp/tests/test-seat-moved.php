<?php
// The grid / group change decision shared by the Zero-Day popup and the application edit form (inc/seat-special.inc).
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-seat-moved.php
require __DIR__ . '/lib.php';

$aa0 = array('aa_chowky' => 0, 'aa_chair' => 0, 'aa_backrest' => 0, 'aa_group' => 1);

t_in_txn(function () use ($aa0) {
  $c = t_make_course(array('c_combined_seat_course' => 0));
  t_make_applicant($c, array(), $aa0 + array('aa_seat_row' => 3, 'aa_seat_col' => 0, 'aa_group_seat_row' => 2, 'aa_group_seat_col' => 0));
  t_make_applicant($c, array(), array('aa_chowky' => 1) + $aa0 + array('aa_seat_row' => 1, 'aa_seat_col' => 0, 'aa_group_seat_row' => 0, 'aa_group_seat_col' => 0));
  $s = t_make_applicant($c, array(), array('aa_chowky' => 1) + $aa0 + array('aa_seat_row' => 0, 'aa_seat_col' => 1, 'aa_group_seat_row' => 1, 'aa_group_seat_col' => 1));
  $prev = array('aa_chowky' => 1, 'aa_chair' => 0, 'aa_group' => '1');
  t_eq(dh_seat_moved_fields($s, $c, 'M', $prev, FALSE, '1'),
    array('aa_seat_row' => 4, 'aa_seat_col' => 0, 'aa_group_seat_row' => 3, 'aa_group_seat_col' => 0),
    'chowky to none: new top row of the main grid in both plans');
  t_eq(dh_seat_moved_fields($s, $c, 'M', $prev, TRUE, '1'), array(), 'same grid, same group: seat kept');
  t_eq(dh_seat_moved_fields($s, $c, 'M', $prev, TRUE, '2'), array('aa_group_seat_row' => 0, 'aa_group_seat_col' => 0),
    'group change only: new seat in the new group grid');
  t_eq(dh_seat_moved_fields($s, $c, 'M', array('aa_chowky' => 0, 'aa_chair' => 1, 'aa_group' => '1'), TRUE, '1'), array(),
    'chair to chowky stays in the chowky grid');
});
t_done();
