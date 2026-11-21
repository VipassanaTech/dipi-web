<?php
// Rooms page actions (dh_rooms_action): add/edit/add many/toilet/join/split/remove/rename section/clear review, safety rules, Change Log.
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-rooms-actions.php
require __DIR__ . '/lib.php';

t_in_txn(function () {
  $cen = t_make_centre();
  $other = t_make_centre();
  $act = function ($in) use ($cen) { return dh_rooms_action($cen, $in, 0); };
  $names = function () use ($cen) { return array_map(function ($r) { return $r['gender'] . '-' . $r['section'] . '-' . $r['name'] . ':' . implode('+', array_map(function ($p) { return $p['name']; }, $r['places'])) . ':' . $r['toilet']; }, dh_rooms_load($cen)); };

  $r = $act(array('op' => 'save_room', 'id' => 0, 'gender' => 'M', 'section' => 'B', 'name' => '12', 'toilet' => 'western', 'places' => array('12A', '12B')));
  t_ok($r['ok'], 'A1 add a room');
  t_eq($names(), array('M-B-12:12A+12B:western'), 'A2 saved with its places');
  t_eq($act(array('op' => 'save_room', 'id' => 0, 'gender' => 'M', 'section' => 'B', 'name' => '13', 'toilet' => 'indian', 'places' => array('12a')))['msg'], 'Place 12a is already in Room 12.', 'A3 a place name is unique in the section');
  t_eq($act(array('op' => 'save_room', 'id' => 0, 'gender' => 'M', 'section' => 'B', 'name' => '13', 'toilet' => 'gold', 'places' => array('13')))['ok'], FALSE, 'A4 unknown toilet refused');
  t_eq($act(array('op' => 'save_room', 'id' => 0, 'gender' => 'M', 'section' => 'B', 'name' => '13', 'toilet' => 'indian', 'places' => array('0123456789ABCDEF')))['ok'], FALSE, 'A5 a place over 15 letters refused');

  $r = $act(array('op' => 'add_many', 'gender' => 'M', 'section' => 'C', 'from' => 101, 'to' => 103, 'suffixes' => array('A', 'B'), 'toilet' => 'both'));
  t_ok($r['ok'], 'A6 add many');
  t_eq(array_slice($names(), 1), array('M-C-101:101A+101B:both', 'M-C-102:102A+102B:both', 'M-C-103:103A+103B:both'), 'A7 three rooms, two places each');
  t_eq($act(array('op' => 'add_many', 'gender' => 'M', 'section' => 'C', 'from' => 1, 'to' => 300, 'suffixes' => array(), 'toilet' => 'both'))['ok'], FALSE, 'A8 more than 200 rooms at once refused');

  $ids = array_map(function ($r) { return $r['id']; }, dh_rooms_load($cen, 'M'));
  t_ok($act(array('op' => 'set_toilet', 'ids' => array($ids[1], $ids[2]), 'toilet' => 'indian'))['ok'], 'A9 set toilet for two rooms');
  t_eq(array_map(function ($r) { return $r['toilet']; }, dh_rooms_load($cen, 'M')), array('western', 'indian', 'indian', 'both'), 'A10 only those two changed');

  t_ok($act(array('op' => 'join', 'ids' => array($ids[2], $ids[3])))['ok'], 'A11 join 102 and 103');
  t_eq(array_slice($names(), 1), array('M-C-101:101A+101B:indian', 'M-C-102:102A+102B+103A+103B:indian'), 'A12 one room holds all four places');
  t_ok($act(array('op' => 'split', 'id' => $ids[2]))['ok'], 'A13 split it again');
  t_eq(count(dh_rooms_load($cen, 'M')), 6, 'A14 one room per place (12, 101, 102A, 102B, 103A, 103B)');

  $up = t_make_course(array('c_center' => $cen, 'c_start' => date('Y-m-d', strtotime('+5 days')), 'c_end' => date('Y-m-d', strtotime('+15 days'))));
  t_make_applicant($up, array('a_gender' => 'M'), array('aa_section' => 'B', 'aa_acco' => '12A'));
  $room12 = dh_rooms_load($cen, 'M')[0];
  $msg = $act(array('op' => 'save_room', 'id' => $room12['id'], 'gender' => 'M', 'section' => 'B', 'name' => '12', 'toilet' => 'western', 'places' => array('12B')))['msg'];
  t_ok(strpos($msg, 'Place 12A is used by') === 0 && strpos($msg, "It can't be changed until that course is over.") !== FALSE, 'A15 a place in use cannot be removed from the room');
  t_eq($act(array('op' => 'remove', 'id' => $room12['id']))['ok'], FALSE, 'A16 a room with a place in use cannot be removed');
  t_eq($act(array('op' => 'rename_section', 'gender' => 'M', 'from' => 'B', 'to' => 'B1'))['ok'], FALSE, 'A17 a section with a place in use cannot be renamed');
  t_ok($act(array('op' => 'save_room', 'id' => $room12['id'], 'gender' => 'M', 'section' => 'B', 'name' => 'Twelve', 'toilet' => 'none', 'places' => array('12A', '12B', '12C')))['ok'], 'A18 renaming the room, changing toilet and adding a place is fine');
  t_eq($act(array('op' => 'rename_section', 'gender' => 'M', 'from' => 'C', 'to' => 'B'))['msg'], 'Section B already exists.', 'A19 rename to an existing section refused');
  t_ok($act(array('op' => 'rename_section', 'gender' => 'M', 'from' => 'C', 'to' => 'C Block'))['ok'], 'A20 rename a free section');
  $men = dh_rooms_load($cen, 'M');
  $last = end($men);
  t_ok($act(array('op' => 'remove', 'id' => $last['id']))['ok'], 'A21 remove a room not in use');

  $foreign = t_make_room($other, 'M', 'B', '1', array('1'));
  t_eq($act(array('op' => 'set_toilet', 'ids' => array($foreign), 'toilet' => 'both'))['msg'], 'This room is not in this centre.', 'A22 another centre\'s room is refused');

  db_update('dh_center_room')->fields(array('cr_review' => 1))->condition('cr_center', $cen)->execute();
  t_ok($act(array('op' => 'clear_review', 'ids' => array($room12['id'])))['ok'], 'A23 clear "needs review"');
  t_eq((int) db_query("select cr_review from dh_center_room where cr_id = :r", array(':r' => $room12['id']))->fetchField(), 0, 'A24 cleared');
  t_ok((int) db_query("select count(*) from dh_center_log where cl_center = :c and cl_area = 'rooms'", array(':c' => $cen))->fetchField() >= 10, 'A25 changes are in the Change Log');
});
t_done();
