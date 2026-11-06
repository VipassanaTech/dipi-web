<?php
// Who is "one person" on an email, and which application fills the form (inc/autofill.inc).
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-autofill-match.php
require __DIR__ . '/lib.php';

t_eq(dh_autofill_norm('  Anand   K.  Kumar '), 'anand k kumar', 'names: lowercase, no dots, one space');
t_eq(dh_autofill_person_key(array('a_f_name' => 'Neha.', 'a_l_name' => ' BANSAL ', 'a_gender' => 'f')), 'neha|bansal|F', 'person key');
t_eq(dh_autofill_mask_name('Neha', 'Bansal'), 'N*** B*****', 'masked name');
t_eq(dh_autofill_mask_name('Anand Kumar', 'Sharma'), 'A**** K**** S*****', 'every word masked');
t_eq(dh_autofill_e164('91', '98765 43210'), '+919876543210', 'mobile as E.164');
t_eq(dh_autofill_e164('91', '09876543210'), '+919876543210', 'a leading 0 is dropped');
t_eq(dh_autofill_e164('', '9876543210'), '', 'no country code: nothing');
t_eq(dh_autofill_e164('91', '12345'), '', 'not a real number: nothing');
t_eq(dh_autofill_ymd(2020, 2, 29), '2020-02-29', 'real date');
t_eq(dh_autofill_ymd(2021, 2, 29), '', 'not a real date');
t_eq(dh_autofill_ymd(2020, 0, 0), '', 'year only');
t_eq(dh_autofill_ymd('', '', ''), '', 'empty parts');
t_eq(dh_autofill_date('1985-03-04'), '1985-03-04', 'stored date');
t_eq(dh_autofill_date('0000-00-00'), '', 'zero date');
t_eq(dh_autofill_date('2026-11-06 10:11:12'), '2026-11-06', 'stored timestamp');
t_eq(count(dh_autofill_forms()), 9, 'nine target forms');

function af_type($key)
{
	return (int) db_query("select td_id from dh_type_detail where td_type = 'COURSE-TYPE' and td_key = :k order by td_id limit 1", array(':k' => $key))->fetchField();
}
function af_app($course, $email, $f, $l, $g, $created, $over = array())
{
	$id = t_make_applicant($course, array_merge(array('a_email' => $email, 'a_f_name' => $f, 'a_l_name' => $l, 'a_gender' => $g), $over));
	db_update('dh_applicant')->fields(array('a_created' => $created))->condition('a_id', $id)->execute();
	return $id;
}
function af_result($email, $group)
{
	$m = dh_autofill_match(dh_autofill_applications($email), $group);
	return $m['result'];
}

