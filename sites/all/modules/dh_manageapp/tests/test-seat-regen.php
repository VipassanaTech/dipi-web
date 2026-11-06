<?php
// Regenerate one section of a seating plan (inc/seat-special.inc): only that section is re-packed.
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-seat-regen.php
require __DIR__ . '/lib.php';

// Section ids as the plans print them.
t_eq(dh_seat_regen_parse('main', 'M'), array('M', NULL, 'Male'), 'main plan: Male section');
t_eq(dh_seat_regen_parse('main', 'F'), array('F', NULL, 'Female'), 'main plan: Female section');
t_eq(dh_seat_regen_parse('group', 'g1_F'), array('F', '1', 'Group 1 - Female'), 'group plan: Group 1 - Female');
t_eq(dh_seat_regen_parse('group', 'gUnassigned_M'), array('M', 'Unassigned', 'Group Unassigned - Male'), 'group plan: no group');
foreach (array(array('main', 'X'), array('main', 'g1_M'), array('group', 'M'), array('group', 'g_M'), array('group', 'g1_X'), array('other', 'M')) as $bad)
  t_eq(dh_seat_regen_parse($bad[0], $bad[1]), FALSE, "refused: {$bad[0]} / {$bad[1]}");

// Row/col of one student in the main plan and the group plan.
function rg_pos($a) {
  return db_query("select aa_seat_row, aa_seat_col, aa_group_seat_row, aa_group_seat_col from dh_applicant_attended where aa_applicant = :a",
    array(':a' => $a))->fetchAssoc();
}
$aa0 = array('aa_chowky' => 0, 'aa_chair' => 0, 'aa_backrest' => 0, 'aa_group' => 1);

// Main plan: regenerating Male re-packs both male grids; Female and the group plan keep their seats.
t_in_txn(function () use ($aa0) {
  $_GET['q'] = 'seating/5/0';   // dh_allocate_seats reads the centre from the path
  $c = t_make_course(array('c_center' => 5, 'c_combined_seat_course' => 0));
  $m = array();
  foreach (array(5, 6, 7) as $i => $row)
    $m[] = t_make_applicant($c, array('a_old' => 1), $aa0 + array('aa_seat_row' => $row, 'aa_seat_col' => 3, 'aa_group_seat_row' => 9, 'aa_group_seat_col' => $i));
  $k = t_make_applicant($c, array(), array('aa_chowky' => 1) + $aa0 + array('aa_seat_row' => 4, 'aa_seat_col' => 0, 'aa_group_seat_row' => 8, 'aa_group_seat_col' => 0));
  $f = t_make_applicant($c, array('a_gender' => 'F'), $aa0 + array('aa_seat_row' => 9, 'aa_seat_col' => 9, 'aa_group_seat_row' => 7, 'aa_group_seat_col' => 7));
  $before = array(); foreach (array_merge($m, array($k, $f)) as $a) $before[$a] = rg_pos($a);

  t_eq(dh_seat_regen_section(5, $c, 'main', 'M'), 'Male', 'Male section regenerated');
  $seen = array();
  foreach ($m as $a) { $p = rg_pos($a); $seen[$p['aa_seat_row'] . '-' . $p['aa_seat_col']] = 1; }
  t_eq(count($seen), 3, 'three males, three different seats');
  t_ok(!isset($seen['5-3']) && !isset($seen['6-3']) && !isset($seen['7-3']), 'males moved off their old seats');
  t_eq(rg_pos($m[0])['aa_seat_row'], 0, 'packed from the front row');
  t_eq(array(rg_pos($k)['aa_seat_row'], rg_pos($k)['aa_seat_col']), array(0, 0), 'male chowky grid re-packed too');
  t_eq(rg_pos($f), $before[$f], 'Female section not changed');
  $grp = TRUE; foreach (array_merge($m, array($k)) as $a) { $p = rg_pos($a); $grp = $grp && $p['aa_group_seat_row'] == $before[$a]['aa_group_seat_row'] && $p['aa_group_seat_col'] == $before[$a]['aa_group_seat_col']; }
  t_ok($grp, 'group plan seats not changed');
});

