<?php
// "Course Left" on the application form (owner, 25 Nov 2026): staff choose the COURSE DAY (Day 0 = start date,
// Day N = start + N days, shown with its date) and the time from dropdowns, and a Left comment is required. DIPI
// works out the date from the course start and saves it as before (aa_left_on). Missing day / time / comment = a clear
// message, never the old crash (an empty date made the save fail: 33 times since 1 Oct).
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-left-on.php
require __DIR__ . '/lib.php';

echo "Course days\n";
$o = dh_left_day_options('2026-12-01', '2026-12-12', '2030-01-01');
t_eq(array_keys($o), range(0, 11), '10-day course 1st..12th: Day 0..Day 11');
t_eq($o[0], 'Day 0 · Tue 1 Dec', 'Day 0 = the start date (1st)');
t_eq($o[1], 'Day 1 · Wed 2 Dec', 'Day 1 = the 2nd');
t_eq($o[10], 'Day 10 · Fri 11 Dec', 'Day 10 = the 11th');
t_eq($o[11], 'Day 11 · Sat 12 Dec', 'Day 11 = the end date');
t_eq(array_keys(dh_left_day_options('2026-12-01', '2026-12-12', '2026-12-04')), range(0, 3), 'running course: no day after today');
t_eq(dh_left_day_options('2026-12-06', '2026-12-06', '2030-01-01'), array(0 => 'Course day · Sun 6 Dec'), 'one-day course: one choice');
t_eq(dh_left_day_options('2026-12-01', '2026-12-12', '2026-11-20'), array(), 'course not started yet: no day');
t_eq(dh_left_day_options('', '', '2030-01-01'), array(), 'no course dates: no day');

echo "Time\n";
$h = dh_left_hour_options();
t_eq(array($h[0], $h[5], $h[12], $h[13], $h[23]), array('12 AM', '5 AM', '12 PM', '1 PM', '11 PM'), 'hours in 12-hour form');
t_eq(array_keys(dh_left_minute_options()), array(0, 15, 30, 45), 'minutes in 15-minute steps');
t_eq(array_keys(dh_left_minute_options(7)), array(0, 7, 15, 30, 45), 'a saved odd minute is kept as a choice');

echo "Date from the course day\n";
t_eq(dh_left_on_value('2026-12-01', '3', '14', '30'), '2026-12-04 14:30:00', 'Day 3, 2:30 PM → 4 Dec 14:30');
t_eq(dh_left_on_value('2026-12-01', '0', '0', '0'), '2026-12-01 00:00:00', 'Day 0, 12 AM');
t_eq(dh_left_on_value('2026-12-01', 'saved', '9', '15', '2025-11-30 18:00:00'), '2025-11-30 09:15:00', 'the saved outside date + the chosen time');
foreach (array(array('', '14', '30'), array('3', '', '30'), array('3', '14', ''), array('x', '1', '0'), array('3', '24', '0'), array('3', '1', '60'), array("3' or 1", '1', '0')) as $bad)
	t_eq(dh_left_on_value('2026-12-01', $bad[0], $bad[1], $bad[2]), '', 'not usable (' . implode('/', $bad) . '): no date');

echo "Saved value back into the form\n";
t_eq(dh_left_on_parts('2026-12-01', '2026-12-12', '2026-12-04 14:30:00'), array('day' => 3, 'hour' => 14, 'minute' => 30, 'saved' => ''), 'inside the course: its day and time');
$p = dh_left_on_parts('2026-12-01', '2026-12-12', '2025-11-30 18:07:00');
t_eq(array($p['day'], $p['hour'], $p['minute']), array('saved', 18, 7), 'outside the course: kept as "saved"');
t_eq($p['saved'], 'Saved: Sun 30 Nov 2025 (outside course dates)', 'with a clear label');
t_eq(dh_left_on_parts('2026-12-01', '2026-12-12', ''), array('day' => '', 'hour' => '', 'minute' => '', 'saved' => ''), 'nothing saved: empty');

