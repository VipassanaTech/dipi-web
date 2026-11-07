<?php
// What fetch returns for an application (inc/autofill.inc): only the listed keys, never health,
// declarations, conduct answers or anything about the old application.
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-autofill-fields.php
require __DIR__ . '/lib.php';

function af_type($key)
{
	return (int) db_query("select td_id from dh_type_detail where td_type = 'COURSE-TYPE' and td_key = :k order by td_id limit 1", array(':k' => $key))->fetchField();
}
// A value for every column of a table: text 'zzq-<column>', numbers 1, dates 2001-02-03.
function af_values($table, $skip)
{
	$set = array();
	foreach (db_query("show columns from $table") as $c)
	{
		if (in_array($c->Field, $skip) || strpos($c->Extra, 'auto_increment') !== FALSE)
			continue;
		$t = strtolower($c->Type);
		if (preg_match('/^(tinyint|smallint|mediumint|int|bigint|decimal|float|double)/', $t))
			$set[$c->Field] = 1;
		elseif (strpos($t, 'datetime') === 0 || strpos($t, 'timestamp') === 0)
			$set[$c->Field] = '2001-02-03 04:05:06';
		elseif (strpos($t, 'date') === 0)
			$set[$c->Field] = '2001-02-03';
		elseif (preg_match('/^(var)?char\((\d+)\)/', $t, $m))
			$set[$c->Field] = substr('zzq-'.$c->Field, 0, (int) $m[2]);
		elseif (strpos($t, 'text') !== FALSE)
			$set[$c->Field] = 'zzq-'.$c->Field;
	}
	return $set;
}

$common = array('a_gender', 'a_old', 'a_f_name', 'a_m_name', 'a_l_name', 'a_address', 'a_zip', 'a_phone_home', 'a_phone_office',
	'a_emergency_name', 'a_emergency_num', 'a_emergency_relation', 'a_lang_1_level', 'a_lang_2_level', 'a_lang_discourse',
	'a_education', 'a_occupation', 'a_department', 'a_designation', 'a_dob', 'a_phone_mobile', 'photo_id_type', 'photo_id_val',
	'a_city', 'country', 'ae_nationality', 'ae_id_issued_by', 'ae_seva_details', 'ae_id_issued',
	'ac_teen', 'ac_10d', 'ac_stp', 'ac_spl', 'ac_20d', 'ac_30d', 'ac_45d', 'ac_60d', 'ac_tsc', 'ac_service', 'ac_practice_details',
	'ac_first_location_str', 'ac_first_teacher_str', 'ac_last_location_str', 'ac_last_teacher_str', 'date_first_course');
$child = array('ae_father', 'ae_mother', 'ae_parent_course', 'ae_ch_course', 'ae_ch_course_detail', 'ae_ch_practice', 'ae_ch_practice_detail');
$lc = array('al_recent_sat_type', 'al_last_lc_type', 'al_last_lc_date', 'al_last_lc_location', 'al_last_lc_teacher');
$never = array('a_email', 'a_extra', 'a_type', 'a_course', 'a_center', 'a_purpose', 'a_status', 'a_aadhar', 'a_passport',
	'ae_desc_physical', 'ae_desc_mental', 'ae_desc_medication', 'ae_teen_std', 'al_at_year', 'al_intoxicants', 'photo', 'tsc_duration', 'a_dob_s',
	'ac_first_location', 'ac_first_teacher', 'ac_last_location', 'ac_last_teacher');

