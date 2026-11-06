<?php
/**
 * Form autofill API for schedule.vridhamma.org. The schedule site (vri-3) calls three JSON
 * endpoints server to server: autofill/email/start, autofill/email/verify and autofill/fetch.
 * DIPI emails a one-time code, groups the email's applications into people, and returns the
 * listed fields of one person's latest application. Design: private Claude Doc
 * "Form autofill - DIPI API design" (6 Nov 2026).
 */

/** Target forms (schedule webform ids) => 'adult' or 'child'. */
function dh_autofill_forms()
{
	return array(
		'application_form' => 'adult', 'stp_application_form' => 'adult', 'long_course_application_form' => 'adult',
		'vi_application_form' => 'adult', 'one_day_application_form' => 'adult',
		'global_pagoda_application_form' => 'adult', 'vi_one_day_application_form' => 'adult',
		'children_application_form' => 'child', 'teenager_form' => 'child',
	);
}

/**
 * Course types (dh_type_detail COURSE-TYPE td_key) => what an application of that type can do:
 * 'adult' fills adult forms, 'child' fills children and teenager forms, 'count' only counts when
 * deciding one person or several. A type not listed here counts as 'count'.
 */
function dh_autofill_course_roles()
{
	return array(
		'10-Day' => 'adult', '10-DayExecutive' => 'adult', '10-DaySpecial' => 'adult', '2-DayOSC' => 'adult',
		'3-DayOSC' => 'adult', '20-DayOSC' => 'adult', '30-DayOSC' => 'adult', '45-DayOSC' => 'adult',
		'60-DayOSC' => 'adult', 'Satipatthana' => 'adult', 'TSC' => 'adult',
		'7-DayTeen' => 'child', 'Child-Teen' => 'child',
		'1-DayOSC' => 'count', 'Other' => 'count', 'ServicePeriod' => 'count', 'OSProgram' => 'count',
	);
}

/** A name part for grouping: lowercase, dots removed, trimmed, runs of spaces collapsed. */
function dh_autofill_norm($s)
{
	$s = mb_strtolower(str_replace('.', '', (string) $s), 'UTF-8');
	return trim(preg_replace('/\s+/u', ' ', $s));
}

/** "One person" = normalized first name + last name + gender. */
function dh_autofill_person_key($r)
{
	return dh_autofill_norm($r['a_f_name']).'|'.dh_autofill_norm($r['a_l_name']).'|'.strtoupper(trim((string) $r['a_gender']));
}

/**
 * The counted applications on an email: a_id, names, gender, a_created, a_center and the course
 * type key. Leaves out status Duplicate and Errors, and the test centre 302 unless the email is
 * the test email (variable autofill_test_email). a_email's collation (utf8mb4_general_ci)
 * already ignores case and trailing spaces; the a_email index makes this a quick lookup.
 */
