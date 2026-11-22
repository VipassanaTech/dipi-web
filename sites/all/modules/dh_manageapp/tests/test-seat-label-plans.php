<?php
// The main and the group seating plan draw and save labels with the course's starts.
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-seat-label-plans.php
require __DIR__ . '/lib.php';

$centre = (int) db_query("select cs_center from dh_center_setting where cs_center > 0 and cs_seat_config like '%[RIGHT]%' and cs_seat_config like '%[LEFT]%'
  order by cs_center limit 1")->fetchField();
function lp_seat($a, $col = 'aa_seat') {
  return db_query("select $col from dh_applicant_attended where aa_applicant = :a", array(':a' => $a))->fetchField();
}

t_in_txn(function () use ($centre) {
  db_query("update dh_center_setting set cs_seat_naming_conv = 1 where cs_center = :c", array(':c' => $centre));
  $c = t_make_course(array('c_center' => $centre, 'c_combined_seat_course' => 0));
  $aa = array('aa_chowky' => 0, 'aa_chair' => 0, 'aa_backrest' => 0, 'aa_group' => 1);
  $m1 = t_make_applicant($c, array(), $aa + array('aa_seat_row' => 0, 'aa_seat_col' => 0, 'aa_group_seat_row' => 0, 'aa_group_seat_col' => 0));
  $m2 = t_make_applicant($c, array(), $aa + array('aa_seat_row' => 0, 'aa_seat_col' => 1, 'aa_group_seat_row' => 0, 'aa_group_seat_col' => 1));
  $mk = t_make_applicant($c, array(), array('aa_chowky' => 1) + $aa + array('aa_seat_row' => 0, 'aa_seat_col' => 0, 'aa_group_seat_row' => 0, 'aa_group_seat_col' => 0));
  $f1 = t_make_applicant($c, array('a_gender' => 'F'), $aa + array('aa_seat_row' => 0, 'aa_seat_col' => 0, 'aa_group_seat_row' => 0, 'aa_group_seat_col' => 0));

  // No start saved: the normal labels.
  $_GET['q'] = "seating/$centre/$c";
  dh_generate_seating_plan($centre, $c, '', FALSE, TRUE);
  t_eq(array(lp_seat($m1), lp_seat($m2), lp_seat($mk), lp_seat($f1)), array('A1', 'B1', 'CW-A1', 'A1'), 'main plan, no start');

  // Male main grid from I, Male chowky grid from C; Female unchanged.
  dh_seat_label_start_save($centre, $c, 'main', 'M', array('main' => 'I', 'chowky' => 'C'), 77);
  $html = dh_generate_seating_plan($centre, $c, '', FALSE, TRUE);
  t_eq(array(lp_seat($m1), lp_seat($m2), lp_seat($mk), lp_seat($f1)), array('I1', 'J1', 'CW-C1', 'A1'), 'main plan: Male from I, chowky from C, Female normal');
  t_ok(strpos($html, '<span class="seatnumber">I1</span>') !== FALSE, 'the page shows I1');
  t_ok(strpos($html, 'data-label-start="8"') !== FALSE && strpos($html, 'data-label-start="2"') !== FALSE, 'the grids carry their starts');
  $ro = dh_generate_seating_plan($centre, $c, '', TRUE, TRUE);
  t_ok(strpos($ro, '<span class="seatnumber">J1</span>') !== FALSE, 'the read-only (AT) view shows the same labels');
  t_ok(strpos($html, 'dh-sls-btn') === FALSE, 'returned HTML (PDF path) has no Seat labels button');

  // Group plan: Group 1 - Male from D; the main plan labels stay.
  $_GET['q'] = "group-seating/$centre/$c";
  dh_seat_label_start_save($centre, $c, 'group', 'g1_M', array('main' => 'D', 'chowky' => ''), 77);
  dh_generate_group_seating_plan($centre, $c, '', TRUE);
  t_eq(array(lp_seat($m1, 'aa_group_seat'), lp_seat($m2, 'aa_group_seat'), lp_seat($mk, 'aa_group_seat'), lp_seat($f1, 'aa_group_seat')),
    array('D1', 'E1', 'CW-A1', 'A1'), 'group plan: Group 1 - Male main grid from D, others normal');
  t_eq(lp_seat($m1), 'I1', 'the main plan label is not touched by the group plan');

  // The column tools relabel with the grid start (both copies of the script).
  t_ok(strpos($html, 'function dhLabel(col,row,st){ st=st||0;') !== FALSE, 'main plan script: dhLabel takes the start');
  t_eq(substr_count($html, 'parseInt($div.attr("data-label-start")||"0",10)'), 2, 'main plan script: both relabel calls pass the start');
  $ghtml = dh_generate_group_seating_plan($centre, $c, '', TRUE);
  t_ok(strpos($html, 'String(sr*cols+sc+1+(st||0))') !== FALSE, 'main plan script: numbered style relabels with numbers');
  t_ok(strpos($ghtml, 'String(sr*cols+sc+1+(st||0))') !== FALSE, 'group plan script: numbered style relabels with numbers');
  t_ok(strpos($ghtml, 'function dhLabel(col,row,st){ st=st||0;') !== FALSE, 'group plan script: dhLabel takes the start');
  t_eq(substr_count($ghtml, 'parseInt($div.attr("data-label-start")||"0",10)'), 2, 'group plan script: both relabel calls pass the start');
});
t_done();