$old_test_email = variable_get('autofill_test_email', NULL);
t_in_txn(function () {
	$ten = t_make_course(array('c_center' => 5, 'c_course_type' => af_type('10-Day')));
	$one = t_make_course(array('c_center' => 5, 'c_course_type' => af_type('1-DayOSC')));
	$kid = t_make_course(array('c_center' => 5, 'c_course_type' => af_type('Child-Teen')));
	$work = t_make_course(array('c_center' => 5, 'c_course_type' => af_type('ServicePeriod')));
	$dummy = t_make_course(array('c_center' => 302, 'c_course_type' => af_type('10-Day')));

	// One person spelled two ways: the latest full course is the source, never the newer one-day.
	$e = 'Zzq-AF-1@Example.Invalid';
	af_app($ten, $e, 'Neha', 'Bansal', 'F', '2024-01-01 10:00:00');
	$newest10 = af_app($ten, $e, 'neha.', 'BANSAL', 'F', '2025-05-01 10:00:00');
	af_app($one, $e, 'Neha', 'Bansal', 'F', '2026-01-01 10:00:00');
	$m = dh_autofill_match(dh_autofill_applications('zzq-af-1@example.invalid'), 'adult');
	t_eq($m['result'], 'single', 'one person, any case of the email');
	t_eq((int) $m['source']['a_id'], $newest10, 'source = latest full course, not the newer one-day');
	t_eq(af_result('zzq-af-1@example.invalid', 'child'), 'none', 'an adult cannot fill a child form');

	// Two people on one email.
	af_app($ten, 'zzq-af-2@example.invalid', 'Neha', 'Bansal', 'F', '2025-01-01 10:00:00');
	af_app($ten, 'zzq-af-2@example.invalid', 'Ravi', 'Bansal', 'M', '2025-02-01 10:00:00');
	t_eq(af_result('zzq-af-2@example.invalid', 'adult'), 'multiple', 'two people');

	// Duplicate and Errors are not counted.
	af_app($ten, 'zzq-af-3@example.invalid', 'Neha', 'Bansal', 'F', '2025-01-01 10:00:00');
	af_app($ten, 'zzq-af-3@example.invalid', 'Ravi', 'Kumar', 'M', '2025-02-01 10:00:00', array('a_status' => 'Duplicate'));
	af_app($ten, 'zzq-af-3@example.invalid', 'Sita', 'Rao', 'F', '2025-03-01 10:00:00', array('a_status' => 'Errors'));
	t_eq(af_result('zzq-af-3@example.invalid', 'adult'), 'single', 'Duplicate and Errors left out');

	// Test centre 302: left out, except for the test email.
	af_app($dummy, 'zzq-af-4@example.invalid', 'Test', 'Person', 'M', '2025-01-01 10:00:00');
	variable_del('autofill_test_email');
	t_eq(af_result('zzq-af-4@example.invalid', 'adult'), 'none', 'test centre left out');
	variable_set('autofill_test_email', 'ZZQ-AF-4@example.invalid');
	t_eq(af_result('zzq-af-4@example.invalid', 'adult'), 'single', 'test centre counted for the test email');

	// Only one-day and work-period applications: a person, but nothing to fill from.
	af_app($one, 'zzq-af-5@example.invalid', 'Neha', 'Bansal', 'F', '2025-01-01 10:00:00');
	af_app($work, 'zzq-af-5@example.invalid', 'Neha', 'Bansal', 'F', '2025-02-01 10:00:00');
	t_eq(af_result('zzq-af-5@example.invalid', 'adult'), 'none', 'no full course: none');

	// A parent's email with one child: a child form finds only the child; adult forms see two people.
	af_app($ten, 'zzq-af-6@example.invalid', 'Neha', 'Bansal', 'F', '2025-01-01 10:00:00');
	$child = af_app($kid, 'zzq-af-6@example.invalid', 'Aarav', 'Bansal', 'M', '2025-06-01 10:00:00');
	$m = dh_autofill_match(dh_autofill_applications('zzq-af-6@example.invalid'), 'child');
	t_eq(array($m['result'], (int) $m['source']['a_id']), array('single', $child), 'child form: only the child counts');
	t_eq(af_result('zzq-af-6@example.invalid', 'adult'), 'multiple', 'adult form: parent and child are two people');
	af_app($kid, 'zzq-af-6@example.invalid', 'Diya', 'Bansal', 'F', '2025-07-01 10:00:00');
	t_eq(af_result('zzq-af-6@example.invalid', 'child'), 'multiple', 'two children: several');

	// The same a_created: the higher a_id wins.
	$x = af_app($ten, 'zzq-af-7@example.invalid', 'Neha', 'Bansal', 'F', '2025-01-01 10:00:00');
	$y = af_app($ten, 'zzq-af-7@example.invalid', 'Neha', 'Bansal', 'F', '2025-01-01 10:00:00');
	$m = dh_autofill_match(dh_autofill_applications('zzq-af-7@example.invalid'), 'adult');
	t_eq((int) $m['source']['a_id'], max($x, $y), 'same time: the later row');

	t_eq(af_result('zzq-af-none@example.invalid', 'adult'), 'none', 'unknown email: none');
	t_eq(dh_autofill_applications(''), array(), 'empty email: nothing');

	// The collation ignores accents; only the exact spelling is found.
	$acc = af_app($ten, 'zzq-andr'."\xC3\xA9".'@example.invalid', 'Neha', 'Bansal', 'F', '2025-01-01 10:00:00');
	t_eq(dh_autofill_applications('zzq-andre@example.invalid'), array(), 'accented email: the unaccented spelling is not found');
	$found = dh_autofill_applications('zzq-andr'."\xC3\xA9".'@example.invalid');
	t_eq(array(count($found), (int) $found[0]['a_id'], array_key_exists('a_email', $found[0])), array(1, $acc, FALSE), 'accented email: the exact spelling is found (no a_email in the row)');
});
if ($old_test_email === NULL)
	variable_del('autofill_test_email');
else
	variable_set('autofill_test_email', $old_test_email);
t_done();
