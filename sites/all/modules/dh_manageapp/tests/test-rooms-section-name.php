<?php
// Section full name: set_section_name op, name kept by the other room ops, shown in both data endpoints,
// in the Zero Day popup labels and in the application form's Section dropdown (value stays the code).
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-rooms-section-name.php
require __DIR__ . '/lib.php';

t_ok(in_array('cr_section_name', db_query("desc dh_center_room")->fetchCol(), TRUE), 'N0 column cr_section_name exists');

t_in_txn(function () {
  $cen = t_make_centre();
  $act = function ($in) use ($cen) { return dh_rooms_action($cen, $in, 0); };
  $nm = function ($sec, $g = 'M') use ($cen) { return db_query("select group_concat(distinct cr_section_name separator '|') from dh_center_room where cr_center = :c and cr_gender = :g and cr_section = :s and cr_deleted = 0", array(':c' => $cen, ':g' => $g, ':s' => $sec))->fetchField(); };
  $act(array('op' => 'save_room', 'id' => 0, 'gender' => 'M', 'section' => 'M', 'name' => '1', 'toilet' => 'western', 'places' => array('1')));
  $act(array('op' => 'save_room', 'id' => 0, 'gender' => 'M', 'section' => 'M', 'name' => '2', 'toilet' => 'western', 'places' => array('2A', '2B')));
  t_eq($nm('M'), '', 'N1 a new section has no name');

  $r = $act(array('op' => 'set_section_name', 'gender' => 'M', 'section' => 'M', 'name' => '  Male Common Block  '));
  t_ok($r['ok'], 'N2 set a name');
  t_eq($nm('M'), 'Male Common Block', 'N3 trimmed, same on every room of the section');
  $log = db_query("select cl_field, cl_old, cl_new, cl_action from dh_center_log where cl_center = :c and cl_area = 'rooms' and cl_field = 'section_name'", array(':c' => $cen))->fetchAll(PDO::FETCH_ASSOC);
  t_eq($log, array(array('cl_field' => 'section_name', 'cl_old' => '', 'cl_new' => 'Male Common Block', 'cl_action' => 'update')), 'N4 one Change Log row, old -> new');
  t_eq($act(array('op' => 'set_section_name', 'gender' => 'M', 'section' => 'M', 'name' => 'Male Common Block'))['ok'], TRUE, 'N4b the same name again is fine');
  t_eq((int) db_query("select count(*) from dh_center_log where cl_center = :c and cl_field = 'section_name'", array(':c' => $cen))->fetchField(), 1, 'N4c ... and writes no second log row');

  t_eq($act(array('op' => 'set_section_name', 'gender' => 'M', 'section' => 'M', 'name' => str_repeat('x', 61)))['ok'], FALSE, 'N5 61 letters refused');
  t_eq($nm('M'), 'Male Common Block', 'N5b name unchanged after a refusal');
  t_eq($act(array('op' => 'set_section_name', 'gender' => 'M', 'section' => 'M', 'name' => str_repeat('é', 60)))['ok'], TRUE, 'N6 60 letters (multi-byte) accepted');
  t_eq($act(array('op' => 'set_section_name', 'gender' => 'M', 'section' => 'Nope', 'name' => 'x'))['ok'], FALSE, 'N7 an unknown section is refused');
  t_eq($act(array('op' => 'set_section_name', 'gender' => 'B', 'section' => 'M', 'name' => 'x'))['ok'], FALSE, 'N7b side must be Men or Women');
  $act(array('op' => 'set_section_name', 'gender' => 'M', 'section' => 'M', 'name' => 'Male Common Block'));

  // Rooms added later copy the name.
  $act(array('op' => 'save_room', 'id' => 0, 'gender' => 'M', 'section' => 'M', 'name' => '3', 'toilet' => 'western', 'places' => array('3')));
  $act(array('op' => 'add_many', 'gender' => 'M', 'section' => 'M', 'from' => 10, 'to' => 11, 'toilet' => 'both'));
  t_eq($nm('M'), 'Male Common Block', 'N8 save_room and add_many copy the section name');
  $other = $act(array('op' => 'add_many', 'gender' => 'M', 'section' => 'P', 'from' => 1, 'to' => 1, 'toilet' => 'both'));
  t_eq($nm('P'), '', 'N8b another section is not touched');
  // A room moved to another section takes that section's name.
  $three = db_query("select cr_id from dh_center_room where cr_center = :c and cr_name = '3'", array(':c' => $cen))->fetchField();
  $act(array('op' => 'save_room', 'id' => $three, 'gender' => 'M', 'section' => 'P', 'name' => '3', 'toilet' => 'western', 'places' => array('3')));
  t_eq($nm('P'), '', 'N8c a room moved into an unnamed section gets no name');
  $act(array('op' => 'save_room', 'id' => $three, 'gender' => 'M', 'section' => 'M', 'name' => '3', 'toilet' => 'western', 'places' => array('3')));
  t_eq($nm('M'), 'Male Common Block', 'N8d ... and one moved back takes the name again');
  // Split keeps it.
  $two = db_query("select cr_id from dh_center_room where cr_center = :c and cr_name = '2'", array(':c' => $cen))->fetchField();
  $act(array('op' => 'split', 'id' => $two));
  t_eq($nm('M'), 'Male Common Block', 'N8e split rooms keep the name');
  // Rename keeps it.
  $act(array('op' => 'rename_section', 'gender' => 'M', 'from' => 'M', 'to' => 'MB'));
  t_eq($nm('MB'), 'Male Common Block', 'N9 rename_section keeps the name');

  // Data endpoint.
  $room = dh_rooms_load($cen, 'M')[0];
  t_ok(array_key_exists('section_name', $room), 'N10 dh_rooms_load carries section_name');
  $names = array_unique(array_map(function ($r) { return $r['section'] . '=' . $r['section_name']; }, dh_rooms_load($cen, 'M')));
  sort($names);
  t_eq(array_values($names), array('MB=Male Common Block', 'P='), 'N10b names per section');

  // Labels.
  t_eq(dh_rooms_section_label('MB', 'Male Common Block'), 'MB — Male Common Block', 'N11 label = CODE — Name');
  t_eq(dh_rooms_section_label('P', ''), 'P', 'N11b no name: the code only');
  $lab = dh_rooms_section_labels($cen);
  t_eq($lab['M'], array('MB' => 'MB — Male Common Block', 'P' => 'P'), 'N12 labels per side');
  t_eq($lab['F'], array(), 'N12b women have none');
  // Both data endpoints carry section_name on each room (the Rooms page data is dh_rooms_load).
  $GLOBALS['user'] = user_load(1);
  $course = t_make_course(array('c_center' => $cen));
  $bd = dh_room_board_data($cen, $course);
  $bn = array_unique(array_map(function ($r) { return $r['section'] . '=' . $r['section_name']; }, $bd['rooms']));
  sort($bn);
  t_eq(array_values($bn), array('MB=Male Common Block', 'P='), 'N12c Room Board data: section_name on each room');
  // Application form: option text has the name, value is the code.
  $opts = _ajax_get_section_options('M', $cen);
  t_eq($opts, array('' => 'Choose', 'MB' => 'MB — Male Common Block', 'P' => 'P'), 'N13 application form Section options');

  // Shared section: one name for both sides; joining a twin keeps this name (or takes the twin's if empty).
  $act(array('op' => 'save_room', 'id' => 0, 'gender' => 'M', 'section' => 'S', 'name' => '1', 'toilet' => 'both', 'places' => array('1')));
  $act(array('op' => 'save_room', 'id' => 0, 'gender' => 'F', 'section' => 'S', 'name' => '1', 'toilet' => 'both', 'places' => array('1')));
  $act(array('op' => 'set_section_name', 'gender' => 'F', 'section' => 'S', 'name' => 'Women Block'));
  t_eq($nm('S', 'F'), 'Women Block', 'N14 women\'s S named');
  t_eq($nm('S', 'M'), '', 'N14b men\'s S has its own (empty) name');
  t_ok($act(array('op' => 'set_used_by', 'gender' => 'M', 'section' => 'S', 'to' => 'B'))['ok'], 'N15 join into a shared section');
  t_eq($nm('S', 'B'), 'Women Block', 'N15b an empty name takes the twin\'s');
  t_eq($act(array('op' => 'set_section_name', 'gender' => 'M', 'section' => 'S', 'name' => 'Shared Hall'))['ok'], TRUE, 'N16 naming a shared section from the men\'s tab');
  t_eq($nm('S', 'B'), 'Shared Hall', 'N16b every room of the shared section');
  $lab = dh_rooms_section_labels($cen);
  t_eq($lab['M']['S'] . '|' . $lab['F']['S'], 'S — Shared Hall|S — Shared Hall', 'N16c a shared section label is under both sides');
  $act(array('op' => 'save_room', 'id' => 0, 'gender' => 'F', 'section' => 'T', 'name' => '1', 'toilet' => 'both', 'places' => array('1')));
  $act(array('op' => 'save_room', 'id' => 0, 'gender' => 'M', 'section' => 'T', 'name' => '1', 'toilet' => 'both', 'places' => array('1')));
  $act(array('op' => 'set_section_name', 'gender' => 'M', 'section' => 'T', 'name' => 'Mine'));
  $act(array('op' => 'set_used_by', 'gender' => 'M', 'section' => 'T', 'to' => 'B'));
  t_eq($nm('T', 'B'), 'Mine', 'N17 joining keeps this section\'s name when it has one');
  // Delete section clears with its rooms.
  t_ok($act(array('op' => 'delete_section', 'gender' => 'M', 'section' => 'T'))['ok'], 'N18 delete section works');
});
t_done();
