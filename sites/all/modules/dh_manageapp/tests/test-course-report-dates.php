<?php
// Day / Month / Year date field (dh_dmy_element, inc/dmy.inc) - DIPI's usual date input, used first on the Centre
// Course Report - and the report's query: the two dates reach the database only as real dates.
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-course-report-dates.php
require __DIR__ . '/lib.php';

echo "The field\n";
$el = dh_dmy_element(array('title' => 'From', 'default' => '2025-11-24', 'required' => TRUE, 'years' => array(2010, 2026)));
t_eq(array($el['day']['#type'], $el['month']['#type'], $el['year']['#type']), array('select', 'select', 'select'), 'three dropdowns: day, month, year');
t_eq(array($el['day']['#default_value'], $el['month']['#default_value'], $el['year']['#default_value']), array(24, 11, 2025), 'default date shown');
t_eq($el['month']['#options'][11], 'Nov', 'months by name (no day/month mix-up)');
t_ok(isset($el['year']['#options'][2010]) && isset($el['year']['#options'][2026]) && !isset($el['year']['#options'][2027]), 'year list from the given range');
t_eq($el['day']['#options'][''], 'Day', 'empty choice labelled Day');
$old = dh_dmy_element(array('title' => 'X', 'default' => '2001-03-05', 'years' => array(2010, 2026)));
t_ok(isset($old['year']['#options'][2001]), 'a default outside the range is still offered');

echo "Checking what was chosen\n";
$check = function ($vals, $required = TRUE) {
	drupal_static_reset('form_set_error');
	form_clear_error();
	$el = dh_dmy_element(array('title' => 'From', 'required' => $required, 'years' => array(2010, 2026)));
	$el['#parents'] = array('report_from_date');
	$el['#array_parents'] = array('report_from_date');
	$fs = array('values' => array('report_from_date' => $vals));
	dh_dmy_element_validate($el, $fs);
	$errors = form_get_errors();
	form_clear_error();
	return array($fs['values']['report_from_date'], $errors ? 'error' : 'ok');
};
t_eq($check(array('day' => '24', 'month' => '11', 'year' => '2025')), array('2025-11-24', 'ok'), 'full date → 2025-11-24');
t_eq($check(array('day' => '5', 'month' => '1', 'year' => '2026')), array('2026-01-05', 'ok'), 'single digits padded');
$r = $check(array('day' => '31', 'month' => '2', 'year' => '2026'));
t_eq($r[1], 'error', '31 February: error');
$r = $check(array('day' => '29', 'month' => '2', 'year' => '2024'));
t_eq($r, array('2024-02-29', 'ok'), '29 February in a leap year: fine');
$r = $check(array('day' => '10', 'month' => '', 'year' => '2026'));
t_eq($r[1], 'error', 'month missing: error');
$r = $check(array('day' => '', 'month' => '', 'year' => ''));
t_eq($r[1], 'error', 'required and empty: error');
t_eq($check(array('day' => '', 'month' => '', 'year' => ''), FALSE), array('', 'ok'), 'not required and empty: empty value');
$r = $check(array('day' => "1' or '1'='1", 'month' => '1', 'year' => '2026'));
t_eq($r[1], 'error', 'not a number: error');

echo "Course report query\n";
t_in_txn(function () {
	$centre = t_make_centre();
	$c1 = t_make_course(array('c_center' => $centre, 'c_combined_seat_course' => 0, 'c_start' => '2026-03-01', 'c_end' => '2026-03-11'));
	$c2 = t_make_course(array('c_center' => $centre, 'c_combined_seat_course' => 0, 'c_start' => '2026-06-01', 'c_end' => '2026-06-11'));
	t_make_applicant($c1, array('a_gender' => 'M', 'a_old' => 0, 'a_type' => 'Student', 'a_attended' => 1));
	t_make_applicant($c1, array('a_gender' => 'F', 'a_old' => 1, 'a_type' => 'Student', 'a_attended' => 1));
	t_make_applicant($c2, array('a_gender' => 'F', 'a_old' => 1, 'a_type' => 'Sevak', 'a_attended' => 1));
	$rows = dh_course_report_rows($centre, '2026-01-01', '2026-12-31')->fetchAll(PDO::FETCH_ASSOC);
	t_eq(count($rows), 2, 'both courses in the range');
	t_eq(array($rows[0]['NewMale'], $rows[0]['OldFemale'], $rows[0]['StudentTotal']), array('1', '1', '2'), 'counts of the first course');
	t_eq($rows[1]['SevakFemale'], '1', 'server counted');
	t_eq(count(dh_course_report_rows($centre, '2026-04-01', '2026-12-31')->fetchAll()), 1, 'range limits the courses');
	t_eq(count(dh_course_report_rows($centre, "2099-01-01' or '1'='1", '2099-12-31')->fetchAll()), 0, 'database text after a date is only data (the OR does nothing)');
});
$src = file_get_contents(__DIR__ . '/../inc/reports.inc');
t_ok(strpos($src, "'\$from_date'") === FALSE && strpos($src, "['input']['report_from_date']") === FALSE, 'the report no longer pastes typed dates into the query');
t_eq(substr_count($src, "'date_popup'"), 0, 'the course report has no calendar popup any more');
t_done();