// Group plan: regenerating Group 1 - Male leaves Group 1 - Female, Group 2, the main plan and absent students alone.
t_in_txn(function () use ($aa0) {
  $_GET['q'] = 'group-seating/5/0';
  $c = t_make_course(array('c_center' => 5, 'c_combined_seat_course' => 0));
  $g1a = t_make_applicant($c, array(), $aa0 + array('aa_seat_row' => 2, 'aa_seat_col' => 2, 'aa_group_seat_row' => 5, 'aa_group_seat_col' => 3));
  $g1b = t_make_applicant($c, array(), $aa0 + array('aa_seat_row' => 3, 'aa_seat_col' => 2, 'aa_group_seat_row' => 6, 'aa_group_seat_col' => 3));
  $g2 = t_make_applicant($c, array(), array('aa_group' => 2) + $aa0 + array('aa_seat_row' => 1, 'aa_seat_col' => 1, 'aa_group_seat_row' => 4, 'aa_group_seat_col' => 1));
  $f1 = t_make_applicant($c, array('a_gender' => 'F'), $aa0 + array('aa_seat_row' => 1, 'aa_seat_col' => 1, 'aa_group_seat_row' => 7, 'aa_group_seat_col' => 2));
  $gone = t_make_applicant($c, array('a_attended' => 0), $aa0 + array('aa_seat_row' => 3, 'aa_seat_col' => 3, 'aa_group_seat_row' => 3, 'aa_group_seat_col' => 3));
  $none = t_make_applicant($c, array(), array('aa_group' => NULL) + $aa0 + array('aa_seat_row' => 8, 'aa_seat_col' => 1, 'aa_group_seat_row' => 8, 'aa_group_seat_col' => 4));
  $before = array(); foreach (array($g1a, $g1b, $g2, $f1, $gone, $none) as $a) $before[$a] = rg_pos($a);

  t_eq(dh_seat_regen_section(5, $c, 'group', 'g1_M'), 'Group 1 - Male', 'Group 1 - Male regenerated');
  $a = rg_pos($g1a); $b = rg_pos($g1b);
  t_eq(min($a['aa_group_seat_row'], $b['aa_group_seat_row']), 0, 'Group 1 males packed from the front row');
  t_ok($a['aa_group_seat_row'] . '-' . $a['aa_group_seat_col'] !== $b['aa_group_seat_row'] . '-' . $b['aa_group_seat_col'], 'Group 1 males on different seats');
  foreach (array('Group 2' => $g2, 'Group 1 - Female' => $f1, 'not attended' => $gone, 'no group' => $none) as $what => $id)
    t_eq(rg_pos($id), $before[$id], "$what not changed");
  t_ok(rg_pos($g1a)['aa_seat_row'] == 2 && rg_pos($g1b)['aa_seat_row'] == 3, 'main plan seats not changed');

  t_eq(dh_seat_regen_section(5, $c, 'group', 'gUnassigned_M'), 'Group Unassigned - Male', 'no-group section regenerated');
  t_eq(array(rg_pos($none)['aa_group_seat_row'], rg_pos($none)['aa_group_seat_col']), array(0, 0), 'no-group student packed to the front');
  t_eq(rg_pos($g2), $before[$g2], 'Group 2 still not changed');
  t_eq(dh_seat_regen_section(5, $c, 'group', 'g1_X'), FALSE, 'a bad section changes nothing');
});

