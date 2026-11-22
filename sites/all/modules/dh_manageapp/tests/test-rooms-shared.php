<?php
// Shared sections (cr_gender 'B' = used by men and women): lists, taken places, the "other side here" warning,
// Rooms page actions (Used by, join a same-name section, delete a whole section), the Room Board, and the Change Log.
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-rooms-shared.php
require __DIR__ . '/lib.php';

$keep = $GLOBALS['user'];
t_in_txn(function () {
  global $user;
  $user = user_load(1);
  drupal_static_reset('user_access');

  // ---- lists and taken places ----
  $cen = t_make_centre();
  t_make_room($cen, 'M', 'A', '1', array('1'));
  t_make_room($cen, 'B', 'C', '5', array('5A', '5B'), 'western');
  t_make_room($cen, 'B', 'C', '6', array('6'));
  t_make_room($cen, 'F', 'D', '9', array('9'));
  t_eq(array(dh_rooms_side_label('M'), dh_rooms_side_label('F'), dh_rooms_side_label('B')), array('Male', 'Female', 'Shared'), 'S1 side labels');
  t_eq(dh_rooms_shared_sections($cen), array('C' => 'C'), 'S2 shared sections of the centre');
  t_ok(dh_rooms_is_shared(dh_rooms_shared_sections($cen), ' c ') && !dh_rooms_is_shared(dh_rooms_shared_sections($cen), 'A'), 'S2b shared check ignores spaces and letter case');
  t_eq(dh_rooms_acco($cen), array('M' => array('A' => array('1'), 'C' => array('5A', '5B', '6')), 'F' => array('C' => array('5A', '5B', '6'), 'D' => array('9'))),
    'S3 a shared section is listed for men and for women, in section order');
  $idx = dh_rooms_place_index($cen);
  t_eq(array($idx['M']['C']['5B'], $idx['F']['C']['5B']), array(array('room' => '5', 'toilet' => 'western'), array('room' => '5', 'toilet' => 'western')), 'S4 place index: shared places on both sides');

  $d = strtotime('-1 day');
  $A = t_make_course(array('c_center' => $cen, 'c_start' => date('Y-m-d', $d), 'c_end' => date('Y-m-d', $d + 10 * 86400)));
  $B = t_make_course(array('c_center' => $cen, 'c_start' => date('Y-m-d', $d + 2 * 86400), 'c_end' => date('Y-m-d', $d + 5 * 86400)));
  $gita = t_make_applicant($A, array('a_gender' => 'F', 'a_f_name' => 'gita'), array('aa_section' => 'C', 'aa_acco' => ' 5A'));
  t_make_applicant($A, array('a_gender' => 'F'), array('aa_section' => 'D', 'aa_acco' => '9'));
  $al = dh_get_alloted($cen, $A);
  t_ok(in_array('5A', $al['M']['C'], TRUE) && in_array('5A', $al['F']['C'], TRUE) && !isset($al['M']['D']), 'S5 Zero Day taken list: a woman in a shared place is taken for men too; a women-only place is not');
  t_make_applicant($B, array('a_gender' => 'M'), array('aa_section' => 'C', 'aa_acco' => '6'));
  $ov = dh_rooms_overlap_taken($A);
  t_ok(isset($ov['M']['C']['6'], $ov['F']['C']['6']), 'S6 a man of an overlapping course in a shared place: taken for both sides');
  $use = dh_rooms_places_in_use($cen);
  t_ok(isset($use['F|C|5A'], $use['B|C|5A'], $use['M|C|6'], $use['B|C|6']) && !isset($use['B|D|9']), 'S7 places in use: shared places also as B|section|place');
  t_eq(dh_rooms_shared_mix($A, 'C', 'M'), 'this course', 'S8 a man going into shared C: women are there in this course');
  t_eq(dh_rooms_shared_mix($A, 'C', 'F'), dh_rooms_course_label($B), 'S8b a woman going into shared C: a man is there in the overlapping course');
  t_eq(array(dh_rooms_shared_mix($A, 'A', 'F'), dh_rooms_shared_mix($A, 'D', 'M')), array('', ''), 'S8c not shared: never a mix warning');
  t_eq(dh_rooms_mix_question('C', 'M', 'this course'), 'Section C has female students in this course. Place a male student here anyway?', 'S9 warning text (same course)');
  t_eq(dh_rooms_mix_question('C', 'F', '3 Day Course (4-7 Dec)'), 'Section C has male students in 3 Day Course (4-7 Dec) at the same time. Place a female student here anyway?', 'S9b warning text (other course)');
  t_eq(_ajax_get_section_options('F', $cen), array('' => 'Choose', 'C' => 'C', 'D' => 'D'), 'S10 application form: women see the shared section');
  t_eq(_ajax_get_acco_options('C', 'M', $cen, $A), array('' => 'Choose', '5B' => '5B', '6' => '6'), 'S10b application form: a place a woman holds in a shared section is taken for men');

  // ---- Room Board ----
  $ram = t_make_applicant($A, array('a_gender' => 'M', 'a_f_name' => 'ram'), array('aa_section' => '', 'aa_acco' => ''));
  $data = dh_room_board_data($cen, $A);
  t_eq(count(array_filter($data['rooms'], function ($r) { return $r['gender'] === 'B'; })), 2, 'B1 board: shared rooms are in the data');
  t_eq($data['cross']['M']['C']['5A'], 'Female', 'B2 board: on the male side a shared place a female student holds shows as "Female"');
  $apply = function ($in) use ($cen, $A) { return dh_room_board_apply($cen, $A, $in, 1); };
  t_eq($apply(array('op' => 'place', 'a_id' => $ram, 'section' => 'C', 'place' => '5A'))['msg'], 'Place 5A is taken by a female student in this course.', 'B3 a shared place a woman holds is refused for a man');
  $r = $apply(array('op' => 'place', 'a_id' => $ram, 'section' => 'C', 'place' => '5B'));
  t_ok($r['confirm'] && $r['msg'] === 'Room 5 has female students in this course. Place a male student here anyway?', 'B4 a man into a room of a shared section where a woman is: asks first (' . $r['msg'] . ')');
  $r = $apply(array('op' => 'place', 'a_id' => $ram, 'section' => 'C', 'place' => '6'));
  t_ok($r['confirm'] && strpos($r['msg'], 'Section C has female students in this course.') !== FALSE, 'B4b a man into another room of a shared section with women: section warning (' . $r['msg'] . ')');
  t_ok($apply(array('op' => 'place', 'a_id' => $ram, 'section' => 'C', 'place' => '5B', 'confirm' => 1))['ok'], 'B5 with confirm it is saved');
  $r = $apply(array('op' => 'place', 'a_id' => $ram, 'section' => 'C', 'place' => '6'));
  t_ok($r['confirm'] && strpos($r['msg'], 'Place 6 is used by ') === 0 && strpos($r['msg'], 'Section C has') === FALSE, 'B6 moving inside the same shared section: only the overlap is asked, not the mix (' . $r['msg'] . ')');
  $sita = t_make_applicant($A, array('a_gender' => 'F'), array('aa_section' => '', 'aa_acco' => ''));
  $mohan = t_make_applicant($A, array('a_gender' => 'M'), array('aa_section' => 'C', 'aa_acco' => ''));
  t_eq(dh_rooms_mix_warning($A, 'C', '6', 'M', 'C', ''), 'Section C has female students in this course.', 'B6b a section saved without a place still counts as entering: asked');
  t_eq(dh_rooms_mix_warning($A, 'C', '5B', 'M', 'C', '5B'), '', 'B6c the same place: nothing to ask');
  t_make_room($cen, 'B', 'C', '8', array('8A', '8B'));
  $hari = t_make_applicant($A, array('a_gender' => 'M'), array('aa_section' => 'C', 'aa_acco' => '8A'));
  t_eq(dh_rooms_mix_warning($A, 'C', '5B', 'M', 'C', '8A'), 'Room 5 has female students in this course.', 'B6d inside the section, into a room where a woman is: asked');
  t_eq(dh_rooms_mix_warning($A, 'C', '8B', 'M', 'C', '8A'), '', 'B6e inside the same room: not asked');
  t_eq(dh_rooms_mix_warning($A, 'D', '9', 'F', '', ''), '', 'B6f not a shared section: never asked');
  $r = $apply(array('op' => 'place', 'a_id' => $sita, 'section' => 'C', 'place' => '6'));
  t_ok($r['confirm'] && strpos($r['msg'], 'Place 6 is used by ') === 0 && strpos($r['msg'], 'Section C has male students in this course.') !== FALSE && substr($r['msg'], -15) === ' Use it anyway?', 'B7 overlap + mix: one question (' . $r['msg'] . ')');

  // ---- Rooms page: Used by ----
  $c2 = t_make_centre();
  $act = function ($in) use ($c2) { return dh_rooms_action($c2, $in, 0); };
  $sides = function ($sec) use ($c2) { return db_query("select group_concat(distinct cr_gender order by cr_gender) from dh_center_room where cr_center = :c and cr_section = :s and cr_deleted = 0", array(':c' => $c2, ':s' => $sec))->fetchField(); };
  $logs = function ($field) use ($c2) { return db_query("select cl_action, cl_old, cl_new from dh_center_log where cl_center = :c and cl_area = 'rooms' and cl_field = :f order by cl_id", array(':c' => $c2, ':f' => $field))->fetchAll(PDO::FETCH_ASSOC); };
  t_make_room($c2, 'M', 'E', '1', array('1A', '1B'));
  t_make_room($c2, 'M', 'E', '2', array('2'));
  $r = $act(array('op' => 'set_used_by', 'gender' => 'M', 'section' => 'e', 'to' => 'B'));
  t_ok($r['ok'] && $sides('E') === 'B' && $r['msg'] === 'Section E is now for male and female students.', 'U1 men\'s section E made shared (' . $r['msg'] . ')');
  t_eq($logs('used_by'), array(array('cl_action' => 'update', 'cl_old' => 'Section E: Male', 'cl_new' => 'Male and Female')), 'U1b Change Log: Used by');
  t_ok($act(array('op' => 'save_room', 'id' => 0, 'gender' => 'F', 'section' => 'E', 'name' => '3', 'toilet' => 'indian', 'places' => array('3')))['ok'] && $sides('E') === 'B', 'U2 a room added from the women\'s tab goes into the shared section');
  t_ok($act(array('op' => 'add_many', 'gender' => 'F', 'section' => 'E', 'from' => 4, 'to' => 5, 'suffixes' => array(), 'toilet' => 'both'))['ok'] && $sides('E') === 'B', 'U3 add many from the women\'s tab: shared too');
  $e1 = (int) db_query("select cr_id from dh_center_room where cr_center = :c and cr_name = '1' and cr_deleted = 0", array(':c' => $c2))->fetchField();
  t_ok($act(array('op' => 'save_room', 'id' => $e1, 'gender' => 'F', 'section' => 'E', 'name' => '1', 'toilet' => 'none', 'places' => array('1A', '1B')))['ok'] && $sides('E') === 'B', 'U4 editing a shared room from the women\'s tab keeps it shared');
  t_eq($act(array('op' => 'save_room', 'id' => 0, 'gender' => 'M', 'section' => 'E', 'name' => '6', 'toilet' => 'none', 'places' => array('1a')))['msg'], 'Place 1a is already in Room 1.', 'U4b a place name is still unique in a shared section');
  t_make_room($c2, 'F', 'G', '1', array('1'));
  t_eq($act(array('op' => 'rename_section', 'gender' => 'F', 'from' => 'E', 'to' => 'G'))['msg'], 'Section G already exists.', 'U5 a shared section can\'t take a women\'s section name');
  t_make_room($c2, 'M', 'H', '1', array('1'));
  t_eq($act(array('op' => 'rename_section', 'gender' => 'M', 'from' => 'H', 'to' => 'E'))['msg'], 'Section E already exists.', 'U5b a men\'s section can\'t take a shared section name');
  t_ok($act(array('op' => 'rename_section', 'gender' => 'F', 'from' => 'E', 'to' => 'E2'))['ok'] && $sides('E2') === 'B', 'U5c rename a shared section from the women\'s tab');

  $d2 = strtotime('-1 day');
  $run = t_make_course(array('c_center' => $c2, 'c_start' => date('Y-m-d', $d2), 'c_end' => date('Y-m-d', $d2 + 10 * 86400)));
  $w = t_make_applicant($run, array('a_gender' => 'F'), array('aa_section' => 'E2', 'aa_acco' => '3'));
  $r = $act(array('op' => 'set_used_by', 'gender' => 'M', 'section' => 'E2', 'to' => 'M'));
  t_ok(!$r['ok'] && strpos($r['msg'], 'Female students are placed in section E2 in ') === 0 && $sides('E2') === 'B', 'U6 shared -> men only refused while a woman is placed there (' . $r['msg'] . ')');
  db_delete('dh_applicant_attended')->condition('aa_applicant', $w)->execute();
  t_make_applicant($run, array('a_gender' => 'M'), array('aa_section' => 'E2', 'aa_acco' => '3'));
  t_ok(strpos($act(array('op' => 'set_used_by', 'gender' => 'F', 'section' => 'E2', 'to' => 'F'))['msg'], 'Male students are placed in section E2 in ') === 0 && $sides('E2') === 'B', 'U6a shared -> women only refused while a man is placed there');
  t_ok($act(array('op' => 'set_used_by', 'gender' => 'F', 'section' => 'E2', 'to' => 'M'))['ok'] && $sides('E2') === 'M', 'U6b shared -> men only is fine when only men are placed there');
  t_ok(strpos($act(array('op' => 'set_used_by', 'gender' => 'M', 'section' => 'E2', 'to' => 'F'))['msg'], 'Male students are placed in section E2 in ') === 0, 'U7 men only -> women only refused while men are placed there');
  t_make_room($c2, 'M', 'G', '2', array('2'));
  t_eq($act(array('op' => 'set_used_by', 'gender' => 'M', 'section' => 'H', 'to' => 'F'))['ok'], TRUE, 'U7b men only -> women only for a free section');
  t_eq($act(array('op' => 'set_used_by', 'gender' => 'F', 'section' => 'H', 'to' => 'M'))['ok'], TRUE, 'U7c and back');
  t_eq($act(array('op' => 'set_used_by', 'gender' => 'M', 'section' => 'G', 'to' => 'F'))['msg'], 'Female students already have a section G. Rename one of them first.', 'U7d women only refused when women already have that section name');
  t_eq($act(array('op' => 'set_used_by', 'gender' => 'M', 'section' => 'Nope', 'to' => 'B'))['msg'], 'There is no section Nope.', 'U7e unknown section');
  t_eq($act(array('op' => 'set_used_by', 'gender' => 'M', 'section' => 'H', 'to' => 'X'))['ok'], FALSE, 'U7f unknown side refused');

  // Join a same-name section of the other side.
  t_make_room($c2, 'M', 'J', '1', array('1A', '1B'));
  t_make_room($c2, 'M', 'J', '2', array('2'));
  $fj = t_make_room($c2, 'F', 'J', '1', array('1a', '4'));
  $r = $act(array('op' => 'set_used_by', 'gender' => 'M', 'section' => 'J', 'to' => 'B'));
  t_eq($r['msg'], "Female students' section J also has place 4, not in this section. Make the two lists the same first, or rename one of them.", 'U8 join refused: the women\'s list has a place the men\'s does not');
  db_update('dh_center_room_place')->fields(array('crp_name' => '2'))->condition('crp_room', $fj)->condition('crp_name', '4')->execute();
  t_eq($act(array('op' => 'set_used_by', 'gender' => 'M', 'section' => 'J', 'to' => 'B'))['msg'], "Female students' section J writes place 1a with other capital letters than this section. Make the two lists exactly the same first.",
    'U10a join refused when a place differs only in capital letters (1a / 1A)');
  db_update('dh_center_room_place')->fields(array('crp_name' => '1A'))->condition('crp_room', $fj)->condition('crp_name', '1a')->execute();
  $m1 = t_make_applicant($run, array('a_gender' => 'M'), array('aa_section' => 'J', 'aa_acco' => '2'));
  $w1 = t_make_applicant($run, array('a_gender' => 'F'), array('aa_section' => 'J', 'aa_acco' => '2'));
  t_ok(strpos($act(array('op' => 'set_used_by', 'gender' => 'F', 'section' => 'J', 'to' => 'B'))['msg'], "Male students' section J also has place 1B, not in this section.") === 0, 'U8b from the women\'s tab the men\'s list must fit into the women\'s');
  $r = $act(array('op' => 'set_used_by', 'gender' => 'M', 'section' => 'J', 'to' => 'B'));
  t_ok(!$r['ok'] && strpos($r['msg'], 'Place 2 has a male and a female student in it now (') === 0 && strpos($r['msg'], 'two different buildings') !== FALSE, 'U9 join refused when one place has a man and a woman now (' . $r['msg'] . ')');
  db_delete('dh_applicant_attended')->condition('aa_applicant', $m1)->execute();
  $r = $act(array('op' => 'set_used_by', 'gender' => 'M', 'section' => 'J', 'to' => 'B'));
  t_ok($r['ok'] && $sides('J') === 'B' && $r['msg'] === "Section J is now for male and female students. Female students' section J was joined into it.", 'U10 join: the women\'s J joins the shared J (' . $r['msg'] . ')');
  t_eq(array_map(function ($r) { return $r['name'] . ':' . implode('+', array_map(function ($p) { return $p['name']; }, $r['places'])); }, array_values(array_filter(dh_rooms_load($c2), function ($r) { return $r['section'] === 'J'; }))),
    array('1:1A+1B', '2:2'), 'U10b the men\'s rooms stay, the women\'s copy is gone');
  $j = $logs('section');
  $last = end($j);
  t_ok($last['cl_old'] === 'Female J (1 room: 1: 1A, 2)' && $last['cl_new'] === 'joined into shared section J', 'U10c Change Log: what was joined (' . $last['cl_old'] . ')');
  t_eq(dh_room_board_data($c2, $run)['cross']['M']['J']['2'], 'Female', 'U10d the woman placed in J-2 is still placed (the shared J has place 2)');

  // Delete a whole section.
  $r = $act(array('op' => 'delete_section', 'gender' => 'M', 'section' => 'J'));
  t_ok(!$r['ok'] && strpos($r['msg'], 'Place 2 is used by ') === 0, 'D1 a section with a place in use cannot be deleted (' . $r['msg'] . ')');
  db_delete('dh_applicant_attended')->condition('aa_applicant', $w1)->execute();
  $r = $act(array('op' => 'delete_section', 'gender' => 'F', 'section' => 'j'));
  t_ok($r['ok'] && $r['msg'] === 'Section J deleted (2 rooms, 3 places).' && $sides('J') === NULL, 'D2 delete the shared section J from the women\'s tab (' . $r['msg'] . ')');
  t_eq((int) db_query("select count(*) from dh_center_room_place p join dh_center_room r on r.cr_id = p.crp_room where r.cr_center = :c and r.cr_section = 'J' and p.crp_deleted = 0", array(':c' => $c2))->fetchField(), 0, 'D3 its places are gone too');
  $j = $logs('section');
  $last = end($j);
  t_ok($last['cl_action'] === 'delete' && $last['cl_old'] === 'Shared J (2 rooms, 3 places): 1: 1A, 1B; 2: 2' && $last['cl_new'] === NULL, 'D4 Change Log: the deleted section with its rooms (' . $last['cl_old'] . ')');
  t_eq($act(array('op' => 'delete_section', 'gender' => 'M', 'section' => 'J'))['msg'], 'There is no section J.', 'D5 a section that is not there');
  t_ok($act(array('op' => 'delete_section', 'gender' => 'F', 'section' => 'G'))['ok'] && $sides('G') === 'M', 'D6 deleting women\'s G leaves men\'s G');

  // Change Log: "Mark as checked".
  $rv = t_make_room($c2, 'M', 'K', '7', array('7'));
  $plain = t_make_room($c2, 'M', 'K', '8', array('8'));
  db_update('dh_center_room')->fields(array('cr_review' => 1))->condition('cr_id', $rv)->execute();
  t_ok($act(array('op' => 'clear_review', 'ids' => array($rv, $plain)))['ok'], 'L1 mark two rooms as checked');
  t_eq($logs('checked'), array(array('cl_action' => 'update', 'cl_old' => 'Needs a look: 7', 'cl_new' => 'Checked')), 'L2 Change Log: only the room that needed a look');
  db_update('dh_center_room')->fields(array('cr_review' => 1))->condition('cr_id', $plain)->execute();
  t_ok($act(array('op' => 'set_toilet', 'ids' => array($plain), 'toilet' => 'unset'))['ok'], 'L3 set toilet (unchanged) on a room that needed a look');
  $c = $logs('checked');
  t_eq(array(count($c), end($c)['cl_old']), array(2, 'Needs a look: 8'), 'L4 setting the toilet also logs "Checked"');
});
$GLOBALS['user'] = $keep;
t_done();
