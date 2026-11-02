<?php
// Shared re-seat step for a grid or group change (inc/seat-special.inc).
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-seat-reseat.php
require __DIR__ . '/lib.php';

$aa0 = array('aa_chowky' => 0, 'aa_chair' => 0, 'aa_backrest' => 0, 'aa_group' => 1);

t_in_txn(function () use ($aa0) {
  $c = t_make_course(array('c_combined_seat_course' => 0));
  $m1 = t_make_applicant($c, array(), $aa0 + array('aa_seat_row' => 0, 'aa_seat_col' => 0, 'aa_group_seat_row' => 0, 'aa_group_seat_col' => 0));
  $m2 = t_make_applicant($c, array(), $aa0 + array('aa_seat_row' => 2, 'aa_seat_col' => 1, 'aa_group_seat_row' => 3, 'aa_group_seat_col' => 0));
  $k1 = t_make_applicant($c, array(), array('aa_chowky' => 1) + $aa0 + array('aa_seat_row' => 0, 'aa_seat_col' => 0, 'aa_group_seat_row' => 1, 'aa_group_seat_col' => 0));
  $f1 = t_make_applicant($c, array('a_gender' => 'F'), array('aa_chowky' => 1) + $aa0 + array('aa_seat_row' => 7, 'aa_seat_col' => 0));
  // Never counted: not attended, and a server.
  t_make_applicant($c, array('a_attended' => 0), $aa0 + array('aa_seat_row' => 9, 'aa_seat_col' => 0));
  t_make_applicant($c, array('a_type' => 'Server'), $aa0 + array('aa_seat_row' => 5, 'aa_seat_col' => 0));

  t_eq(dh_seat_plan_courses($c), array($c), 'a course alone is its own plan');
  t_eq(dh_seat_reseat_fields($m1, $c, 'M', TRUE, TRUE, NULL), array('aa_seat_row' => 1, 'aa_seat_col' => 0),
    'to the chowky grid: new top row (female rows not counted)');
  t_eq(dh_seat_reseat_fields($k1, $c, 'M', FALSE, TRUE, 1),
    array('aa_seat_row' => 3, 'aa_seat_col' => 0, 'aa_group_seat_row' => 4, 'aa_group_seat_col' => 0),
    'to the main grid: new top row in both plans (not-attended and server rows not counted)');
  t_eq(dh_seat_reseat_fields($m2, $c, 'M', FALSE, TRUE, NULL), array('aa_seat_row' => 1, 'aa_seat_col' => 0),
    'the student own seat is never counted');
  t_eq(dh_seat_reseat_fields($f1, $c, 'F', FALSE, TRUE, NULL), array('aa_seat_row' => 0, 'aa_seat_col' => 0),
    'empty grid: front row');
  t_eq(dh_seat_reseat_fields($m1, $c, 'M', FALSE, FALSE, 2), array('aa_group_seat_row' => 0, 'aa_group_seat_col' => 0),
    'group change only: the empty group 2 grid starts at the front row');
  t_eq(dh_seat_reseat_fields($m1, $c, 'M', FALSE, FALSE, NULL), array(), 'nothing asked, nothing returned');
});

// Combined-seat courses: course B seats together with course A.
t_in_txn(function () use ($aa0) {
  $a = t_make_course(array('c_combined_seat_course' => 0));
  $b = t_make_course(array('c_combined_seat_course' => $a));
  t_make_applicant($a, array(), array('aa_chowky' => 1) + $aa0 + array('aa_seat_row' => 0, 'aa_seat_col' => 0));
  t_make_applicant($b, array(), array('aa_chowky' => 1) + $aa0 + array('aa_seat_row' => 4, 'aa_seat_col' => 0));
  $ma = t_make_applicant($a, array(), $aa0 + array('aa_seat_row' => 0, 'aa_seat_col' => 1));
  t_eq(dh_seat_plan_courses($a), array($a, $b), 'plan of A includes B (B combines with A)');
  t_eq(dh_seat_plan_courses($b), array($a, $b), 'plan of B includes A');
  t_eq(dh_seat_reseat_fields($ma, $a, 'M', TRUE, TRUE, NULL), array('aa_seat_row' => 5, 'aa_seat_col' => 0),
    'combined: the chowky row 4 of course B is counted');
});
t_done();
