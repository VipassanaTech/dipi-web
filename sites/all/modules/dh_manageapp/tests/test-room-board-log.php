<?php
// Room Board change log (owner, 25 Nov 2026): the bottom of the board lists every room change of the course, newest
// first: time, confirmation number, name, old room (empty when placed fresh), new room (empty when taken out), user.
// It reads the dh_log 'Room' lines the board already writes ("Room: A-1 → A-2"). Made-up data, rolled back.
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-room-board-log.php
require __DIR__ . '/lib.php';

t_in_txn(function () {
	$centre = t_make_centre();
	$course = t_make_course(array('c_center' => $centre, 'c_combined_seat_course' => 0));
	$other = t_make_course(array('c_center' => $centre, 'c_combined_seat_course' => 0));
	$m = t_make_applicant($course, array('a_gender' => 'M', 'a_conf_no' => 'NM901', 'a_f_name' => 'Zzqlog', 'a_l_name' => 'Male'));
	$f = t_make_applicant($course, array('a_gender' => 'F', 'a_conf_no' => 'NF902', 'a_f_name' => 'Zzqlog', 'a_l_name' => 'Female'));
	$x = t_make_applicant($other, array('a_gender' => 'M', 'a_conf_no' => 'NM903'));
	$person = function ($a) { return db_query("select a_id, aa_section, aa_acco from dh_applicant join dh_applicant_attended on aa_applicant = a_id where a_id = :a", array(':a' => $a))->fetchAssoc(); };
	$admin = db_query("select name from users where uid = 1")->fetchField();

	_dh_room_board_set($centre, $person($m), 'A', '1', 1);     // placed fresh
	_dh_room_board_set($centre, $person($m), 'A', '2', 1);     // moved
	_dh_room_board_set($centre, $person($f), 'F', '7', 1);     // other side
	_dh_room_board_set($centre, $person($m), '', '', 1);       // taken out
	_dh_room_board_set($centre, $person($x), 'A', '9', 1);     // another course

	echo "Male side\n";
	$log = dh_room_board_log($course, array('M'));
	t_eq(count($log), 3, 'three changes of this course on the male side');
	t_eq(array($log[0]['old'], $log[0]['new']), array('A-2', ''), 'newest first: taken out of A-2 (new room empty)');
	t_eq(array($log[1]['old'], $log[1]['new']), array('A-1', 'A-2'), 'moved A-1 → A-2');
	t_eq(array($log[2]['old'], $log[2]['new']), array('', 'A-1'), 'placed fresh in A-1 (old room empty)');
	t_eq(array($log[0]['conf'], $log[0]['name'], $log[0]['user'], $log[0]['gender']), array('NM901', 'Zzqlog Male', $admin, 'M'), 'confirmation number, name, user, side');
	t_ok(preg_match('/^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}$/', $log[0]['t']) === 1, 'time as Y-m-d H:i:s');
	t_ok(!in_array('NM903', array_column($log, 'conf'), TRUE), 'another course: not listed');

	echo "Sides\n";
	t_eq(array_column(dh_room_board_log($course, array('F')), 'conf'), array('NF902'), 'female side: only the female change');
	t_eq(count(dh_room_board_log($course, array('M', 'F'))), 4, 'both sides: all four');
	t_eq(dh_room_board_log($course, array()), array(), 'no side: nothing');
	t_eq(count(dh_room_board_log($course, array('M'), 2)), 2, 'a limit keeps the newest');

	echo "In the board data\n";
	$saved = $GLOBALS['user'];
	$GLOBALS['user'] = user_load(1, TRUE);
	drupal_static_reset('user_access');
	$d = dh_room_board_data($centre, $course);
	t_ok(isset($d['log']) && count($d['log']) === 4, 'the board data carries the log');
	$GLOBALS['user'] = $saved;
	drupal_static_reset('user_access');
});
t_done();