// The request step: whole plan, one section with a good token, one with a bad token.
t_in_txn(function () use ($aa0) {
  $_GET['q'] = 'seating/5/0';
  $c = t_make_course(array('c_center' => 5, 'c_combined_seat_course' => 0));
  $mm = t_make_applicant($c, array('a_old' => 1), $aa0 + array('aa_seat_row' => 6, 'aa_seat_col' => 3));
  $ff = t_make_applicant($c, array('a_gender' => 'F', 'a_old' => 1), $aa0 + array('aa_seat_row' => 6, 'aa_seat_col' => 2));

  $go = dh_seat_regen_run(5, $c, $c, 'main', array('r' => 1, 'sec' => 'F', 'token' => 'wrong'));
  t_eq($go, array("seating/5/$c", array('query' => array('failed' => 'F'), 'fragment' => 'dh-sec-F')), 'bad token: not regenerated, back to the section');
  t_eq(rg_pos($ff)['aa_seat_row'], 6, 'bad token changed nothing');

  $go = dh_seat_regen_run(5, $c, $c, 'main', array('r' => 1, 'sec' => 'F', 'token' => drupal_get_token('seat-regen')));
  t_eq($go, array("seating/5/$c", array('query' => array('done' => 'F'), 'fragment' => 'dh-sec-F')), 'good token: done, back to the section');
  t_eq(rg_pos($ff)['aa_seat_row'], 0, 'Female re-packed');
  t_eq(rg_pos($mm)['aa_seat_row'], 6, 'Male not changed');

  $go = dh_seat_regen_run(5, $c, $c, 'main', array('r' => 1));
  t_eq($go, array("seating/5/$c", array('query' => array('done' => 'all'))), 'whole plan: to the plain address');
  t_eq(rg_pos($mm)['aa_seat_row'], 0, 'whole plan re-packed Male too');

  $go = dh_seat_regen_run(5, $c, $c, 'group', array('r' => 1, 'sec' => 'g1_M', 'token' => drupal_get_token('seat-regen')));
  t_eq($go, array("group-seating/5/$c", array('query' => array('done' => 'g1_M'), 'fragment' => 'dh-sec-g1_M')), 'group section: back to the group page');
});

// The link and the notes.
$link = dh_seat_regen_link('group', 5, 123, 'g1_M', 'Group 1 - Male');
t_ok(strpos($link, 'class="dh-sec-regen"') !== FALSE && strpos($link, 'Regenerate this section') !== FALSE, 'section link');
t_ok(strpos($link, 'group-seating/5/123?r=1&amp;sec=g1_M&amp;token=' . drupal_get_token('seat-regen')) !== FALSE, 'link carries the section and a token');
t_ok(strpos($link, 'Group 1 - Male') !== FALSE && strpos($link, 'Other sections are not changed') !== FALSE && strpos($link, 'dhSeatPendingCount') !== FALSE, 'link asks first, with the unsaved-changes warning');
t_ok(strpos(dh_seat_regen_link('main', 5, 123, 'M', 'Male'), '"/seating/5/123?r=1&amp;sec=M&amp;') !== FALSE || strpos(dh_seat_regen_link('main', 5, 123, 'M', 'Male'), 'seating/5/123?r=1&amp;sec=M&amp;') !== FALSE, 'main plan link');

$_GET = array('done' => 'M');
t_ok(strpos(dh_seat_regen_notice('M'), 'Regenerated') !== FALSE, 'note at the regenerated section');
t_eq(dh_seat_regen_notice('F'), '', 'no note at another section');
$_GET = array('failed' => 'M');
t_ok(strpos(dh_seat_regen_notice('M'), 'not regenerated') !== FALSE, 'note when the link was refused');
$_GET = array('done' => 'all');
t_ok(strpos(dh_seat_regen_notice('all'), 'All sections regenerated') !== FALSE, 'note for the whole plan');
$_GET = array('done' => '<b>');
t_eq(dh_seat_regen_notice('all') . dh_seat_regen_notice('M'), '', 'unknown value: no note');
$_GET = array();

// Print/PDF HTML never has the link; the sections carry their anchor.
t_in_txn(function () use ($aa0) {
  $_GET['q'] = 'seating/5/0';
  $c = t_make_course(array('c_center' => 5, 'c_combined_seat_course' => 0));
  t_make_applicant($c, array(), $aa0 + array('aa_seat_row' => 0, 'aa_seat_col' => 0, 'aa_group_seat_row' => 0, 'aa_group_seat_col' => 0));
  $html = dh_generate_seating_plan(5, $c, '', FALSE, TRUE);
  t_ok(strpos($html, 'id="dh-sec-M"') !== FALSE, 'main plan section anchor');
  t_ok(strpos($html, 'dh-sec-regen') === FALSE, 'main plan PDF HTML has no section regenerate link');
  t_ok(strpos($html, '13- To make only one section again') !== FALSE, 'main plan help has the section regenerate step');
  $html = dh_generate_group_seating_plan(5, $c, '', TRUE);
  t_ok(strpos($html, 'id="dh-sec-g1_M"') !== FALSE, 'group plan section anchor');
  t_ok(strpos($html, 'dh-sec-regen') === FALSE, 'group plan PDF HTML has no section regenerate link');
  t_ok(strpos($html, '13- To make only one section again') !== FALSE, 'group plan help has the section regenerate step');
});
t_done();
