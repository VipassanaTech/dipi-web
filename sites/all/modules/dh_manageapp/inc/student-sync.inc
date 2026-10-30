<?php
/* Course finalize sync: student matching, student updates and the per-course sync step.
   See the design "Correcting a Finalized Course". Every value goes into SQL as a placeholder. */

define('DH_ID_SHARED_NAMES', 3);

function dh_id_clean($v)
{
	return strtoupper(preg_replace('/[\s\-\/\.]/', '', (string) $v));
}

/* 12 digits, not starting with 0 or 1, and the Verhoeff check digit (the last digit) is right. */
function dh_aadhaar_valid($v)
{
	$s = dh_id_clean($v);
	if (!preg_match('/^[2-9][0-9]{11}$/', $s))
		return FALSE;
	static $d = array(array(0,1,2,3,4,5,6,7,8,9), array(1,2,3,4,0,6,7,8,9,5), array(2,3,4,0,1,7,8,9,5,6), array(3,4,0,1,2,8,9,5,6,7),
		array(4,0,1,2,3,9,5,6,7,8), array(5,9,8,7,6,0,4,3,2,1), array(6,5,9,8,7,1,0,4,3,2), array(7,6,5,9,8,2,1,0,4,3),
		array(8,7,6,5,9,3,2,1,0,4), array(9,8,7,6,5,4,3,2,1,0));
	static $p = array(array(0,1,2,3,4,5,6,7,8,9), array(1,5,7,6,2,8,3,0,9,4), array(5,8,0,3,7,9,6,1,4,2), array(8,9,1,6,0,4,3,5,2,7),
		array(9,4,5,3,1,2,6,8,7,0), array(4,2,8,6,5,7,3,9,0,1), array(2,7,9,3,8,0,6,4,1,5), array(7,0,4,6,9,1,3,2,5,8));
	$c = 0;
	foreach (array_reverse(str_split($s)) as $i => $n)
		$c = $d[$c][$p[$i % 8][(int) $n]];
	return $c === 0;
}

/* A value that cannot be a real ID of its kind: placeholders, runs, wrong length or format. */
function dh_id_is_junk($kind, $v)
{
	$s = dh_id_clean($v);
	if ($s === '' || preg_match('/^(.)\1*$/', $s) || preg_match('/^(NA|NIL|NONE|NULL|NO|TEST|NOTAVAILABLE)$/', $s))
		return TRUE;
	if (preg_match('/^[0-9]{6}/', $s) && strpos('0123456789', substr($s, 0, 6)) !== FALSE)
		return TRUE;
	switch ($kind)
	{
		case 'aadhar':   return !dh_aadhaar_valid($s);
		case 'pancard':  return !preg_match('/^[A-Z]{5}[0-9]{4}[A-Z]$/', $s);
		case 'voter_id': return strlen($s) < 8;
		case 'passport': return strlen($s) < 6;
	}
	return FALSE;
}

/* A real date of birth: not empty and not a zero or placeholder date ('0000-00-00' and anything before 1900). */
function dh_dob_known($d)
{
	$d = trim((string) $d);
	return $d !== '' && strcmp($d, '1900-01-01') >= 0;
}

/* An ID typed by more than DH_ID_SHARED_NAMES different first names among existing students.
   Only a TRUE answer is remembered (a value found shared stays shared for the whole run); a FALSE
   answer is counted again on every call, because a long run (the back-fill) can make a value shared. */
function dh_id_is_shared($kind, $v)
{
	static $shared = array();
	$key = $kind . '|' . $v;
	if (isset($shared[$key]))
		return TRUE;
	if ((int) db_query("select count(distinct s_f_name) from dh_student where s_$kind = :v", array(':v' => (string) $v))->fetchField() > DH_ID_SHARED_NAMES)
	{
		$shared[$key] = TRUE;
		return TRUE;
	}
	return FALSE;
}

/* The existing student for an applicant, or sid 0. IDs first (each needs the same first name or
   date of birth), then first + last name with email, mobile or date of birth. */
function dh_student_find($a)
{
	$f = trim((string) $a['a_f_name']);
	$dob = isset($a['a_dob']) ? trim((string) $a['a_dob']) : '';
	if (!dh_dob_known($dob))
		$dob = '';
	foreach (array('aadhar', 'pancard', 'voter_id', 'passport') as $k)
	{
		$v = isset($a["a_$k"]) ? trim((string) $a["a_$k"]) : '';
		if ($v === '' || dh_id_is_junk($k, $v) || dh_id_is_shared($k, $v))
			continue;
		$best = NULL;
		foreach (db_query("select s_id, s_f_name, s_dob from dh_student where s_$k = :v order by s_id", array(':v' => $v)) as $s)
		{
			$same_dob = ($dob !== '' && dh_dob_known($s->s_dob) && (string) $s->s_dob === $dob);
			$same_first = ($f !== '' && strcasecmp(trim((string) $s->s_f_name), $f) === 0);
			if (!$same_dob && !$same_first)
				continue;
			if ($best === NULL || ($same_dob && !$best['strong']))
				$best = array('sid' => (int) $s->s_id, 'how' => "id:$k", 'strong' => $same_dob);
		}
		if ($best)
			return $best;
	}
	$name = array(':f' => $f, ':l' => trim((string) $a['a_l_name']));
	foreach (array('email', 'phone_mobile', 'dob') as $k)
	{
		$v = isset($a["a_$k"]) ? trim((string) $a["a_$k"]) : '';
		if ($v === '' || $f === '' || ($k === 'dob' && !dh_dob_known($v)))
			continue;
		$sid = db_query("select s_id from dh_student where s_f_name = :f and s_l_name = :l and s_$k = :v order by s_id limit 1", $name + array(':v' => $v))->fetchField();
		if ($sid)
			return array('sid' => (int) $sid, 'how' => "name+$k", 'strong' => TRUE);
	}
	return array('sid' => 0, 'how' => 'none', 'strong' => FALSE);
}
