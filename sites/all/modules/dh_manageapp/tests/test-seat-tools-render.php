<?php
// Seating plan markup for the new tools (inc/seat-special.inc, dh_render_seat_grid).
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-seat-tools-render.php
require __DIR__ . '/lib.php';

$row = array('a_id' => 101, 'a_old' => 0, 'aa_chowky' => 0, 'aa_chair' => 0, 'aa_backrest' => 1, 'aa_seat_aid' => '',
  'aa_section' => 'A', 'aa_acco' => '1', 'aa_cell' => '', 'Full Name' => 'Zzqtest One', 'Age' => 40);
$grid = array(0 => array(0 => $row));

$html = dh_render_seat_grid($grid, 2, NULL, LEFT_TO_RIGHT, 'main-div', 0, array('special_edit' => TRUE));
t_ok(strpos($html, 'data-special="backrest"') !== FALSE, 'main grid card carries its special seating');
t_ok(strpos($html, 'class="s-special-btn no-print"') !== FALSE, 'main grid card has the student menu button');
$plain = dh_render_seat_grid($grid, 2, NULL, LEFT_TO_RIGHT, 'main-div', 0, array());
t_ok(strpos($plain, 's-special-btn') === FALSE && strpos($plain, 'data-special') === FALSE, 'no menu without special_edit');
$chair = array_merge($row, array('aa_backrest' => 0, 'aa_chair' => 1));
$html = dh_render_seat_grid(array(0 => array(0 => $chair)), 1, NULL, LEFT_TO_RIGHT, 'chowky-div', 0, array('special_edit' => TRUE));
t_ok(strpos($html, 'data-special="chair"') !== FALSE && strpos($html, 's-special-btn') !== FALSE, 'chowky grid card has the menu too');

$bar = dh_seat_section_tools_html();
t_ok(strpos($bar, 'class="dh-swap-toggle"') !== FALSE && strpos($bar, 'Swap / Move seats') !== FALSE, 'section bar: Swap / Move seats');
t_ok(strpos($bar, 'class="store-seat-changes dh-sec-store"') !== FALSE && strpos($bar, '>Store Seat Changes<') !== FALSE, 'section bar: Store Seat Changes');

drupal_static_reset('dh_seat_tools_js');
$js = dh_seat_tools_js(5, 123);
t_ok(strpos($js, 'seat-special\\/5\\/123') !== FALSE, 'tools settings carry the save URL');
t_ok(strpos($js, 'js/seat-tools.js?v=') !== FALSE, 'tools script is loaded');
t_ok(preg_match('/"token":"[^"]+"/', $js) === 1, 'tools settings carry a token');
t_eq(dh_seat_tools_js(5, 123), '', 'printed once per page');

// The section-PDF path ($return_html) never carries editing tools.
t_in_txn(function () {
  $c = t_make_course(array('c_center' => 5, 'c_combined_seat_course' => 0));
  t_make_applicant($c, array(), array('aa_chowky' => 0, 'aa_chair' => 0, 'aa_backrest' => 0, 'aa_group' => 1,
    'aa_seat_row' => 0, 'aa_seat_col' => 0, 'aa_group_seat_row' => 0, 'aa_group_seat_col' => 0));
  $none = function ($h) { return strpos($h, 'dh-swap-toggle') === FALSE && strpos($h, 's-special-btn') === FALSE && strpos($h, 'seat-tools.js') === FALSE; };
  $html = dh_generate_seating_plan(5, $c, '', FALSE, TRUE);
  t_ok(strpos($html, 'Zzqtest') !== FALSE, 'seating plan rendered');
  t_ok($none($html), 'seating plan PDF HTML has no editing tools');
  $html = dh_generate_group_seating_plan(5, $c, '', TRUE);
  t_ok(strpos($html, 'Zzqtest') !== FALSE, 'group plan rendered');
  t_ok($none($html), 'group plan PDF HTML has no editing tools');
});
t_done();
