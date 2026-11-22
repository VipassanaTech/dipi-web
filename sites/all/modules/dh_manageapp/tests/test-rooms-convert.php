<?php
// Rooms conversion: room key, grouping, per-centre dry run and apply.
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-rooms-convert.php
require __DIR__ . '/lib.php';

$keys = array('12A' => '12', '12 b' => '12', '12-A' => '12', '12.A' => '12', '12/1' => '12', '12-2' => '12', '1.1' => '1', '12(L)' => '12', '12 (U)' => '12',
  '13' => '', 'A12' => '', 'B-3' => '', '12AB' => '', '' => '');
foreach ($keys as $in => $want) t_eq(dh_rooms_room_key($in), $want, "K room key of '$in'");

$g = dh_rooms_group_places(array('12A', '12B', ' 13 ', '14/1', '14/2', '15(L)', '15(U)', 'A1', '12a', '', '16A'));
t_eq($g['rooms'], array(
  array('name' => '12', 'places' => array('12A', '12B')),
  array('name' => '13', 'places' => array('13')),
  array('name' => '14', 'places' => array('14/1', '14/2')),
  array('name' => '15', 'places' => array('15(L)', '15(U)')),
  array('name' => 'A1', 'places' => array('A1')),
  array('name' => '16A', 'places' => array('16A')),
), 'G1 shared places become one room; single ones keep their own name; typed order kept');
t_eq($g['duplicates'], 1, 'G2 "12a" repeats "12A" (case and spaces ignored)');

t_in_txn(function () {
  $cen = t_make_centre();
  foreach (array(array('M', 'B', '12A, 12B, 13, 13, 0123456789ABCDEFG'), array('M', 'C', '1,2'), array('F', 'D', '7A,7B,7C')) as $row)
    db_insert('dh_center_setting_acco')->fields(array('csa_center' => $cen, 'csa_gender' => $row[0], 'csa_section' => $row[1], 'csa_room' => $row[2], 'csa_deleted' => 0))->execute();
  db_insert('dh_center_setting_acco')->fields(array('csa_center' => $cen, 'csa_gender' => 'M', 'csa_section' => 'Z', 'csa_room' => '99', 'csa_deleted' => 1))->execute();
  $up = t_make_course(array('c_center' => $cen, 'c_start' => date('Y-m-d', strtotime('+10 days')), 'c_end' => date('Y-m-d', strtotime('+20 days'))));
  t_make_applicant($up, array('a_gender' => 'M'), array('aa_section' => 'B', 'aa_acco' => '40'));
  t_make_applicant($up, array('a_gender' => 'M'), array('aa_section' => 'B', 'aa_acco' => '12A'));
  $past = t_make_course(array('c_center' => $cen));
  t_make_applicant($past, array('a_gender' => 'M'), array('aa_section' => 'B', 'aa_acco' => '41'));
  $want = array('centre' => $cen, 'skipped' => '', 'rooms' => 6, 'shared_rooms' => 2, 'places' => 9, 'duplicates' => 1, 'too_long' => 1, 'review_added' => 1, 'past_only_missing' => 1);
  t_eq(dh_rooms_convert_centre($cen, FALSE), $want, 'C1 dry run report: rooms 12,13 / 1 / 2 / 7 + needs-review 40; repeat 13 and the 17-letter place counted; deleted row ignored');
  t_eq(dh_rooms_centre_has_rooms($cen), FALSE, 'C2 the dry run wrote nothing');
  t_eq(dh_rooms_convert_centre($cen, TRUE), $want, 'C3 apply gives the same report');
  $rooms = dh_rooms_load($cen);
  t_eq(array_map(function ($r) { return $r['gender'] . '-' . $r['section'] . '-' . $r['name'] . ':' . implode('+', array_map(function ($p) { return $p['name']; }, $r['places'])) . ':' . $r['toilet'] . ':' . $r['review']; }, $rooms),
    array('M-B-12:12A+12B:unset:0', 'M-B-13:13:unset:0', 'M-B-40:40:unset:1', 'M-C-1:1:unset:0', 'M-C-2:2:unset:0', 'F-D-7:7A+7B+7C:unset:0'), 'C4 rooms written, toilet Not set, the course-only place marked needs review');
  t_eq(db_query("select count(*) from dh_center_log where cl_center = :c and cl_area = 'rooms'", array(':c' => $cen))->fetchField(), 1, 'C5 one Change Log line');
  t_eq(dh_rooms_convert_centre($cen, TRUE)['skipped'], 'already has rooms', 'C6 a second run skips the centre');

  $cenb = t_make_centre();
  db_insert('dh_center_setting_acco')->fields(array('csa_center' => $cenb, 'csa_gender' => 'M', 'csa_section' => 'B', 'csa_room' => '12A, 12B', 'csa_deleted' => 0))->execute();
  $upb = t_make_course(array('c_center' => $cenb, 'c_start' => date('Y-m-d', strtotime('+10 days')), 'c_end' => date('Y-m-d', strtotime('+20 days'))));
  t_make_applicant($upb, array('a_gender' => 'M'), array('aa_section' => 'B', 'aa_acco' => ' 12A'));
  $repb = dh_rooms_convert_centre($cenb, FALSE);
  t_eq($repb['review_added'], 0, 'C1b a saved place with a leading space (" 12A") matches the list: nothing needs review');
  t_eq($repb['rooms'], 1, 'C1b the list makes one room (12 with 12A and 12B)');
});
t_done();
