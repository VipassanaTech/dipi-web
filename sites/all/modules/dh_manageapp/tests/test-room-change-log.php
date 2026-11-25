<?php
// Room log lines from every page that changes a student's room (owner, 25 Nov 2026): Room Board, Zero Day popup and
// Edit Application all write "Room: <old|none> → <new|none>" (dh_room_log_change), only when the room really changed,
// so the change log at the bottom of the Room Board is complete. Made-up data, rolled back.
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-room-change-log.php
require __DIR__ . '/lib.php';

echo "Room text\n";
t_eq(dh_room_place_text('M', '5A'), 'M-5A', 'section-place');
t_eq(dh_room_place_text(' M ', ' 5A '), 'M-5A', 'trimmed');
t_eq(dh_room_place_text('', '12'), '12', 'no section: the place alone');
t_eq(dh_room_place_text('M', ''), '', 'no place: no room');

echo "Log line\n";
t_in_txn(function () {
	$centre = t_make_centre();
	$course = t_make_course(array('c_center' => $centre, 'c_combined_seat_course' => 0));
	$a = t_make_applicant($course, array('a_gender' => 'M'));
	$lines = function () use ($a) { return db_query("select l_msg from dh_log where l_module = 'Applicant' and l_identifier = :a and l_event = 'Room' order by l_id", array(':a' => $a))->fetchCol(); };
	t_ok(dh_room_log_change($centre, $a, '', '', 'M', '5A', 1), 'placed fresh: written');
	t_ok(dh_room_log_change($centre, $a, 'M', '5A', 'M', '6A', 1), 'moved: written');
	t_ok(!dh_room_log_change($centre, $a, 'M', '6A', ' M', '6A ', 1), 'same room (spaces only): nothing written');
	t_ok(!dh_room_log_change($centre, $a, '', '', '', '', 1), 'no room before or after: nothing written');
	t_ok(dh_room_log_change($centre, $a, 'M', '6A', '', '', 1), 'taken out: written');
	t_eq($lines(), array('Room: none → M-5A', 'Room: M-5A → M-6A', 'Room: M-6A → none'), 'the three lines, in the board\'s format');
	$log = dh_room_board_log($course, array('M'));
	t_eq(array(count($log), $log[0]['old'], $log[0]['new'], $log[2]['old'], $log[2]['new']), array(3, 'M-6A', '', '', 'M-5A'), 'and they show in the Room Board change log');
});

echo "Every page that changes a room writes the line\n";
$module = file_get_contents(__DIR__ . '/../dh_manageapp.module');
preg_match('/function dh_app_update_attended\(\).*?\n}\n/s', $module, $m);
t_ok(!empty($m[0]) && preg_match('/\$room_before = db_query\(.*?db_insert\(.dh_applicant_attended.\).*?dh_room_log_change\(\$centre_id, \$app_id, \$room_before/s', $m[0]) === 1, 'Zero Day popup: room before, save, then the log line');
$app = file_get_contents(__DIR__ . '/../inc/application.inc');
t_ok(preg_match('/\$room_before = \$app_id \? db_query\(.*?if \(\$input\[.attending.\]\).*?dh_room_log_change\(\$centre_id, \$app_id, \$room_was\[0\], \$room_was\[1\], isset\(\$app_aa\[.aa_section.\]\)/s', $app) === 1, 'Edit Application: a chosen room is logged');
t_ok(preg_match('/delete from dh_applicant_attended where aa_applicant=".\$app_id\);\s*\n\s*dh_room_log_change\(\$centre_id, \$app_id, \$room_was\[0\], \$room_was\[1\], \'\', \'\'/s', $app) === 1, 'Edit Application: not attending any more = room gone, logged');
$board = file_get_contents(__DIR__ . '/../inc/room-board.inc');
t_ok(preg_match('/function _dh_room_board_set\(.*?dh_room_log_change\(/s', $board) === 1, 'Room Board: uses the same helper');
t_done();
