<?php
// Rooms data: tables, toilet types, load/acco/index, course label, overlap and places in use.
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-rooms-data.php
require __DIR__ . '/lib.php';

t_ok(db_table_exists('dh_center_room') && db_table_exists('dh_center_room_place'), 'D0 both tables exist');
t_eq(dh_room_toilet_types(), array('indian' => 'Indian', 'western' => 'Western', 'both' => 'Both', 'none' => 'No attached toilet', 'unset' => 'Not set'), 'D1 toilet types');

t_in_txn(function () {
  $cen = t_make_centre();
  t_eq(dh_rooms_centre_has_rooms($cen), FALSE, 'D2 a new centre has no rooms');
  $r12 = t_make_room($cen, 'M', 'B', '12', array('12A', '12B'), 'western');
  $r3 = t_make_room($cen, 'M', 'B', '3', array('3'), 'indian');
  $rA = t_make_room($cen, 'M', 'A', '1', array('1'));
  $rF = t_make_room($cen, 'F', 'C', '7', array('7A', '7B', '7C'), 'both');
  t_eq(dh_rooms_centre_has_rooms($cen), TRUE, 'D3 now it has rooms');
  $all = dh_rooms_load($cen);
  t_eq(array_map(function ($r) { return $r['gender'] . '-' . $r['section'] . '-' . $r['name']; }, $all), array('F-C-7', 'M-A-1', 'M-B-3', 'M-B-12'), 'D4 order: gender, section, then natural room name');
  t_eq($all[3]['places'][1]['name'], '12B', 'D5 places in their order');
  t_eq(count(dh_rooms_load($cen, 'F')), 1, 'D6 one side only');
  t_eq(dh_rooms_acco($cen), array('M' => array('A' => array('1'), 'B' => array('3', '12A', '12B')), 'F' => array('C' => array('7A', '7B', '7C'))), 'D7 acco shape like dh_get_acco');
  $idx = dh_rooms_place_index($cen);
  t_eq($idx['M']['B']['12B'], array('room' => '12', 'toilet' => 'western'), 'D8 place index: room and toilet');

  $type3 = db_query("select td_id from dh_type_detail where td_key = '3-DayOSC'")->fetchField();
  $type10 = db_query("select td_id from dh_type_detail where td_key = '10-Day'")->fetchField();
  $d = strtotime('+30 days');
  $A = t_make_course(array('c_center' => $cen, 'c_course_type' => $type10, 'c_start' => date('Y-m-d', $d), 'c_end' => date('Y-m-d', $d + 11 * 86400)));
  $B = t_make_course(array('c_center' => $cen, 'c_course_type' => $type3, 'c_start' => date('Y-m-d', $d + 2 * 86400), 'c_end' => date('Y-m-d', $d + 5 * 86400)));
  $C = t_make_course(array('c_center' => $cen, 'c_course_type' => $type3, 'c_start' => date('Y-m-d', $d + 11 * 86400), 'c_end' => date('Y-m-d', $d + 14 * 86400)));
  $s = $d + 2 * 86400; $e = $d + 5 * 86400;
  $want = '3 Day Course (' . (date('M', $s) === date('M', $e) ? date('j', $s) . '-' . date('j M', $e) : date('j M', $s) . ' - ' . date('j M', $e)) . ')';
  t_eq(dh_rooms_course_label($B), $want, 'D9 course label');
  t_eq(dh_rooms_overlapping_courses($A), array((string) $B), 'D10 B overlaps A; C only shares the changeover day');
  t_make_applicant($B, array('a_gender' => 'M'), array('aa_section' => 'B', 'aa_acco' => '12A'));
  t_eq(dh_rooms_overlap_taken($A), array('M' => array('B' => array('12A' => $want)), 'F' => array()), 'D11 a place B uses shows as taken for A');
  $past = t_make_course(array('c_center' => $cen, 'c_course_type' => $type3));
  t_make_applicant($past, array('a_gender' => 'M'), array('aa_section' => 'B', 'aa_acco' => '3'));
  $use = dh_rooms_places_in_use($cen);
  t_ok(isset($use['M|B|12A']) && !isset($use['M|B|3']), 'D12 places in use: an upcoming course counts, a past one does not');
});
t_done();
