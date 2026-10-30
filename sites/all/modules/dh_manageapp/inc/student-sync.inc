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

/* One row in the student change log (created, name-change, updated). */
function dh_student_log($row)
{
	global $user;
	$row += array('sl_student' => NULL, 'sl_applicant' => NULL, 'sl_course' => NULL, 'sl_field' => NULL, 'sl_old' => NULL, 'sl_new' => NULL, 'sl_batch' => NULL);
	$row['sl_user'] = isset($row['sl_user']) ? $row['sl_user'] : (int) $user->uid;
	$row['sl_tstamp'] = date('Y-m-d H:i:s');
	db_insert('dh_student_log')->fields($row)->execute();
}

/* The dh_student columns finalize copies from the applicant (s_x <- a_x). */
function dh_student_fields()
{
	static $fields = NULL;
	if ($fields === NULL)
		$fields = array_values(array_diff(db_query('desc dh_student')->fetchCol(),
			array('s_id', 's_updated', 's_created', 's_created_by', 's_updated_by', 's_first')));
	return $fields;
}

/* Create or update the student record for one attended applicant. Returns the student id.
   $a = the full applicant row; $strong = the match was strong (may change the name);
   $ctx = course, applicant, batch (string or NULL), uid. A zero or unknown date counts as empty. */
function dh_student_apply($sid, $a, $strong, $ctx)
{
	$log = array('sl_applicant' => $ctx['applicant'], 'sl_course' => $ctx['course'], 'sl_batch' => $ctx['batch'], 'sl_user' => $ctx['uid']);
	$new = array();
	foreach (dh_student_fields() as $f)
	{
		$k = str_replace('s_', 'a_', $f);
		if (array_key_exists($k, $a))
			$new[$f] = $a[$k];
	}
	$now = array('s_updated' => date('Y-m-d H:i:s'), 's_updated_by' => $ctx['uid']);
	if (!$sid)
	{
		if (isset($new['s_photo']) && $new['s_photo'] === '')
			unset($new['s_photo']);
		if (isset($new['s_dob']) && !dh_dob_known($new['s_dob']))
			unset($new['s_dob']);                                  // a zero date is no date: leave the box empty
		$row = $new + $now + array('s_created_by' => $ctx['uid'], 's_first' => empty($a['a_old']) ? 1 : 0);
		dh_finalize_fit('dh_student', $row, $ctx['applicant']);
		$sid = (int) db_insert('dh_student')->fields($row)->execute();
		dh_student_log(array('sl_student' => $sid, 'sl_action' => 'created') + $log);
		return $sid;
	}
	$old = db_query('select * from dh_student where s_id = :s', array(':s' => $sid))->fetchAssoc();
	$own = !db_query('select 1 from dh_student_course where sc_student = :s and sc_course <> :c limit 1', array(':s' => $sid, ':c' => $ctx['course']))->fetchField();
	$set = array();
	foreach ($new as $f => $v)
	{
		$v = ($v === NULL) ? '' : (string) $v;
		$was = ($old[$f] === NULL) ? '' : (string) $old[$f];
		if ($f === 's_dob')
		{
			if (!dh_dob_known($v))
				$v = '';
			if (!dh_dob_known($was))
				$was = '';
		}
		if ($v === '' || $v === $was)
			continue;                                              // a blank never wipes; nothing changed
		if (!$own)
		{
			if (in_array($f, array('s_f_name', 's_m_name', 's_l_name')) && !$strong)
				continue;
			if (in_array($f, array('s_gender', 's_dob', 's_aadhar', 's_pancard', 's_voter_id', 's_passport')) && $was !== '')
				continue;
		}
		$set[$f] = $v;
		if (in_array($f, array('s_f_name', 's_m_name', 's_l_name')))
			dh_student_log(array('sl_student' => $sid, 'sl_action' => 'name-change', 'sl_field' => $f, 'sl_old' => $was, 'sl_new' => $v) + $log);
		elseif ($ctx['batch'])
			dh_student_log(array('sl_student' => $sid, 'sl_action' => 'updated', 'sl_field' => $f, 'sl_old' => $was, 'sl_new' => $v) + $log);
	}
	if (empty($a['a_old']))
		$set['s_first'] = 1;
	$set += $now;
	dh_finalize_fit('dh_student', $set, $ctx['applicant']);
	db_update('dh_student')->fields($set)->condition('s_id', $sid)->execute();
	return $sid;
}