t_in_txn(function () use ($common, $child, $lc, $never) {
	$c = t_make_course(array('c_center' => 5, 'c_course_type' => af_type('10-Day')));
	$city = db_query("select ci.c_id, ci.c_name, ci.c_country, ci.c_state, co.c_name country_name, s.s_name state_name
		from dh_city ci join dh_country co on co.c_code = ci.c_country join dh_state s on s.s_code = ci.c_state and s.s_country = ci.c_country
		where ci.c_name <> '' order by ci.c_id limit 1")->fetchObject();
	$a = t_make_applicant($c);
	$skip = array('a_id', 'a_course', 'a_center', 'a_created', 'a_updated', 'a_created_by', 'a_updated_by');
	db_update('dh_applicant')->fields(array_merge(af_values('dh_applicant', $skip), array(
		'a_gender' => 'F', 'a_old' => 1, 'a_f_name' => 'Zzqfirst', 'a_m_name' => 'Zzqmid', 'a_l_name' => 'Zzqlast',
		'a_dob' => '1985-03-04', 'a_email' => 'zzq-af-fields@example.invalid', 'a_mob_country' => 91, 'a_phone_mobile' => '9876543210',
		'a_aadhar' => '123412341234', 'a_passport' => 'Z1234567', 'a_city' => $city->c_id, 'a_country' => $city->c_country,
		'a_state' => $city->c_state, 'a_company' => '', 'a_status' => 'Confirmed',
		'a_phone_home' => '022 2345 6789', 'a_phone_office' => '+91 98765 43211', 'a_emergency_num' => '9876543212',
		'a_occupation' => ' Student ',
	)))->condition('a_id', $a)->execute();
	db_insert('dh_applicant_extra')->fields(array_merge(af_values('dh_applicant_extra', array()), array('ae_applicant' => $a)))->execute();
	db_insert('dh_applicant_course')->fields(array_merge(af_values('dh_applicant_course', array()), array('ac_applicant' => $a,
		'ac_first_year' => 2005, 'ac_first_month' => 6, 'ac_first_day' => 7, 'ac_last_year' => 2024, 'ac_last_month' => 3, 'ac_last_day' => 0, 'ac_teen' => 0)))->execute();
	db_insert('dh_applicant_lc')->fields(array_merge(af_values('dh_applicant_lc', array()), array('al_applicant' => $a)))->execute();

	$f = dh_autofill_fields($a, 'application_form');
	$keys = array_keys($f); sort($keys); $want = $common; sort($want);
	t_eq($keys, $want, 'adult form: exactly the listed keys (empty company and half date left out)');
	t_eq(array($f['a_old'], $f['a_dob'], $f['a_f_name'], $f['a_m_name']), array('1', '1985-03-04', 'Zzqfirst', 'Zzqmid'), 'names, DOB, old student');
	t_eq(array($f['photo_id_type'], $f['photo_id_val']), array('Aadhar', '123412341234'), 'ID: Aadhaar first, full number');
	t_eq($f['a_phone_mobile'], '+919876543210', 'mobile as E.164');
	t_eq(array($f['a_phone_home'], $f['a_phone_office'], $f['a_emergency_num']), array('+912223456789', '+919876543211', '+919876543212'),
		'other phones as E.164, using the mobile country code when they have none');
	t_eq($f['a_gender'], 'Female', 'gender as the form option (Female/Male)');
	t_eq($f['a_occupation'], 'Student', 'values trimmed');
	t_eq($f['a_city'], $city->c_name, 'city name, not its id');
	t_eq($f['country'], array('country_code' => $city->c_country, 'country_name' => $city->country_name,
		'state_code' => $city->c_state, 'state_name' => $city->state_name), 'country and state names');
	t_eq(array($f['date_first_course'], $f['ae_id_issued'], $f['ac_teen']), array('2005-06-07', '2001-02-03', '0'), 'dates and a zero count');
	foreach ($never as $k)
		t_ok(!array_key_exists($k, $f), "never: $k");
	$dump = json_encode($f);
	foreach (array('zzq-ae_desc', 'zzq-a_extra', 'zzq-al_at', 'zzq-a_purpose', 'zzq-ae_teen') as $m)
		t_ok(strpos($dump, $m) === FALSE, "no value from $m*");

	$f = dh_autofill_fields($a, 'long_course_application_form');
	$keys = array_keys($f); sort($keys); $want = array_merge($common, $lc); sort($want);
	t_eq($keys, $want, 'long course: plus only the five course-history al_* keys');

	$f = dh_autofill_fields($a, 'children_application_form');
	$keys = array_keys($f); sort($keys); $want = array_merge($common, $child); sort($want);
	t_eq($keys, $want, 'children form: plus the family and children-course fields');

	// VI forms offer only Passport and Voter ID: the best of those, and no issuer/date that belonged to another ID.
	foreach (array('vi_application_form', 'vi_one_day_application_form') as $vi)
	{
		$f = dh_autofill_fields($a, $vi);
		t_eq(array($f['photo_id_type'], $f['photo_id_val']), array('Passport', 'Z1234567'), "$vi: Passport, not Aadhaar");
		t_ok(!isset($f['ae_id_issued_by']) && !isset($f['ae_id_issued']), "$vi: no issuer or date of the Aadhaar");
	}
	db_update('dh_applicant')->fields(array('a_aadhar' => ''))->condition('a_id', $a)->execute();
	$f = dh_autofill_fields($a, 'vi_application_form');
	t_eq(array($f['photo_id_type'], $f['ae_id_issued_by'], $f['ae_id_issued']), array('Passport', 'zzq-ae_id_issued_by', '2001-02-03'),
		'VI form: Passport is also the usual first choice, so its issuer and date stay');

	db_update('dh_applicant')->fields(array('a_aadhar' => '', 'a_passport' => ' ', 'a_pancard' => 'ABCDE1234F', 'a_voter_id' => 'ZZQ1234567'))->condition('a_id', $a)->execute();
	$f = dh_autofill_fields($a, 'application_form');
	t_eq(array($f['photo_id_type'], $f['photo_id_val']), array('Pancard', 'ABCDE1234F'), 'ID: next one set');
	t_eq(array($f['ae_id_issued_by'], $f['ae_id_issued']), array('zzq-ae_id_issued_by', '2001-02-03'), 'PAN is the usual ID here: its issuer and date stay');
	$f = dh_autofill_fields($a, 'vi_application_form');
	t_eq(array($f['photo_id_type'], $f['photo_id_val']), array('Voter ID', 'ZZQ1234567'), 'VI form: Voter ID when there is no passport');
	t_ok(!isset($f['ae_id_issued_by']) && !isset($f['ae_id_issued']), 'VI form: no issuer or date of the PAN');
	db_update('dh_applicant')->fields(array('a_voter_id' => ''))->condition('a_id', $a)->execute();
	$f = dh_autofill_fields($a, 'vi_one_day_application_form');
	t_ok(!isset($f['photo_id_type']) && !isset($f['photo_id_val']) && !isset($f['ae_id_issued_by']) && !isset($f['ae_id_issued']),
		'VI form, only a PAN: no ID fields at all');
	db_update('dh_applicant')->fields(array('a_pancard' => ''))->condition('a_id', $a)->execute();
	$f = dh_autofill_fields($a, 'application_form');
	t_ok(!isset($f['photo_id_type']) && !isset($f['ae_id_issued_by']) && !isset($f['ae_id_issued']), 'no ID at all: no issuer or date either');
	t_eq(dh_autofill_fields(-1, 'application_form'), array(), 'unknown application: nothing');
	t_eq(dh_autofill_fields($a, 'no_such_form'), array(), 'unknown form: nothing');

	// Phones that are not real numbers are left out; no country code anywhere means no other phones.
	db_update('dh_applicant')->fields(array('a_phone_home' => 'abc', 'a_emergency_num' => '12', 'a_phone_office' => '0044 20 7946 0958', 'a_gender' => 'X'))->condition('a_id', $a)->execute();
	$f = dh_autofill_fields($a, 'application_form');
	t_ok(!isset($f['a_phone_home']) && !isset($f['a_emergency_num']), 'not a real number: left out');
	t_eq($f['a_phone_office'], '+442079460958', '00 prefix read as an international number');
	t_ok(!isset($f['a_gender']), 'unknown gender code: left out');
	db_update('dh_applicant')->fields(array('a_mob_country' => 0, 'a_phone_office' => '9876543211', 'a_gender' => 'm'))->condition('a_id', $a)->execute();
	$f = dh_autofill_fields($a, 'application_form');
	t_ok(!isset($f['a_phone_office']) && !isset($f['a_phone_mobile']), 'no country code anywhere: no phones');
	t_eq($f['a_gender'], 'Male', 'lowercase m is Male');

	// City fallback: no dh_city name, so the typed city is used.
	db_update('dh_applicant')->fields(array('a_city' => 0, 'a_city_str' => ' Zzqtown '))->condition('a_id', $a)->execute();
	t_eq(dh_autofill_fields($a, 'application_form')['a_city'], 'Zzqtown', 'city: a_city 0 falls back to a_city_str');
	db_update('dh_applicant')->fields(array('a_city' => 99999999, 'a_city_str' => 'Zzqtown'))->condition('a_id', $a)->execute();
	t_eq(dh_autofill_fields($a, 'application_form')['a_city'], 'Zzqtown', 'city: no matching dh_city row falls back too');
	db_update('dh_applicant')->fields(array('a_city' => 0, 'a_city_str' => ''))->condition('a_id', $a)->execute();
	t_ok(!array_key_exists('a_city', dh_autofill_fields($a, 'application_form')), 'city: nothing at all is left out');
});
// Phones: a + after brackets is still international; text, extensions or two numbers are left out.
t_eq(dh_autofill_phone(1, '(+91) 98765 43212'), '+919876543212', '(+91) is international');
t_eq(dh_autofill_phone(44, '020 7946 0958 ext 12'), '', 'number with an extension: left out');
t_eq(dh_autofill_phone(91, '9876543212 / 9876543213'), '', 'two numbers in one field: left out');
t_eq(dh_autofill_phone(91, '98765-43212'), '+919876543212', 'dashes are fine');

// The code email: the code leads the subject, so mail apps never group two codes; no personal data.
$m = dh_autofill_code_email('012345');
t_eq($m['subject'], '012345 is your code for your Vipassana application', 'code first in the subject');
t_ok(strpos($m['text'], '012345') !== FALSE && strpos($m['text'], '10 minutes') !== FALSE, 'body has the code and the expiry');
t_done();
