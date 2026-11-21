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
  t_eq($act(array('op' => 'add_many', 'gender' => 'M', 'section' => 'Z', 'from' => 1, 'to' => 11, 'suffixes' => array('1', '11'), 'toilet' => 'both'))['msg'], 'Place 111 is already in Room 1.', 'A8b a place repeated inside one add is refused');
  t_eq((int) db_query("select count(*) from dh_center_room where cr_center = :c and cr_section = 'Z' and cr_deleted = 0", array(':c' => $cen))->fetchField(), 0, 'A8c nothing was added');

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
  $twelveD = t_make_room($cen, 'M', 'B', '12d', array('12d'));
  t_make_applicant($up, array('a_gender' => 'M'), array('aa_section' => 'B', 'aa_acco' => '12D'));
  t_ok(strpos($act(array('op' => 'remove', 'id' => $twelveD))['msg'], 'Place 12d is used by') === 0, 'A21b a place in use is refused whatever the letter case');
  $bBefore = (int) db_query("select count(*) from dh_center_room where cr_center = :c and cr_gender = 'M' and cr_section = 'B' and cr_deleted = 0", array(':c' => $cen))->fetchField();
  t_eq($act(array('op' => 'rename_section', 'gender' => 'M', 'from' => 'b', 'to' => 'Bx'))['ok'], FALSE, 'A21c a section in use is refused when renamed in lower case');
  t_eq((int) db_query("select count(*) from dh_center_room where cr_center = :c and cr_gender = 'M' and cr_section = 'B' and cr_deleted = 0", array(':c' => $cen))->fetchField(), $bBefore, 'A21c-b section B still has its rooms');

  $foreign = t_make_room($other, 'M', 'B', '1', array('1'));
  t_eq($act(array('op' => 'set_toilet', 'ids' => array($foreign), 'toilet' => 'both'))['msg'], 'This room is not in this centre.', 'A22 another centre\'s room is refused');

  db_update('dh_center_room')->fields(array('cr_review' => 1))->condition('cr_center', $cen)->execute();
  t_ok($act(array('op' => 'clear_review', 'ids' => array($room12['id'])))['ok'], 'A23 clear "needs review"');
  t_eq((int) db_query("select cr_review from dh_center_room where cr_id = :r", array(':r' => $room12['id']))->fetchField(), 0, 'A24 cleared');
  t_ok((int) db_query("select count(*) from dh_center_log where cl_center = :c and cl_area = 'rooms'", array(':c' => $cen))->fetchField() >= 10, 'A25 changes are in the Change Log');

  // Final review: letter case, '|', order of new rooms.
  $c2 = t_make_centre();
  $a2 = function ($in) use ($c2) { return dh_rooms_action($c2, $in, 0); };
  $sec2 = function () use ($c2) { return db_query("select group_concat(distinct binary cr_section) from dh_center_room where cr_center = :c and cr_deleted = 0", array(':c' => $c2))->fetchField(); };
  t_make_room($c2, 'M', 'B', '1', array('1'));
  $r = $a2(array('op' => 'save_room', 'id' => 0, 'gender' => 'M', 'section' => 'b', 'name' => '2', 'toilet' => 'western', 'places' => array('2')));
  t_ok($r['ok'] && $sec2() === 'B', 'A1b a section typed in another letter case keeps the existing spelling (save_room)');
  $r = $a2(array('op' => 'add_many', 'gender' => 'M', 'section' => 'b', 'from' => 5, 'to' => 6, 'suffixes' => array(), 'toilet' => 'both'));
  t_ok($r['ok'] && $sec2() === 'B', 'A1b-2 same for add_many');
  t_eq($a2(array('op' => 'save_room', 'id' => 0, 'gender' => 'M', 'section' => 'B|1', 'name' => '9', 'toilet' => 'western', 'places' => array('9')))['msg'], 'Section can\'t contain "|".', 'A1c save_room refuses |');
  t_eq($a2(array('op' => 'add_many', 'gender' => 'M', 'section' => 'B|1', 'from' => 7, 'to' => 7, 'toilet' => 'both'))['msg'], 'Section can\'t contain "|".', 'A1c-2 add_many refuses |');
  t_eq($a2(array('op' => 'rename_section', 'gender' => 'M', 'from' => 'B', 'to' => 'B|1'))['msg'], 'Section can\'t contain "|".', 'A1c-3 rename_section refuses |');

  $up2 = t_make_course(array('c_center' => $c2, 'c_start' => date('Y-m-d', strtotime('+5 days')), 'c_end' => date('Y-m-d', strtotime('+15 days'))));
  $r12 = t_make_room($c2, 'M', 'D', '12', array('12A'));
  t_make_applicant($up2, array('a_gender' => 'M'), array('aa_section' => 'D', 'aa_acco' => '12A'));
  $m = $a2(array('op' => 'save_room', 'id' => $r12, 'gender' => 'M', 'section' => 'D', 'name' => '12', 'toilet' => 'western', 'places' => array('12a')))['msg'];
  t_ok(strpos($m, 'Place 12A is used by') === 0, 'A21d a case-only rename of an in-use place is refused');

  $a2(array('op' => 'add_many', 'gender' => 'F', 'section' => 'C', 'from' => 101, 'to' => 103, 'suffixes' => array(), 'toilet' => 'both'));
  t_ok($a2(array('op' => 'save_room', 'id' => 0, 'gender' => 'F', 'section' => 'C', 'name' => '50', 'toilet' => 'both', 'places' => array('50')))['ok'], 'A26 add room 50 later');
  t_eq(array_map(function ($r) { return $r['name']; }, dh_rooms_load($c2, 'F')), array('101', '102', '103', '50'), 'A27 a later new room sorts after the earlier ones');

  // Centre not converted yet: old typed list.
  $c3 = t_make_centre();
  $a3 = function ($in) use ($c3) { return dh_rooms_action($c3, $in, 0); };
  t_eq(dh_rooms_has_old_list($c3), FALSE, 'A30 no typed list, no old list');
  db_insert('dh_center_setting_acco')->fields(array('csa_center' => $c3, 'csa_gender' => 'M', 'csa_section' => 'B', 'csa_room' => '1, 2', 'csa_deleted' => 0))->execute();
  t_eq(dh_rooms_has_old_list($c3), TRUE, 'A31 a typed list is an old list');
  $old = 'This centre still uses its old room list. Use "Bring in the old room list" first.';
  t_eq($a3(array('op' => 'save_room', 'id' => 0, 'gender' => 'M', 'section' => 'B', 'name' => '3', 'toilet' => 'western', 'places' => array('3')))['msg'], $old, 'A32 new room refused while old list');
  t_eq($a3(array('op' => 'add_many', 'gender' => 'M', 'section' => 'B', 'from' => 3, 'to' => 4, 'toilet' => 'both'))['msg'], $old, 'A33 add_many refused while old list');
  $r = $a3(array('op' => 'import_old'));
  t_ok($r['ok'] && strpos($r['msg'], '2 rooms and 2 places brought in') === 0, 'A34 import ok: ' . $r['msg']);
  t_eq(dh_rooms_has_old_list($c3), FALSE, 'A35 old list gone once rooms exist');
  t_ok($a3(array('op' => 'save_room', 'id' => 0, 'gender' => 'M', 'section' => 'B', 'name' => '3', 'toilet' => 'western', 'places' => array('3')))['ok'], 'A36 save_room works now');
  t_eq($a3(array('op' => 'import_old'))['msg'], 'Rooms are already set up for this centre.', 'A37 import twice refused');
});
t_done();