echo "The form\n";
t_in_txn(function () {
	$centre = t_make_centre();
	$course = t_make_course(array('c_center' => $centre, 'c_combined_seat_course' => 0, 'c_start' => date('Y-m-d', strtotime('-5 days')), 'c_end' => date('Y-m-d', strtotime('+6 days'))));
	$app = t_make_applicant($course, array('a_gender' => 'M'), array('aa_left' => 1, 'aa_left_on' => date('Y-m-d', strtotime('-3 days')) . ' 16:45:00', 'aa_left_comment' => 'went home'));
	$row = db_query("select * from dh_applicant left join dh_applicant_attended on aa_applicant = a_id where a_id = :a", array(':a' => $app))->fetchAssoc();
	$_GET['q'] = "app/$app/edit";
	drupal_static_reset('arg');
	$fs = array('input' => array());
	$form = dh_ma_applicant_form(array(), $fs, $row);
	$g = $form['Course Left'];
	t_ok(!isset($g['aa_left_on']), 'no calendar any more');
	t_eq(count($g['left_day']['#options']), 1 + 6, 'day choices: Day 0..today (5 days in) + "Choose"');
	t_eq(array($g['left_day']['#default_value'], $g['left_hour']['#default_value'], $g['left_minute']['#default_value']), array(2, 16, 45), 'saved Left shown as Day 2, 4 PM, 45');
	t_ok(isset($g['aa_left_comment']['#states']['required']), 'comment marked required when Course Left = Yes');

	echo "Checks before saving\n";
	$check = function ($in) use ($form) {
		drupal_static_reset('form_set_error');
		form_clear_error();
		$fs = array('input' => $in + array('a_gender' => 'M', 'ac_teacher_code' => ''), 'values' => array());
		dh_left_validate($fs);
		$e = form_get_errors();
		form_clear_error();
		return $e ? array_keys($e) : array();
	};
	t_eq($check(array('aa_left' => '1', 'left_day' => '2', 'left_hour' => '16', 'left_minute' => '45', 'aa_left_comment' => 'went home')), array(), 'all filled: fine');
	t_eq($check(array('aa_left' => '1', 'left_day' => '', 'left_hour' => '16', 'left_minute' => '45', 'aa_left_comment' => 'x')), array('left_day'), 'no day: message');
	t_eq($check(array('aa_left' => '1', 'left_day' => '2', 'left_hour' => '', 'left_minute' => '45', 'aa_left_comment' => 'x')), array('left_hour'), 'no hour: message');
	t_eq($check(array('aa_left' => '1', 'left_day' => '2', 'left_hour' => '16', 'left_minute' => '', 'aa_left_comment' => 'x')), array('left_hour'), 'no minutes: message');
	t_eq($check(array('aa_left' => '1', 'left_day' => '2', 'left_hour' => '16', 'left_minute' => '45', 'aa_left_comment' => '   ')), array('aa_left_comment'), 'no comment: message');
	t_eq($check(array('aa_left' => '0')), array(), 'not Left: nothing asked');
	t_eq($check(array()), array(), 'Left fields switched off (not attending): nothing asked');

	echo "Saving\n";
	t_eq(dh_left_on_from_input(array('left_day' => '2', 'left_hour' => '7', 'left_minute' => '15'), $course, $app), date('Y-m-d', strtotime('-3 days')) . ' 07:15:00', 'Day 2 of this course at 7:15 AM');
	db_update('dh_applicant_attended')->fields(array('aa_left_on' => '2025-11-30 18:00:00'))->condition('aa_applicant', $app)->execute();
	t_eq(dh_left_on_from_input(array('left_day' => 'saved', 'left_hour' => '18', 'left_minute' => '0'), $course, $app), '2025-11-30 18:00:00', '"saved" keeps the saved outside date');
	t_eq(dh_left_on_from_input(array('left_day' => '', 'left_hour' => '7', 'left_minute' => '15'), $course, $app), '', 'not usable: empty (never the broken " :00")');
});

$src = file_get_contents(__DIR__ . '/../inc/application.inc');
t_ok(strpos($src, "['time'].':00'") === FALSE, 'the old date + time gluing is gone');
$js = file_get_contents(__DIR__ . '/../js/manageapp.js');
t_ok(strpos($js, 'select[name=left_day]') !== FALSE && strpos($js, 'aa_left_comment') !== FALSE, 'the page also checks day, time and comment before sending');
t_done();
