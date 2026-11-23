<?php
// Manage Courses > Delete first asks app-student-count-cancel/<course ids> "does this course have applications?"
// (Yes = "Course has applications. Cannot be deleted"). Only plain course numbers reach the database, and only
// people who may manage courses can ask.
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-course-delete-check.php
require __DIR__ . '/lib.php';

echo "Who can ask\n";
$items = dh_manageapp_menu();
$route = isset($items['app-student-count-cancel/%course_arr']) ? $items['app-student-count-cancel/%course_arr'] : array();
t_eq(isset($route['access arguments']) ? $route['access arguments'] : NULL, array('manage course'), 'same permission as the Manage Courses page');
t_eq($items['manage-course/%centre_id']['access arguments'], array('manage course'), '(Manage Courses page: manage course)');

echo "Answers\n";
t_in_txn(function () {
	$full = t_make_course(array('c_combined_seat_course' => 0));
	t_make_applicant($full);
	$empty = t_make_course(array('c_combined_seat_course' => 0));
	t_eq(dh_course_ids_have_applications("$full"), 'Yes', 'course with an application: Yes (cannot delete)');
	t_eq(dh_course_ids_have_applications("$empty"), 'No', 'course without applications: No (can delete)');
	t_eq(dh_course_ids_have_applications("$empty,$full"), 'Yes', 'several courses, one with applications: Yes');
	t_eq(dh_course_ids_have_applications("$empty,$empty"), 'No', 'several courses, none with applications: No');
	$ok = TRUE;
	try {
		t_eq(dh_course_ids_have_applications("$full' or '1'='1"), 'No', 'database text in the address: No');
		t_eq(dh_course_ids_have_applications("0 union select 1"), 'No', 'database text (union): No');
		t_eq(dh_course_ids_have_applications("$empty,1' or sleep(5) -- "), 'No', 'a bad part is ignored, the good part still answered');
		t_eq(dh_course_ids_have_applications(''), 'No', 'nothing: No');
		t_eq(dh_course_ids_have_applications(array($full)), 'No', 'not text: No');
	}
	catch (Throwable $e) {
		$ok = FALSE;
	}
	t_ok($ok, 'no database error from any of these');
});
t_done();
