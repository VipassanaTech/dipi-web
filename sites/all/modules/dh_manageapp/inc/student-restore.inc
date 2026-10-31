<?php
/* The 2020 migrated students: bring back the students the April 2020 import made (numbers 82,481 to 281,166) that were
   later deleted by hand, or merge them into the student DIPI already has, so the old course lines point to real people.
   Design and plan: private Claude Docs "Restoring the 2020 migrated students". The old data comes from dh_restore_source,
   a temporary table filled from the prod-only database dipi_migration (deploy/2026-11-restore-source-fill.sql). */

define('DH_RESTORE_FIRST', 82481);
define('DH_RESTORE_LAST', 281166);

/* The temporary table of old rows: one row per old course line of part_b and part_c (a line in both has two rows).
   Safe to call twice. A CREATE TABLE commits any open transaction: never call it inside one. */
function dh_restore_source_create()
{
	db_query("CREATE TABLE IF NOT EXISTS dh_restore_source (
		rs_id bigint(20) NOT NULL AUTO_INCREMENT,
		rs_src char(1) NOT NULL,
		rs_old_id bigint(20) NOT NULL,
		rs_course bigint(20) NOT NULL,
		rs_conf_no varchar(20) NOT NULL,
		rs_type varchar(30) DEFAULT NULL,
		rs_f_name text, rs_m_name text, rs_l_name text, rs_gender varchar(10) DEFAULT NULL, rs_dob date DEFAULT NULL,
		rs_address text, rs_zip text, rs_country_name text, rs_state_name text, rs_city_name text,
		rs_email text, rs_phone_mobile text, rs_phone_home text, rs_phone_office text, rs_phone_fax text,
		rs_education text, rs_occupation text, rs_company text, rs_post text, rs_department text,
		rs_mother_tongue text, rs_other_langs text,
		rs_health_physical text, rs_health_mental text, rs_medication text, rs_intoxicants text, rs_other_technique text, rs_teach_others text,
		rs_acco text, rs_seat text,
		PRIMARY KEY (rs_id),
		KEY rs_line (rs_course, rs_conf_no)
	) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci");
}

/* An old free-text answer as a yes/no box: NULL when not answered, 0 for a "no" word (NO, N, NA, N/A, NIL, NILL, NONE, NOTHING,
   NOT APPLICABLE, 0, NULL, and the words people wrote for "no problem": GOOD, NORMAL, OK, FIT, NO PROBLEM, NO PROBLEMS),
   1 for anything else, in any script (the text itself is already on the old course line). */
function dh_restore_flag($v)
{
	if ($v === NULL || trim((string) $v) === '')
		return NULL;
	$t = preg_replace('/[^\p{L}\p{N}]+/u', ' ', (string) $v);
	if ($t === NULL)
		return 1; // not valid UTF-8: the person did write something
	$t = trim(mb_strtoupper($t, 'UTF-8'));
	return in_array($t, array('', 'NO', 'N', 'NA', 'N A', 'NIL', 'NILL', 'NONE', 'NOTHING', 'NOT APPLICABLE', 'GOOD', 'NORMAL', 'OK', 'FIT',
		'NO PROBLEM', 'NO PROBLEMS', '0', 'NULL'), TRUE) ? 0 : 1;
}

/* An old mobile number in DIPI's form: digits only; an Indian number as its 10 digits ('[91]9876543210', '919876543210' and
   '09876543210' all give '9876543210'); another country keeps its code in front ('[44]7700900123' gives '447700900123'). */
function dh_restore_mobile($v)
{
	$v = trim((string) $v);
	$cc = '';
	if (preg_match('/^\[(\d*)\]\s*(.*)$/', $v, $m))
	{
		$cc = $m[1];
		$v = $m[2];
	}
	$d = preg_replace('/\D/', '', $v);
	if ($d === '')
		return '';
	if ($cc !== '' && $cc !== '91')
		return $cc.$d;
	if (strlen($d) == 12 && substr($d, 0, 2) === '91')
		return substr($d, 2);
	if (strlen($d) == 11 && $d[0] === '0')
		return substr($d, 1);
	return $d;
}

