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

/* The old rows of one student number: each of its old lines joined to its old row (a line in both part_b and part_c has
   two), newest course first. */
function dh_restore_rows($old_sid)
{
	return db_query("select r.*, sc.sc_id, sc.sc_course, c.c_start from dh_student_course sc
		join dh_restore_source r on r.rs_course = sc.sc_course and r.rs_conf_no = sc.sc_conf_no
		join dh_course c on c.c_id = sc.sc_course
		where sc.sc_student = :s and sc.from_migration_script = 'Yes'
		order by c.c_start desc, sc.sc_course desc, r.rs_src, r.rs_id", array(':s' => $old_sid))->fetchAll(PDO::FETCH_ASSOC);
}

/* The student DIPI already has for this person: DIPI's own matcher with the newest old row first, then each older row that
   gives it something new (another name, email, mobile or date of birth). sid 0 = none. */
function dh_restore_find($rows)
{
	$tried = array();
	foreach ($rows as $r)
	{
		$a = dh_restore_person($r);
		$key = strtolower($a['a_f_name'].'|'.$a['a_l_name'].'|'.$a['a_email'].'|'.$a['a_phone_mobile'].'|'.$a['a_dob']);
		if (isset($tried[$key]))
			continue;
		$tried[$key] = TRUE;
		$m = dh_student_find($a);
		if ($m['sid'])
			return $m;
	}
	return array('sid' => 0, 'how' => 'none', 'strong' => FALSE);
}

/* One course-counts row per student. A restored student ($to = $old_sid) keeps the newest (highest id) of its old rows. A merged
   student ($to = the current student) keeps its own row; if it has none, the newest old row moves over. The rest are removed. */
function dh_restore_sci($old_sid, $to, $log)
{
	$old = db_query('select * from dh_student_course_input where sci_student = :s order by sci_id desc', array(':s' => $old_sid))->fetchAll(PDO::FETCH_ASSOC);
	if (!$old)
		return;
	if ($to == $old_sid)
		array_shift($old);                                         // the newest stays where it is
	elseif (!db_query('select 1 from dh_student_course_input where sci_student = :s limit 1', array(':s' => $to))->fetchField())
	{
		$keep = array_shift($old);
		dh_student_log(array('sl_student' => $to, 'sl_action' => 'sci-updated', 'sl_old' => json_encode($keep), 'sl_new' => json_encode(array('sci_student' => $to))) + $log);
		db_update('dh_student_course_input')->fields(array('sci_student' => $to))->condition('sci_id', $keep['sci_id'])->execute();
	}
	foreach ($old as $sci)
	{
		dh_student_log(array('sl_student' => $old_sid, 'sl_action' => 'sci-removed', 'sl_old' => json_encode($sci)) + $log);
		db_delete('dh_student_course_input')->condition('sci_id', $sci['sci_id'])->execute();
	}
}

/* Restore or merge one old student number. $ctx: batch, uid. Returns result ('restored', 'merged', 'unclear', or 'done': the
   number exists, or has no old line left), sid (the student its old lines now belong to; 0 for unclear, and for done with no
   line) and how (the matcher's way, for a merge). Every write is logged under the batch. */
function dh_restore_one($old_sid, $ctx)
{
	$old_sid = (int) $old_sid;
	if (db_query('select 1 from dh_student where s_id = :s', array(':s' => $old_sid))->fetchField())
		return array('result' => 'done', 'sid' => $old_sid, 'how' => '');
	$lines = db_query("select * from dh_student_course where sc_student = :s and from_migration_script = 'Yes' order by sc_id", array(':s' => $old_sid))->fetchAll(PDO::FETCH_ASSOC);
	if (!$lines)
		return array('result' => 'done', 'sid' => 0, 'how' => '');
	$rows = dh_restore_rows($old_sid);
	$found = array();
	foreach ($rows as $r)
		$found[$r['sc_id']] = TRUE;
	if (count($found) < count($lines) || !dh_restore_same_person($rows))
		return array('result' => 'unclear', 'sid' => 0, 'how' => '');     // a line with no old row, or rows of more than one person
	$log = array('sl_batch' => $ctx['batch'], 'sl_user' => $ctx['uid']);
	$newest = $rows[0];
	$m = dh_restore_find($rows);
	if ($m['sid'])
	{
		$sid = (int) $m['sid'];
		// Newer data wins: the old data only fills the current student's empty boxes; no value that is there changes.
		dh_student_apply($sid, dh_restore_person($newest), $m['strong'],
			array('course' => (int) $newest['sc_course'], 'applicant' => NULL, 'batch' => $ctx['batch'], 'uid' => $ctx['uid'], 'newer' => TRUE, 'own' => FALSE));
		foreach ($lines as $l)
		{
			dh_student_log(array('sl_student' => $sid, 'sl_course' => $l['sc_course'], 'sl_action' => 'course-updated', 'sl_old' => json_encode($l),
				'sl_new' => json_encode(array('sc_student' => $sid))) + $log);
			db_update('dh_student_course')->fields(array('sc_student' => $sid))->condition('sc_id', $l['sc_id'])->execute();
		}
		dh_restore_sci($old_sid, $sid, $log);
		return array('result' => 'merged', 'sid' => $sid, 'how' => $m['how']);
	}
	$a = dh_restore_person($newest);
	$row = array();
	foreach (dh_student_fields() as $f)
	{
		$k = str_replace('s_', 'a_', $f);
		if (array_key_exists($k, $a) && $a[$k] !== NULL && $a[$k] !== '')
			$row[$f] = $a[$k];
	}
	$oldest = end($rows);
	$now = date('Y-m-d H:i:s');
	$row += array('s_id' => $old_sid, 's_created' => $now, 's_created_by' => $ctx['uid'], 's_updated' => $now, 's_updated_by' => $ctx['uid'],
		's_first' => strtoupper(substr(trim((string) $oldest['rs_type']), 0, 1)) === 'N' ? 1 : 0, 'from_migration_script' => 'Yes');
	dh_finalize_fit('dh_student', $row, NULL);
	db_insert('dh_student')->fields($row)->execute();
	dh_student_log(array('sl_student' => $old_sid, 'sl_action' => 'restored') + $log);
	dh_restore_sci($old_sid, $old_sid, $log);
	return array('result' => 'restored', 'sid' => $old_sid, 'how' => '');
}

/* Tidy the old lines of one student: fill the room and the seat from the old row where the line has none, and remove the copy of
   a line that is there twice (the same course and confirmation number: one old application imported twice; the first stays).
   Safe to run again. Returns counts: rooms, seats, copies. */
function dh_restore_tidy_lines($sid, $log)
{
	$n = array('rooms' => 0, 'seats' => 0, 'copies' => 0);
	$seen = array();
	$lines = db_query("select * from dh_student_course where sc_student = :s and from_migration_script = 'Yes' order by sc_id", array(':s' => $sid))->fetchAll(PDO::FETCH_ASSOC);
	foreach ($lines as $l)
	{
		$key = $l['sc_course'].'|'.$l['sc_conf_no'];
		if (isset($seen[$key]))
		{
			dh_student_log(array('sl_student' => $sid, 'sl_course' => $l['sc_course'], 'sl_action' => 'copy-removed', 'sl_old' => json_encode($l)) + $log);
			db_delete('dh_student_course')->condition('sc_id', $l['sc_id'])->execute();
			$n['copies']++;
			continue;
		}
		$seen[$key] = TRUE;
		$r = db_query('select rs_acco, rs_seat from dh_restore_source where rs_course = :c and rs_conf_no = :n order by rs_src, rs_id limit 1',
			array(':c' => $l['sc_course'], ':n' => $l['sc_conf_no']))->fetchAssoc();
		if (!$r)
			continue;
		$set = array();
		$acco = trim((string) $r['rs_acco']);
		if (in_array(trim((string) $l['sc_acco']), array('', '-'), TRUE) && $acco !== '')
			$set['sc_acco'] = $acco;
		$seat = trim((string) $r['rs_seat']);
		if ($l['sc_seat_row'] === NULL && $l['sc_seat_col'] === NULL && ctype_digit($seat) && (int) $seat > 0)
			$set['sc_seat_col'] = (int) $seat;
		if (!$set)
			continue;
		dh_finalize_fit('dh_student_course', $set, NULL);
		dh_student_log(array('sl_student' => $sid, 'sl_course' => $l['sc_course'], 'sl_action' => 'course-updated', 'sl_old' => json_encode($l), 'sl_new' => json_encode($set)) + $log);
		db_update('dh_student_course')->fields($set)->condition('sc_id', $l['sc_id'])->execute();
		$n['rooms'] += isset($set['sc_acco']) ? 1 : 0;
		$n['seats'] += isset($set['sc_seat_col']) ? 1 : 0;
	}
	return $n;
}
