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
