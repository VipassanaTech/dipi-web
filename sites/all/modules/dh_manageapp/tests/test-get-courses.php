<?php
// Transfer popup course list (get-courses/<centre number>): the centre number reaches the database only as a
// number. Same list for real centres; anything else gives an empty list.
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-get-courses.php
require __DIR__ . '/lib.php';

t_in_txn(function () {
	$centre = (int) db_query("select c_center from dh_course where c_center > 0 and c_deleted = 0 order by c_id desc limit 1")->fetchField();
	$up = t_make_course(array('c_center' => $centre, 'c_combined_seat_course' => 0, 'c_start' => date('Y-m-d', strtotime('+20 days')), 'c_end' => date('Y-m-d', strtotime('+30 days')), 'c_name' => 'ZZQ Upcoming'));
	$past = t_make_course(array('c_center' => $centre, 'c_combined_seat_course' => 0, 'c_name' => 'ZZQ Past'));
	$want = db_query("select c_name as 'name', c_id as 'id' from dh_course where c_center = :c and c_deleted = 0 and c_start >= curdate() order by c_start", array(':c' => $centre))->fetchAll(PDO::FETCH_ASSOC);
	$got = dh_centre_upcoming_courses((string) $centre);
	t_eq($got, $want, 'real centre: its upcoming courses, same order and shape (name, id)');
	$ids = array_column($got, 'id');
	t_ok(in_array((string) $up, array_map('strval', $ids)), 'an upcoming course is listed');
	t_ok(!in_array((string) $past, array_map('strval', $ids)), 'a past course is not listed');
	$ok = TRUE;
	try {
		t_eq(dh_centre_upcoming_courses("$centre or 1=1"), array(), 'database text after the number: empty list');
		t_eq(dh_centre_upcoming_courses("0 union select name, pass from users"), array(), 'database text (union): empty list');
		t_eq(dh_centre_upcoming_courses(''), array(), 'nothing: empty list');
		t_eq(dh_centre_upcoming_courses(array($centre)), array(), 'not text: empty list');
	}
	catch (Throwable $e) {
		$ok = FALSE;
	}
	t_ok($ok, 'no database error from any of these');
});
$src = file_get_contents(__DIR__ . '/../dh_manageapp.module');
preg_match('/function _get_courses\(.*?\n}\n/s', $src, $m);
t_ok(!empty($m[0]) && strpos($m[0], 'c_center=$') === FALSE, 'the centre number is no longer pasted into the query');
t_done();
