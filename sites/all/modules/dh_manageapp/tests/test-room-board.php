<?php
// Room Board: data (rooms, people with hints, overlap) and actions (place, move, swap, remove) with all refusals.
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-room-board.php
require __DIR__ . '/lib.php';

$keep = $GLOBALS['user'];
t_in_txn(function () {
  global $user;
  $user = user_load(1);
  drupal_static_reset('user_access');
  $cen = t_make_centre();
  t_make_room($cen, 'M', 'B', '12', array('12A', '12B'), 'western');
  t_make_room($cen, 'M', 'B', '14', array('14A', '14B'), 'indian');
  t_make_room($cen, 'F', 'D', '7', array('7A'), 'both');
  $d = strtotime('+20 days');
  $A = t_make_course(array('c_center' => $cen, 'c_start' => date('Y-m-d', $d), 'c_end' => date('Y-m-d', $d + 11 * 86400)));
  $B = t_make_course(array('c_center' => $cen, 'c_start' => date('Y-m-d', $d + 2 * 86400), 'c_end' => date('Y-m-d', $d + 5 * 86400)));
  $co = db_query("select c_code, c_name, c_nationality from dh_country where c_nationality <> '' order by c_id limit 1")->fetchAssoc();
  db_insert('dh_state')->fields(array('s_country' => $co['c_code'], 's_name' => 'Zzqtest State', 's_code' => 'ZQS'))->execute();
  $ram = t_make_applicant($A, array('a_gender' => 'M', 'a_f_name' => 'ram', 'a_l_name' => 'kumar', 'a_old' => 1, 'a_dob' => (date('Y') - 70) . '-01-01', 'a_problem_physical' => 1,
    'a_occupation' => 'RETIRED TEACHER', 'a_education' => ' M.A. ', 'a_country' => $co['c_code'], 'a_state' => 'ZQS'),
    array('aa_section' => 'B', 'aa_acco' => '12A', 'aa_chair' => 1));
  $shyam = t_make_applicant($A, array('a_gender' => 'M', 'a_f_name' => 'shyam', 'a_type' => 'Sevak', 'a_country' => $co['c_code']));
  $gita = t_make_applicant($A, array('a_gender' => 'F', 'a_f_name' => 'gita', 'a_country' => $co['c_code']));
  db_insert('dh_applicant_extra')->fields(array('ae_applicant' => $ram, 'ae_created_by' => 0, 'ae_updated_by' => 0, 'ae_created' => date('Y-m-d H:i:s'), 'ae_updated' => date('Y-m-d H:i:s'), 'ae_desc_physical' => 'knee pain',
    'ae_desc_special_requirement' => 'needs a western toilet', 'ae_nationality' => strtoupper($co['c_nationality'])))->execute();
  db_insert('dh_applicant_extra')->fields(array('ae_applicant' => $gita, 'ae_created_by' => 0, 'ae_updated_by' => 0, 'ae_created' => date('Y-m-d H:i:s'), 'ae_updated' => date('Y-m-d H:i:s'), 'ae_nationality' => 'Zzqlandish'))->execute();
  t_make_applicant($B, array('a_gender' => 'M'), array('aa_section' => 'B', 'aa_acco' => '14B'));

  $old = t_make_applicant($A, array('a_gender' => 'M', 'a_f_name' => 'old'), array('aa_section' => 'B', 'aa_acco' => ' 14A'));

  $data = dh_room_board_data($cen, $A);
  t_eq(array($data['ok'], count($data['rooms']), count($data['people'])), array(TRUE, 3, 4), 'B1 data: rooms of both sides, three arrived people');
  $p = array(); foreach ($data['people'] as $x) $p[$x['a_id']] = $x;
  t_eq(array($p[$ram]['name'], $p[$ram]['old'], $p[$ram]['section'], $p[$ram]['place']), array('Ram Kumar', TRUE, 'B', '12A'), 'B2 a placed person');
  t_ok(in_array('Chair', $p[$ram]['hints'], TRUE) && in_array('Physical: knee pain', $p[$ram]['hints'], TRUE) && in_array('Needs: needs a western toilet', $p[$ram]['hints'], TRUE), 'B3 hints: chair, physical problem, special needs');
  t_eq(array($p[$ram]['occupation'], $p[$ram]['education'], $p[$ram]['state'], $p[$ram]['country'], $p[$ram]['nationality']),
    array('Retired Teacher', 'M.A.', 'Zzqtest State', $co['c_name'], ''), 'B3b occupation, education, state, country; nationality hidden when it matches the country');
  t_eq(array($p[$gita]['nationality'], $p[$shyam]['state']), array('Zzqlandish', ''), 'B3c a different nationality is shown; no state = empty');
  t_eq(array($p[$shyam]['server'], $p[$shyam]['place']), array(TRUE, ''), 'B4 a server not placed yet');
  t_ok(isset($data['overlap']['M']['B']['14B']), 'B5 the other course\'s place shows as taken');

  t_eq($p[$old]['place'], '14A', 'B5b saved place with a leading space is trimmed on the board');

  $ap = function ($in) use ($cen, $A) { return dh_room_board_apply($cen, $A, $in, 1); };
  $place = function ($a) { return db_query("select concat(aa_section, '-', aa_acco) from dh_applicant_attended where aa_applicant = :a", array(':a' => $a))->fetchField(); };

  $r = $ap(array('op' => 'place', 'a_id' => $shyam, 'section' => 'B', 'place' => '14A'));
  t_ok(!$r['ok'] && strpos($r['msg'], 'Place 14A was just given to') === 0, 'B5b a place saved with a leading space is still seen as taken');
  t_ok($ap(array('op' => 'place', 'a_id' => $shyam, 'section' => 'B', 'place' => '12B'))['ok'], 'B6 place a server');
  t_eq($place($shyam), 'B-12B', 'B7 saved as section + place');
  t_eq($ap(array('op' => 'place', 'a_id' => $gita, 'section' => 'B', 'place' => '14A'))['msg'], 'There is no place 14A in section B.', 'B8 a woman cannot be put in a men\'s room');
  $r = $ap(array('op' => 'place', 'a_id' => $shyam, 'section' => 'B', 'place' => '12A'));
  t_ok(!$r['ok'] && strpos($r['msg'], 'Place 12A was just given to Ram Kumar.') === 0, 'B9 a taken place is refused, naming who has it');
  $r = $ap(array('op' => 'place', 'a_id' => $shyam, 'section' => 'B', 'place' => '14B'));
  t_ok(!$r['ok'] && $r['confirm'] && strpos($r['msg'], 'Place 14B is used by') === 0, 'B10 a place of an overlapping course asks first');
  t_eq($place($shyam), 'B-12B', 'B11 nothing changed yet');
  t_ok($ap(array('op' => 'place', 'a_id' => $shyam, 'section' => 'B', 'place' => '14B', 'confirm' => '1'))['ok'], 'B12 "Use it anyway" places him');
  t_ok($ap(array('op' => 'swap', 'a_id' => $ram, 'other' => $shyam))['ok'], 'B13 swap two placed people');
  t_eq(array($place($ram), $place($shyam)), array('B-14B', 'B-12A'), 'B14 their places are exchanged');
  t_eq($ap(array('op' => 'swap', 'a_id' => $ram, 'other' => $gita))['msg'], 'Only people on the same side can swap.', 'B15 swap across sides refused');
  t_ok($ap(array('op' => 'remove', 'a_id' => $ram))['ok'], 'B16 remove from room');
  t_eq($place($ram), '-', 'B17 room emptied');
  t_ok((int) db_query("select count(*) from dh_log where l_event = 'Room' and l_identifier in (:a)", array(':a' => array((string) $ram, (string) $shyam)))->fetchField() >= 4, 'B18 every change is logged');
  $stranger = t_make_applicant($B, array('a_gender' => 'M'));
  t_eq($ap(array('op' => 'remove', 'a_id' => $stranger))['msg'], 'This person has not arrived for this course.', 'B19 a person of another course is refused');
  // B22: a user with only the men's side cannot touch a woman, and sees no women or women's overlap.
  t_make_applicant($B, array('a_gender' => 'F'), array('aa_section' => 'D', 'aa_acco' => '7A'));
  $role = new stdClass(); $role->name = 'zzq room men ' . mt_rand(1000, 9999); user_role_save($role);
  user_role_grant_permissions($role->rid, array('access zero day', 'access male'));
  $men = (object) array('uid' => 999999, 'name' => 'zzqmen', 'status' => 1, 'roles' => array(DRUPAL_AUTHENTICATED_RID => 'authenticated user', $role->rid => $role->name));
  $user = $men; drupal_static_reset('user_access'); drupal_static_reset('user_role_permissions');
  t_eq(dh_room_board_apply($cen, $A, array('op' => 'remove', 'a_id' => $gita), 999999)['msg'], 'You do not have access to this person.', 'B22 a men-only user cannot change a woman');
  t_eq(dh_room_board_apply($cen, $A, array('op' => 'swap', 'a_id' => $ram, 'other' => $gita), 999999)['msg'], 'You do not have access to this person.', 'B22b swap with a woman refused');
  $md = dh_room_board_data($cen, $A);
  $fem = 0; foreach ($md['people'] as $x) if ($x['gender'] === 'F') $fem++;
  t_eq(array($md['sides'], $fem, isset($md['overlap']['F'])), array(array('M'), 0, FALSE), 'B22c men-only data: no women, no women\'s overlap');
  $user = user_load(1); drupal_static_reset('user_access'); drupal_static_reset('user_role_permissions');
  t_eq($place($gita), NULL, 'B22d the woman\'s room is unchanged');

  db_update('dh_course')->fields(array('c_finalized' => 1))->condition('c_id', $A)->execute();
  t_eq($ap(array('op' => 'remove', 'a_id' => $shyam))['msg'], "This course is finalized. Rooms can't be changed here.", 'B20 finalized course is read-only');
  $other = t_make_centre();
  t_eq(dh_room_board_data($cen, $A)['has_rooms'], TRUE, 'B22 has_rooms true for a centre with rooms');
  $cB = t_make_course(array('c_center' => $other));
  t_eq(dh_room_board_data($other, $cB)['has_rooms'], FALSE, 'B23 has_rooms false for a centre without rooms');
  t_eq(dh_room_board_apply($other, $A, array('op' => 'remove', 'a_id' => $shyam), 1)['msg'], 'This course is not in this centre.', 'B21 course of another centre refused');
});
$GLOBALS['user'] = $keep;
t_done();