/* DIPI's country code, state code and city id for old place names, matched by name (the database compares, any case).
   A level that does not match is empty, and so is every level under it. Remembered per name set for the run. */
function dh_restore_place($country, $state, $city)
{
	static $memo = array();
	$key = strtolower(trim((string) $country).'|'.trim((string) $state).'|'.trim((string) $city));
	if (isset($memo[$key]))
		return $memo[$key];
	$out = array('country' => '', 'state' => '', 'city' => NULL);
	if (trim((string) $country) !== '')
		$out['country'] = (string) db_query('select c_code from dh_country where c_name = :n order by c_id limit 1', array(':n' => trim((string) $country)))->fetchField();
	if ($out['country'] !== '' && trim((string) $state) !== '')
		$out['state'] = (string) db_query('select s_code from dh_state where s_country = :c and s_name = :n order by s_id limit 1',
			array(':c' => $out['country'], ':n' => trim((string) $state)))->fetchField();
	if ($out['state'] !== '' && trim((string) $city) !== '')
	{
		$id = db_query('select c_id from dh_city where c_country = :c and c_state = :s and c_name = :n order by c_id limit 1',
			array(':c' => $out['country'], ':s' => $out['state'], ':n' => trim((string) $city)))->fetchField();
		$out['city'] = $id ? (int) $id : NULL;
	}
	return $memo[$key] = $out;
}

/* The old rows of one student number are one person: the same first name, last name and gender (trimmed, any case). */
function dh_restore_same_person($rows)
{
	$seen = array();
	foreach ($rows as $r)
		$seen[strtolower(trim((string) $r['rs_f_name'])).'|'.strtolower(trim((string) $r['rs_l_name'])).'|'.strtoupper(trim((string) $r['rs_gender']))] = TRUE;
	return count($seen) == 1;
}

/* One old row as an attended applicant: the a_* keys dh_student_find() and dh_student_apply() read. a_old is always 1, so a
   merge never touches a current student's first-course box. */
function dh_restore_person($r)
{
	$t = function ($k) use ($r) { return trim((string) $r[$k]); };
	$place = dh_restore_place($r['rs_country_name'], $r['rs_state_name'], $r['rs_city_name']);
	return array(
		'a_f_name' => $t('rs_f_name'), 'a_m_name' => $t('rs_m_name'), 'a_l_name' => $t('rs_l_name'), 'a_gender' => strtoupper($t('rs_gender')),
		'a_dob' => dh_dob_known($t('rs_dob')) ? $t('rs_dob') : '',
		'a_address' => $t('rs_address'), 'a_zip' => $t('rs_zip'), 'a_country' => $place['country'], 'a_state' => $place['state'], 'a_city' => $place['city'],
		'a_email' => $t('rs_email'), 'a_phone_mobile' => dh_restore_mobile($r['rs_phone_mobile']),
		'a_phone_home' => $t('rs_phone_home'), 'a_phone_office' => $t('rs_phone_office'), 'a_phone_fax' => $t('rs_phone_fax'),
		'a_education' => $t('rs_education'), 'a_occupation' => $t('rs_occupation'), 'a_company' => $t('rs_company'),
		'a_designation' => $t('rs_post'), 'a_department' => $t('rs_department'),
		'a_lang_1' => $t('rs_mother_tongue'), 'a_langs' => $t('rs_other_langs'),
		'a_problem_physical' => dh_restore_flag($r['rs_health_physical']), 'a_problem_mental' => dh_restore_flag($r['rs_health_mental']),
		'a_medication' => dh_restore_flag($r['rs_medication']), 'a_addiction_current' => dh_restore_flag($r['rs_intoxicants']),
		'a_other_technique' => dh_restore_flag($r['rs_other_technique']), 'a_teach_others' => dh_restore_flag($r['rs_teach_others']),
		'a_old' => 1);
}