function dh_autofill_applications($email)
{
	$email = trim((string) $email);
	if ($email === '')
		return array();
	$test = trim((string) variable_get('autofill_test_email', ''));
	$rows = db_query("select a.a_id, a.a_f_name, a.a_l_name, a.a_gender, a.a_created, a.a_center, td.td_key
		from dh_applicant a
		left join dh_course c on c.c_id = a.a_course
		left join dh_type_detail td on td.td_id = c.c_course_type
		where a.a_email = :e and ifnull(a.a_status, '') not in ('Duplicate', 'Errors')", array(':e' => $email))->fetchAll(PDO::FETCH_ASSOC);
	$out = array();
	foreach ($rows as $r)
		if ((int) $r['a_center'] != 302 || ($test !== '' && strcasecmp($test, $email) == 0))
			$out[] = $r;
	return $out;
}

/**
 * One person or several, for a target group ('adult' or 'child'). Adult forms count every
 * application; children's forms count only Children and Teenager applications. The source is
 * the person's latest application (a_created, then a_id) whose course type can fill the group.
 * Returns array('result' => 'none'|'single'|'multiple'), plus 'source' (the row) for 'single'.
 */
function dh_autofill_match($apps, $group)
{
	$roles = dh_autofill_course_roles();
	$people = array();
	$best = NULL;
	foreach ($apps as $r)
	{
		$role = isset($roles[(string) $r['td_key']]) ? $roles[(string) $r['td_key']] : 'count';
		if ($group == 'child' && $role != 'child')
			continue;
		$people[dh_autofill_person_key($r)] = TRUE;
		if ($role == $group && ($best === NULL || $r['a_created'] > $best['a_created']
			|| ($r['a_created'] == $best['a_created'] && (int) $r['a_id'] > (int) $best['a_id'])))
			$best = $r;
	}
	if (count($people) > 1)
		return array('result' => 'multiple');
	if (count($people) == 0 || $best === NULL)
		return array('result' => 'none');
	return array('result' => 'single', 'source' => $best);
}

/** "N*** B*****": the first letter of every word of the first and last name, the rest '*'. */
function dh_autofill_mask_name($first, $last)
{
	$out = array();
	foreach (preg_split('/\s+/u', trim($first.' '.$last), -1, PREG_SPLIT_NO_EMPTY) as $w)
		$out[] = mb_substr($w, 0, 1, 'UTF-8').str_repeat('*', mb_strlen($w, 'UTF-8') - 1);
	return implode(' ', $out);
}

/** A stored mobile as E.164 ('+' country code + number), or '' when it is not a valid number. */
function dh_autofill_e164($cc, $num)
{
	$c = preg_replace('/\D/', '', (string) $cc);
	$n = ltrim(preg_replace('/\D/', '', (string) $num), '0');
	if ($c === '' || (int) $c == 0 || $n === '')
		return '';
	$util = \libphonenumber\PhoneNumberUtil::getInstance();
	try
	{
		$p = $util->parse('+'.$c.$n, NULL);
		return $util->isValidNumber($p) ? $util->format($p, \libphonenumber\PhoneNumberFormat::E164) : '';
	}
	catch (\libphonenumber\NumberParseException $e)
	{
		return '';
	}
}

/** 'YYYY-MM-DD' for a real date from its parts (year 1900 or later), else ''. */
function dh_autofill_ymd($y, $m, $d)
{
	if (!is_numeric($y) || !is_numeric($m) || !is_numeric($d))
		return '';
	$y = (int) $y;
	$m = (int) $m;
	$d = (int) $d;
	return ($y >= 1900 && checkdate($m, $d, $y)) ? sprintf('%04d-%02d-%02d', $y, $m, $d) : '';
}

/** A stored DATE / DATETIME / TIMESTAMP as 'YYYY-MM-DD', or '' when it is not a real date. */
function dh_autofill_date($v)
{
	if (!preg_match('/^(\d{4})-(\d{2})-(\d{2})/', (string) $v, $m))
		return '';
	return dh_autofill_ymd($m[1], $m[2], $m[3]);
}

/**
 * The fields fetch returns for one application and target form. Only keys on these lists ever
 * leave DIPI; a column not listed here is never returned. Empty values are left out.
 */
function dh_autofill_fields($a_id, $form)
{
	$forms = dh_autofill_forms();
	$group = $forms[$form];
	$a = db_query("select * from dh_applicant where a_id = :id", array(':id' => $a_id))->fetchAssoc();
	if (!$a)
		return array();
	$ae = db_query("select * from dh_applicant_extra where ae_applicant = :id", array(':id' => $a_id))->fetchAssoc();
	$ac = db_query("select * from dh_applicant_course where ac_applicant = :id", array(':id' => $a_id))->fetchAssoc();
	$al = db_query("select * from dh_applicant_lc where al_applicant = :id", array(':id' => $a_id))->fetchAssoc();
	$ae = $ae ? $ae : array();
	$ac = $ac ? $ac : array();
	$al = $al ? $al : array();

	$f = array();
	foreach (array('a_gender', 'a_old', 'a_f_name', 'a_m_name', 'a_l_name', 'a_address', 'a_zip',
		'a_phone_home', 'a_phone_office', 'a_emergency_name', 'a_emergency_num', 'a_emergency_relation',
		'a_lang_1_level', 'a_lang_2_level', 'a_lang_discourse',
		'a_education', 'a_occupation', 'a_company', 'a_department', 'a_designation') as $k)
		$f[$k] = $a[$k];
	$f['a_dob'] = dh_autofill_date($a['a_dob']);
	$f['a_phone_mobile'] = dh_autofill_e164($a['a_mob_country'], $a['a_phone_mobile']);

	// ID: the first one set, Aadhaar > Passport > PAN > Voter ID; the full number (owner decision, 6 Nov).
	foreach (array('a_aadhar' => 'Aadhar', 'a_passport' => 'Passport', 'a_pancard' => 'Pancard', 'a_voter_id' => 'Voter ID') as $col => $type)
		if (trim((string) $a[$col]) !== '')
		{
			$f['photo_id_type'] = $type;
			$f['photo_id_val'] = trim($a[$col]);
			break;
		}

	// DIPI stores a city id; the form wants the name. Country and state go as codes and names.
	if ((int) $a['a_city'] > 0)
		$f['a_city'] = db_query("select c_name from dh_city where c_id = :id", array(':id' => $a['a_city']))->fetchField();
	if (trim((string) $a['a_country']) !== '')
	{
		$cn = db_query("select c_name from dh_country where c_code = :c", array(':c' => $a['a_country']))->fetchField();
		$sn = (trim((string) $a['a_state']) === '') ? '' : db_query("select s_name from dh_state where s_code = :s and s_country = :c",
			array(':s' => $a['a_state'], ':c' => $a['a_country']))->fetchField();
		if ($cn)
			$f['country'] = array('country_code' => $a['a_country'], 'country_name' => $cn,
				'state_code' => $sn ? $a['a_state'] : '', 'state_name' => $sn ? $sn : '');
	}

	foreach (array('ae_nationality', 'ae_id_issued_by', 'ae_seva_details') as $k)
		$f[$k] = isset($ae[$k]) ? $ae[$k] : '';
	$f['ae_id_issued'] = isset($ae['ae_id_issued']) ? dh_autofill_date($ae['ae_id_issued']) : '';
	if ($group == 'child')
		foreach (array('ae_father', 'ae_mother', 'ae_parent_course', 'ae_ch_course', 'ae_ch_course_detail', 'ae_ch_practice', 'ae_ch_practice_detail') as $k)
			$f[$k] = isset($ae[$k]) ? $ae[$k] : '';

	foreach (array('ac_teen', 'ac_10d', 'ac_stp', 'ac_spl', 'ac_20d', 'ac_30d', 'ac_45d', 'ac_60d', 'ac_tsc', 'ac_service',
		'ac_practice_details', 'ac_first_location', 'ac_first_teacher', 'ac_last_location', 'ac_last_teacher',
		'ac_first_location_str', 'ac_first_teacher_str', 'ac_last_location_str', 'ac_last_teacher_str') as $k)
		$f[$k] = isset($ac[$k]) ? $ac[$k] : '';
	if ($ac)
	{
		$f['date_first_course'] = dh_autofill_ymd($ac['ac_first_year'], $ac['ac_first_month'], $ac['ac_first_day']);
		$f['date_last_course'] = dh_autofill_ymd($ac['ac_last_year'], $ac['ac_last_month'], $ac['ac_last_day']);
	}

	if ($form == 'long_course_application_form')
		foreach (array('al_recent_sat_type', 'al_last_lc_type', 'al_last_lc_date', 'al_last_lc_location', 'al_last_lc_teacher') as $k)
			$f[$k] = isset($al[$k]) ? $al[$k] : '';

	foreach ($f as $k => $v)
		if ($v === NULL || $v === FALSE || (is_string($v) && trim($v) === ''))
			unset($f[$k]);
	return $f;
}
