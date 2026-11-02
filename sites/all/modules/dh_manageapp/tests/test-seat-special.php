<?php
// Backrest / chowky / chair save from the seating plans (inc/seat-special.inc).
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-seat-special.php
require __DIR__ . '/lib.php';

function t_aa($a) {
  return db_query("select aa_chowky, aa_chair, aa_backrest, aa_seat_row, aa_seat_col, aa_group_seat_row, aa_group_seat_col, aa_updated_by
    from dh_applicant_attended where aa_applicant = :a", array(':a' => $a))->fetchAssoc();
}

$aa0 = array('aa_chowky' => 0, 'aa_chair' => 0, 'aa_backrest' => 0, 'aa_group' => 1, 'aa_updated_by' => 0);

t_in_txn(function () use ($aa0) {
  $uid = 1;
  $c = t_make_course(array('c_combined_seat_course' => 0));
  $c2 = t_make_course(array('c_combined_seat_course' => $c));   // seats together with $c
  $other = t_make_course(array('c_combined_seat_course' => 0));
  $m = t_make_applicant($c, array(), $aa0 + array('aa_seat_row' => 2, 'aa_seat_col' => 3, 'aa_group_seat_row' => 1, 'aa_group_seat_col' => 1));
  $k = t_make_applicant($c, array(), array('aa_chowky' => 1) + $aa0 + array('aa_seat_row' => 0, 'aa_seat_col' => 0, 'aa_group_seat_row' => 0, 'aa_group_seat_col' => 0));

  // Refusals: nothing is saved.
  $r = dh_seat_special_apply($c, $m, 'sofa', 'main', $uid);
  t_eq(array($r['status'], $r['msg']), array('Failed', 'Invalid value.'), 'unknown value refused');
  $r = dh_seat_special_apply($c, $m, 'backrest', 'side', $uid);
  t_eq(array($r['status'], $r['msg']), array('Failed', 'Invalid value.'), 'unknown grid refused');
  $x = t_make_applicant($other, array(), $aa0);
  $r = dh_seat_special_apply($c, $x, 'backrest', 'main', $uid);
  t_eq(array($r['status'], $r['msg']), array('Failed', 'Student not found in this course.'), 'student of another course refused');
  $sv = t_make_applicant($c, array('a_type' => 'Server'), $aa0);
  $r = dh_seat_special_apply($c, $sv, 'backrest', 'main', $uid);
  t_eq($r['status'], 'Failed', 'server refused');
  $na = t_make_applicant($c, array('a_attended' => 0), $aa0);
  $r = dh_seat_special_apply($c, $na, 'backrest', 'main', $uid);
  t_eq($r['status'], 'Failed', 'not attended refused');
  $r = dh_seat_special_apply($c, $m, 'backrest', 'chowky', $uid);
  t_eq(array($r['status'], $r['msg']), array('Failed', 'This student was changed by someone else. Please reload the page.'), 'stale grid refused');
  $row = t_aa($m);
  t_eq(array($row['aa_backrest'], $row['aa_updated_by']), array(0, 0), 'refusals saved nothing');

  // Backrest on / off: saved at once, the seat is kept, no reload.
  $r = dh_seat_special_apply($c, $m, 'backrest', 'main', $uid);
  t_eq(array($r['status'], $r['reload']), array('OK', FALSE), 'backrest on');
  t_eq(t_aa($m), array('aa_chowky' => 0, 'aa_chair' => 0, 'aa_backrest' => 1, 'aa_seat_row' => 2, 'aa_seat_col' => 3,
    'aa_group_seat_row' => 1, 'aa_group_seat_col' => 1, 'aa_updated_by' => $uid), 'backrest saved, seat kept');
  $r = dh_seat_special_apply($c, $m, 'none', 'main', $uid);
  $row = t_aa($m);
  t_eq(array($r['status'], $row['aa_backrest']), array('OK', 0), 'backrest off');

  // Main grid -> chowky grid (Chair): a new top row of the chowky grid in both plans.
  $r = dh_seat_special_apply($c, $m, 'chair', 'main', $uid);
  t_eq(array($r['status'], $r['reload']), array('OK', TRUE), 'move to the chowky grid');
  t_eq(t_aa($m), array('aa_chowky' => 0, 'aa_chair' => 1, 'aa_backrest' => 0, 'aa_seat_row' => 1, 'aa_seat_col' => 0,
    'aa_group_seat_row' => 1, 'aa_group_seat_col' => 0, 'aa_updated_by' => $uid), 'chair, top of the chowky grid in both plans');

  // Chowky grid -> main grid with backrest: the main grid is now empty, so the front row.
  $r = dh_seat_special_apply($c, $k, 'backrest', 'chowky', $uid);
  t_eq(array($r['status'], $r['reload']), array('OK', TRUE), 'move to the main grid');
  t_eq(t_aa($k), array('aa_chowky' => 0, 'aa_chair' => 0, 'aa_backrest' => 1, 'aa_seat_row' => 0, 'aa_seat_col' => 0,
    'aa_group_seat_row' => 0, 'aa_group_seat_col' => 0, 'aa_updated_by' => $uid), 'with backrest, front row of the empty main grid');

  // Chair -> Chowky stays in the chowky grid: saved, seat kept, no reload.
  $r = dh_seat_special_apply($c, $m, 'chowky', 'chowky', $uid);
  $row = t_aa($m);
  t_eq(array($r['reload'], $row['aa_chowky'], $row['aa_chair'], $row['aa_seat_row']), array(FALSE, 1, 0, 1), 'chair to chowky: seat kept');

  // A student of the combined-seat course is accepted through this plan.
  $j = t_make_applicant($c2, array(), $aa0 + array('aa_seat_row' => 5, 'aa_seat_col' => 0));
  $r = dh_seat_special_apply($c, $j, 'backrest', 'main', $uid);
  t_eq($r['status'], 'OK', 'student of the combined-seat course accepted');

  t_ok(lock_may_be_available('dh_app_update_attended'), 'lock released');
});
t_done();
