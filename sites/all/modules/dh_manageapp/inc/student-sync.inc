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

/* Two database values are the same: NULL equals only NULL, anything else is compared as text ('1' equals 1). */
function dh_value_same($a, $b)
{
	if ($a === NULL || $b === NULL)
		return $a === $b;
	return (string) $a === (string) $b;
}

/* The student already has a record of a LATER course than this one (later start, or the same start and a higher id).
   Such a student's stored data came from the later course and is newer than anything this course can say. */
function dh_student_has_newer_course($sid, $course_id)
{
	return (bool) db_query('select 1 from dh_student_course sc join dh_course c on c.c_id = sc.sc_course join dh_course me on me.c_id = :c1
		where sc.sc_student = :s and sc.sc_course <> :c2 and (c.c_start > me.c_start or (c.c_start = me.c_start and c.c_id > me.c_id)) limit 1',
		array(':c1' => $course_id, ':c2' => $course_id, ':s' => $sid))->fetchField();
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
	if (!$old)
		throw new Exception("Student $sid not found");
	/* Own record: this course is the only course the student has, and the student was made from this
	   applicant (a course record of this course from this applicant or with no applicant noted, or a
	   'created' log row for this applicant). Anything else, an orphan with no course record at all
	   included, is a returning student whose name, gender, date of birth and IDs are protected. */
	$own = FALSE;
	if (isset($ctx['own']))
		$own = (bool) $ctx['own'];                                 // the caller knows (a course record claimed by name: never own)
	elseif (!db_query('select 1 from dh_student_course where sc_student = :s and sc_course <> :c limit 1', array(':s' => $sid, ':c' => $ctx['course']))->fetchField())
		$own = (bool) db_query('select 1 from dh_student_course where sc_student = :s and sc_course = :c and (sc_applicant = :a or sc_applicant is null) limit 1',
				array(':s' => $sid, ':c' => $ctx['course'], ':a' => $ctx['applicant']))->fetchField()
			|| (bool) db_query("select 1 from dh_student_log where sl_student = :s and sl_action = 'created' and sl_applicant = :a limit 1",
				array(':s' => $sid, ':a' => $ctx['applicant']))->fetchField();
	/* Newer data wins: when the student already has a later course, this (older) course only fills EMPTY fields.
	   No stored value is changed (contact, work, medical, photo, IDs, name), whatever the match was. */
	$newer = isset($ctx['newer']) ? $ctx['newer'] : dh_student_has_newer_course($sid, $ctx['course']);
	$id_kind = array('s_aadhar' => 'aadhar', 's_pancard' => 'pancard', 's_voter_id' => 'voter_id', 's_passport' => 'passport');
	$set = array();
	foreach ($new as $f => $v)
	{
		$v = trim(($v === NULL) ? '' : (string) $v);
		$was = trim(($old[$f] === NULL) ? '' : (string) $old[$f]);
		if ($f === 's_dob')
		{
			if (!dh_dob_known($v))
				$v = '';
			if (!dh_dob_known($was))
				$was = '';
		}
		if (isset($id_kind[$f]))
		{
			if ($v !== '' && dh_id_is_junk($id_kind[$f], $v))
				$v = '';                                               // a junk ID is never written
			if ($was !== '' && dh_id_is_junk($id_kind[$f], $was))
				$was = '';                                             // a stored junk ID counts as empty
		}
		if ($v === '' || $v === $was)
			continue;                                              // a blank never wipes; nothing changed
		if ($newer && $was !== '')
			continue;                                              // a later course already gave this value
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
	{
		$set['s_first'] = 1;
		if ($ctx['batch'] && (string) $old['s_first'] !== '1')
			dh_student_log(array('sl_student' => $sid, 'sl_action' => 'updated', 'sl_field' => 's_first', 'sl_old' => (string) $old['s_first'], 'sl_new' => '1') + $log);
	}
	$set += $now;          // s_updated and s_updated_by are audit stamps: not logged, not reversed by an undo
	dh_finalize_fit('dh_student', $set, $ctx['applicant']);
	db_update('dh_student')->fields($set)->condition('s_id', $sid)->execute();
	return $sid;
}

/* One row in the course Change Log (dh_course_log), e.g. the finalize summary or a reopen reason. */
function dh_course_log_note($course_id, $field, $text, $uid)
{
	$centre = (int) db_query('select c_center from dh_course where c_id = :c', array(':c' => $course_id))->fetchField();
	db_insert('dh_course_log')->fields(array('crl_course' => $course_id, 'crl_center' => $centre, 'crl_action' => 'update', 'crl_field' => $field,
		'crl_old' => '', 'crl_new' => $text, 'crl_user' => (int) $uid, 'crl_tstamp' => date('Y-m-d H:i:s')))->execute();
}

function dh_course_sync_summary_text($sum)
{
	$parts = array($sum['added'].' added');
	if ($sum['removed']) $parts[] = $sum['removed'].' removed';
	if ($sum['copies']) $parts[] = $sum['copies'].($sum['copies'] == 1 ? ' copy' : ' copies').' removed';
	if ($sum['students_removed']) $parts[] = $sum['students_removed'].' student '.($sum['students_removed'] == 1 ? 'record' : 'records').' removed';
	$parts[] = $sum['emails'].' '.($sum['emails'] == 1 ? 'email' : 'emails').' sent';
	if (!empty($sum['mail_failed'])) $parts[] = $sum['mail_failed'].' '.($sum['mail_failed'] == 1 ? 'email' : 'emails').' failed';
	if ($sum['errors']) $parts[] = $sum['errors'].' failed (see the Finalization Error log)';
	return $sum['students'].' students: '.implode(', ', $parts);
}

/* Photo and application PDF into the final folders. files: 'copy' (download and upload, as finalize
   always did), 's3copy' (copy inside S3, skipped when already there), 'count' (dry run), 'none'.
   Returns the paths to store on the student and course records: a path comes back only when the
   final file is known to be there. $sum: files_copied, files_missing (S3 says the source is not there),
   files_failed (S3 gave an error, or a copy failed; already in the log), bytes. */
function dh_course_sync_files($r, $sid, $opts, &$sum)
{
	$mode = $opts['files'];
	if (!in_array($mode, array('none', 'copy', 's3copy', 'count'), TRUE))
		throw new Exception("Unknown files mode: $mode");
	$head = isset($opts['s3_head']) ? $opts['s3_head'] : 's3_head_file';
	$copy = isset($opts['s3_copy']) ? $opts['s3_copy'] : 's3_copy_file';
	$out = array('s_photo' => NULL, 'sc_uri' => NULL);
	if ($mode === 'none')
		return $out;
	$bucket = variable_get('s3_bucket_name', '');
	$jobs = array();
	if (!empty($r['a_photo']))
		$jobs['s_photo'] = array(preg_replace('#^private:/+#', '', $r['a_photo']), 'final/photo-id/'.$r['a_center'].'/'.$r['a_course'].'/'.sprintf('%08d', $sid).'.jpg');
	if (!empty($r['a_uri']))
		$jobs['sc_uri'] = array(preg_replace('#^private:/+#', '', $r['a_uri']), 'final/applications/'.$r['a_center'].'/'.$r['a_course'].'/'.sprintf('%08d', $sid).'-'.sprintf('%06d', $r['a_course']).'.pdf');
	foreach ($jobs as $k => $j)
	{
		list($from, $to) = $j;
		if ($mode === 'count' || $mode === 's3copy')
		{
			// A head gives the size, FALSE (S3 says it is not there) or NULL (some other error, already logged).
			$size = $head($bucket, $from);
			if ($size === FALSE) { $sum['files_missing']++; continue; }
			if ($size === NULL) { $sum['files_failed']++; continue; }
			if ($mode === 'count') { $sum['files_copied']++; $sum['bytes'] += $size; continue; }      // would be copied
			$there = $head($bucket, $to);
			if ($there === NULL) { $sum['files_failed']++; continue; }      // cannot tell if it is there: do not copy blind
			if ($there === FALSE)
			{
				$cp = $copy($bucket, $from, $to);
				if (!$cp['success']) { $sum['files_failed']++; watchdog('S3Photo', $cp['message']); continue; }
			}
			$out[$k] = 'private://'.$to;
			$sum['files_copied']++;
			$sum['bytes'] += $size;
			continue;
		}
		$out[$k] = 'private://'.$to;
		$ret = s3_get_file($bucket, $from, 'file');
		if (!$ret['success']) { $sum['files_missing']++; watchdog('S3Photo', $ret['message']); continue; }
		$put = s3_put_file($bucket, $ret['data'], $to);
		@unlink($ret['data']);
		if ($put['success']) $sum['files_copied']++; else watchdog('S3Photo', $put['message']);
	}
	return $out;
}

/* Remove a student record that has no course left and no referral pointing to it. A patrika subscription is kept; its
   student link is set to 0 ("not a student"). Returns 'courses' (kept: the student has other courses), 'referral'
   (kept: a referral points to it) or 'removed'. */
function dh_student_remove_if_empty($sid, $log, &$sum)
{
	// dh_applicant.a_student / a_old_student are not checked on purpose: unused, unindexed, a lookup is a full scan of 3.2 million rows.
	if (db_query('select 1 from dh_student_course where sc_student = :s limit 1', array(':s' => $sid))->fetchField())
		return 'courses';
	if (db_query('select 1 from dh_referral where r_student = :s limit 1', array(':s' => $sid))->fetchField())
		return 'referral';
	foreach (db_query('select * from dh_student_course_input where sci_student = :s', array(':s' => $sid))->fetchAll(PDO::FETCH_ASSOC) as $sci)
		dh_student_log(array('sl_student' => $sid, 'sl_action' => 'sci-removed', 'sl_old' => json_encode($sci)) + $log);
	db_delete('dh_student_course_input')->condition('sci_student', $sid)->execute();
	foreach (db_query('select p_id from dh_patrika where p_student = :s', array(':s' => $sid))->fetchCol() as $pid)
	{
		dh_student_log(array('sl_student' => $sid, 'sl_action' => 'patrika-unlinked', 'sl_old' => (string) $pid) + $log);
		db_update('dh_patrika')->fields(array('p_student' => 0))->condition('p_id', $pid)->execute();
	}
	$row = db_query('select * from dh_student where s_id = :s', array(':s' => $sid))->fetchAssoc();
	dh_student_log(array('sl_student' => $sid, 'sl_action' => 'student-removed', 'sl_old' => json_encode($row)) + $log);
	db_delete('dh_student')->condition('s_id', $sid)->execute();
	$sum['students_removed']++;
	return 'removed';
}

/* No student matched an attended applicant. Before a new student is made: this course's own old record (no applicant
   noted, as the old finalize wrote them) of the same person, so one person does not become two students. A candidate is
   such a record whose student has the same first and last name (trimmed, any case), or the same first name and the same
   known date of birth. Records claimed earlier in this run ($claimed: sc_id => TRUE) and students that an attended
   applicant of this course already has are not candidates. Returns the record (sc_id, sc_student) when all candidates
   are one student (its first record), else NULL (none, or two people: unclear, so a new student is made as before). */
function dh_course_record_claim($course_id, $a, $claimed)
{
	$f = trim((string) $a['a_f_name']);
	if ($f === '')
		return NULL;
	$l = trim((string) $a['a_l_name']);
	$dob = trim((string) $a['a_dob']);
	$found = array();
	foreach (db_query("select sc.sc_id, sc.sc_student, s.s_f_name, s.s_l_name, s.s_dob from dh_student_course sc join dh_student s on s.s_id = sc.sc_student
		where sc.sc_course = :c1 and sc.sc_applicant is null and not exists (select 1 from dh_student_course x join dh_applicant xa on xa.a_id = x.sc_applicant
			where x.sc_course = :c2 and x.sc_student = sc.sc_student and xa.a_attended = 1 and xa.a_type in ('Student', 'Sevak'))
		order by sc.sc_id", array(':c1' => $course_id, ':c2' => $course_id))->fetchAll(PDO::FETCH_ASSOC) as $r)
	{
		if (isset($claimed[$r['sc_id']]) || strcasecmp(trim((string) $r['s_f_name']), $f) !== 0)
			continue;
		$same_last = ($l !== '' && strcasecmp(trim((string) $r['s_l_name']), $l) === 0);
		$same_dob = (dh_dob_known($dob) && dh_dob_known($r['s_dob']) && trim((string) $r['s_dob']) === $dob);
		if (($same_last || $same_dob) && !isset($found[$r['sc_student']]))
			$found[$r['sc_student']] = array('sc_id' => (int) $r['sc_id'], 'sc_student' => (int) $r['sc_student']);
	}
	return (count($found) == 1) ? reset($found) : NULL;
}

/* Bring the student records of one course in line with its attended students and servers.
   $opts: mail (send the Attended email to those who never got it), mail_fn, files, batch, uid,
   mark_mailed (no email: date the attended students as emailed, with the course's finalize time or end date; the back-fill),
   log_fn (where the log rows go; a dry run keeps them until after its rollback). */
function dh_course_sync($course_id, $opts = array())
{
	global $user;
	$opts += array('mail' => TRUE, 'mail_fn' => 'dh_send_letter', 'files' => 'copy', 'batch' => NULL, 'uid' => (int) $user->uid,
		'mark_mailed' => FALSE, 'log_fn' => 'watchdog');
	if (!in_array($opts['files'], array('none', 'copy', 's3copy', 'count'), TRUE))
		throw new Exception("Unknown files mode: {$opts['files']}");      // before any write, not once per applicant
	$wd = $opts['log_fn'];
	$sum = array_fill_keys(array('students', 'added', 'updated', 'removed', 'copies', 'students_removed', 'removed_kept', 'emails', 'mail_failed', 'errors',
		'files_copied', 'files_missing', 'files_failed', 'bytes'), 0);
	$attended = db_query("select td_val1 from dh_type_detail where td_type = 'COURSE-SYSTEM-STATUS' and td_key = 'STATUS-ATTENDED'")->fetchField();
	$mailed_at = NULL;                                              // mark_mailed: the date the course counts as emailed
	if ($opts['mark_mailed'])
	{
		$cd = db_query('select c_finalized_tstamp, c_end from dh_course where c_id = :c', array(':c' => $course_id))->fetchAssoc();
		$mailed_at = ($cd && dh_dob_known($cd['c_finalized_tstamp'])) ? $cd['c_finalized_tstamp']
			: (($cd && dh_dob_known($cd['c_end'])) ? $cd['c_end'].' 00:00:00' : date('Y-m-d H:i:s'));
	}
	$claimed = array();                                             // old course records claimed by name in this run (sc_id => TRUE)
	$teacher = array('M' => array(), 'F' => array());
	foreach (db_query('select ct_teacher, ct_group, t_gender from dh_course_teacher left join dh_teacher on ct_teacher = t_id where ct_course = :c', array(':c' => $course_id)) as $t)
		$teacher[$t->t_gender][$t->ct_group] = $t->ct_teacher;
	$log = array('sl_course' => $course_id, 'sl_batch' => $opts['batch'], 'sl_user' => $opts['uid']);
	$keep = array(); $kept_students = array();

	$res = db_query("select * from dh_applicant left join dh_applicant_course on a_id = ac_applicant left join dh_applicant_attended on a_id = aa_applicant
		left join dh_applicant_extra ae on a_id = ae_applicant where a_course = :c and a_attended = 1 and a_type in ('Student', 'Sevak') order by a_id", array(':c' => $course_id));
	foreach ($res as $o)
	{
		$r = (array) $o;
		$sum['students']++;
		try
		{
			$ctx = array('course' => $course_id, 'applicant' => (int) $r['a_id'], 'batch' => $opts['batch'], 'uid' => $opts['uid']);
			$m = dh_student_find($r);
			$sc = db_query('select sc_id, sc_student from dh_student_course where sc_course = :c and sc_applicant = :a order by sc_id limit 1', array(':c' => $course_id, ':a' => $r['a_id']))->fetchAssoc();
			if ($sc)
			{
				$sid = (int) $sc['sc_student'];
				$strong = ($m['sid'] == $sid) && $m['strong'];
			}
			else
			{
				$sid = $m['sid']; $strong = $m['strong'];
				if ($sid)
					$sc = db_query('select sc_id, sc_student from dh_student_course where sc_course = :c and sc_student = :s and sc_applicant is null order by sc_id limit 1', array(':c' => $course_id, ':s' => $sid))->fetchAssoc();
				elseif ($sc = dh_course_record_claim($course_id, $r, $claimed))
				{
					// No match, but this course's own old record of the same person: a weak match (how 'course-record').
					// Not strong and not own: no name change; gender, date of birth and IDs only fill empty boxes.
					$m = array('sid' => $sc['sc_student'], 'how' => 'course-record', 'strong' => FALSE);
					$sid = $m['sid']; $strong = FALSE; $ctx['own'] = FALSE;
					$claimed[$sc['sc_id']] = TRUE;
				}
			}
			$pre_sid = (int) $sid;                                 // 0 = this run makes the student
			$newer = $pre_sid ? dh_student_has_newer_course($pre_sid, $course_id) : FALSE;
			$ctx['newer'] = $newer;
			$sid = dh_student_apply($sid, $r, $strong, $ctx);

			$c = array('sc_student' => $sid, 'sc_applicant' => (int) $r['a_id'], 'sc_center' => $r['a_center'], 'sc_course' => $r['a_course'],
				'sc_teacher' => isset($teacher[$r['a_gender']][$r['aa_group']]) ? $teacher[$r['a_gender']][$r['aa_group']] : NULL,
				'sc_group' => $r['aa_group'], 'sc_acco' => $r['aa_section'].'-'.$r['aa_acco'], 'sc_laundry' => $r['aa_laundry'], 'sc_valuable' => $r['aa_valuable'],
				'sc_cell' => $r['aa_cell'], 'sc_chowky' => $r['aa_chowky'], 'sc_chair' => $r['aa_chair'], 'sc_seat_row' => $r['aa_seat_row'], 'sc_seat_col' => $r['aa_seat_col'],
				'sc_comment' => $r['aa_comment'], 'sc_desc_physical' => $r['ae_desc_physical'], 'sc_desc_mental' => $r['ae_desc_mental'],
				'sc_addiction_past' => $r['ae_desc_addiction_past'], 'sc_addiction_current' => $r['ae_desc_addiction_current'],
				'sc_desc_other_technique' => $r['ae_desc_other_technique'], 'sc_desc_medication' => $r['ae_desc_medication'], 'sc_teach_other_details' => $r['ae_teach_other_details']);
			$files = dh_course_sync_files($r, $sid, $opts, $sum);
			if ($files['sc_uri']) $c['sc_uri'] = $files['sc_uri'];
			if ($files['s_photo'])
			{
				// The final photo path. An older course never replaces the photo of a student who has a later course.
				$cur = db_query('select s_photo from dh_student where s_id = :s', array(':s' => $sid))->fetchField();
				if (!dh_value_same($cur, $files['s_photo']) && (!$newer || trim((string) $cur) === trim((string) $r['a_photo'])))
				{
					if ($opts['batch'] && $pre_sid)
						dh_student_log(array('sl_student' => $sid, 'sl_applicant' => $r['a_id'], 'sl_action' => 'updated', 'sl_field' => 's_photo', 'sl_old' => (string) $cur, 'sl_new' => $files['s_photo']) + $log);
					db_update('dh_student')->fields(array('s_photo' => $files['s_photo']))->condition('s_id', $sid)->execute();
				}
			}
			dh_finalize_fit('dh_student_course', $c, $r['a_id']);
			if ($sc)
			{
				if ($opts['batch'])
				{
					// Batch mode: what an existing (or claimed) course record had before, so an undo can put it back.
					$was = db_query('select * from dh_student_course where sc_id = :i', array(':i' => $sc['sc_id']))->fetchAssoc();
					$chg = array();
					foreach ($c as $k => $v)
						if ($was && !dh_value_same($was[$k], $v))
							$chg[$k] = $v;
					if ($chg)
						dh_student_log(array('sl_student' => $sid, 'sl_applicant' => $r['a_id'], 'sl_action' => 'course-updated', 'sl_old' => json_encode($was), 'sl_new' => json_encode($chg)) + $log);
				}
				db_update('dh_student_course')->fields($c)->condition('sc_id', $sc['sc_id'])->execute();
				$keep[] = (int) $sc['sc_id']; $sum['updated']++;
			}
			else
			{
				$id = (int) db_insert('dh_student_course')->fields($c)->execute();
				$keep[] = $id; $sum['added']++;
				dh_student_log(array('sl_student' => $sid, 'sl_applicant' => $r['a_id'], 'sl_action' => 'course-added', 'sl_new' => (string) $id) + $log);
			}
			$kept_students[$sid] = TRUE;

			$sci = array('sci_10d' => $r['ac_10d'] ? $r['ac_10d'] : '1', 'sci_stp' => $r['ac_stp'] ? $r['ac_stp'] : '0', 'sci_spl' => $r['ac_spl'] ? $r['ac_spl'] : '0',
				'sci_20d' => $r['ac_20d'] ? $r['ac_20d'] : '0', 'sci_30d' => $r['ac_30d'] ? $r['ac_30d'] : '0', 'sci_45d' => $r['ac_45d'] ? $r['ac_45d'] : '0',
				'sci_60d' => $r['ac_60d'] ? $r['ac_60d'] : '0', 'sci_tsc' => $r['ac_tsc'] ? $r['ac_tsc'] : '0', 'sci_teacher' => $r['ac_teacher'] ? $r['ac_teacher'] : '0',
				'sci_service' => $r['ac_service'] ? $r['ac_service'] : '0', 'sci_first_year' => $r['ac_first_year'], 'sci_first_month' => $r['ac_first_month'],
				'sci_first_day' => $r['ac_first_day'], 'sci_first_location_str' => $r['ac_first_location_str'], 'sci_first_teacher_str' => $r['ac_first_teacher_str']);
			dh_finalize_fit('dh_student_course_input', $sci, $r['a_id']);
			$sci_id = db_query('select sci_id from dh_student_course_input where sci_student = :s order by sci_id desc limit 1', array(':s' => $sid))->fetchField();
			if ($sci_id)
			{
				if (!$newer)                                       // a later course already gave this student their numbers: keep them
				{
					if ($opts['batch'])
					{
						$was = db_query('select * from dh_student_course_input where sci_id = :i', array(':i' => $sci_id))->fetchAssoc();
						$chg = array();
						foreach ($sci as $k => $v)
							if ($was && !dh_value_same($was[$k], $v))
								$chg[$k] = $v;
						if ($chg)
							dh_student_log(array('sl_student' => $sid, 'sl_applicant' => $r['a_id'], 'sl_action' => 'sci-updated', 'sl_old' => json_encode($was), 'sl_new' => json_encode($chg)) + $log);
					}
					db_update('dh_student_course_input')->fields($sci)->condition('sci_id', $sci_id)->execute();
				}
			}
			else
			{
				$new_sci = (int) db_insert('dh_student_course_input')->fields($sci + array('sci_student' => $sid))->execute();
				if ($opts['batch'] && $pre_sid)                    // a student made in this run is removed with its numbers by the undo of 'created'
					dh_student_log(array('sl_student' => $sid, 'sl_applicant' => $r['a_id'], 'sl_action' => 'sci-added', 'sl_new' => (string) $new_sci) + $log);
			}

			if ($opts['mail'] && empty($r['a_attended_mail']))
			{
				// Claim first: only the run that dates the applicant sends (two finalize runs of one course never both send).
				// The date stays even when the letter cannot go out or the send fails (nobody gets it twice).
				$won = db_query('update dh_applicant set a_attended_mail = :t where a_id = :a and a_attended_mail is null',
					array(':t' => date('Y-m-d H:i:s'), ':a' => $r['a_id']), array('return' => Database::RETURN_AFFECTED));
				if ($won == 1)
				{
					try
					{
						$fn = $opts['mail_fn'];
						$ret = $fn('applicant', $r['a_id'], $attended);
						if (empty($ret['error'])) $sum['emails']++;   // a letter that could not go out (no email or mobile) is not counted
					}
					catch (Throwable $t)
					{
						// A mail failure is not a sync failure: it does not stop the removals.
						$sum['mail_failed']++;
						$wd('Finalization Error', 'Applicant @a, course @c: the Attended email failed: @m', array('@a' => (int) $r['a_id'], '@c' => (int) $course_id, '@m' => $t->getMessage()), WATCHDOG_ERROR);
					}
				}
			}
			elseif ($opts['mark_mailed'] && !$opts['mail'] && empty($r['a_attended_mail']))
				db_update('dh_applicant')->fields(array('a_attended_mail' => $mailed_at))->condition('a_id', $r['a_id'])->isNull('a_attended_mail')->execute();
		}
		catch (Throwable $t)
		{
			$sum['errors']++;
			$wd('Finalization Error', "Applicant @a, course @c: @m\n@trace", array('@a' => (int) $r['a_id'], '@c' => (int) $course_id, '@m' => $t->getMessage(), '@trace' => $t->getTraceAsString()), WATCHDOG_NOTICE);
		}
	}

	if ($sum['errors'])
		$wd('Finalization', 'Course @c: removals skipped because @n applicant(s) failed.', array('@c' => $course_id, '@n' => $sum['errors']), WATCHDOG_WARNING);
	else
	{
		$q = 'select * from dh_student_course where sc_course = :c'.($keep ? ' and sc_id not in (:keep)' : '');
		$args = array(':c' => $course_id) + ($keep ? array(':keep' => $keep) : array());
		foreach (db_query($q, $args)->fetchAll(PDO::FETCH_ASSOC) as $g)
		{
			$copy = isset($kept_students[$g['sc_student']]);
			dh_student_log(array('sl_student' => $g['sc_student'], 'sl_applicant' => $g['sc_applicant'], 'sl_action' => $copy ? 'copy-removed' : 'course-removed', 'sl_old' => json_encode($g)) + $log);
			db_delete('dh_student_course')->condition('sc_id', $g['sc_id'])->execute();
			if ($copy) { $sum['copies']++; continue; }
			$sum['removed']++;
			if (dh_student_remove_if_empty((int) $g['sc_student'], $log, $sum) === 'courses')
				$sum['removed_kept']++;                            // the student keeps other courses (worth a look: maybe one person split in two)
		}
	}

	db_delete('dh_course_stat')->condition('cs_course', $course_id)->execute();
	foreach (db_query('select a_gender, a_status, a_old, count(a_id) tot from dh_applicant where a_course = :c group by a_gender, a_status, a_old', array(':c' => $course_id)) as $s)
		db_insert('dh_course_stat')->fields(array('cs_course' => $course_id, 'cs_old' => $s->a_old, 'cs_gender' => $s->a_gender, 'cs_status' => $s->a_status, 'cs_count' => $s->tot))->execute();
	return $sum;
}

/* The Drupal lock that finalize_course() and the back-fill tool hold while they sync one course. */
function dh_finalize_lock_name($course_id)
{
	return 'dh_finalize_'.(int) $course_id;
}

/* TRUE, or a message saying why this user cannot reopen this course. */
function dh_course_can_reopen($course_id, $account = NULL)
{
	global $user;
	$account = $account ? $account : $user;
	$c = db_query('select c_finalized, c_finalized_tstamp, c_finalize_requested, c_processed, c_deleted from dh_course where c_id = :c', array(':c' => $course_id))->fetchAssoc();
	if (!$c || $c['c_deleted'])
		return t('Course not found.');
	if (!$c['c_finalized'])
		return t('This course is not finalized.');
	// Still running: the cron has not picked the request up yet, or a finalize (or the back-fill) is syncing the course now.
	// A flag of 1 with c_processed 1 does not count: the old cron could leave that behind.
	if (($c['c_finalize_requested'] && $c['c_processed'] <= 0) || !lock_may_be_available(dh_finalize_lock_name($course_id)))
		return t('Finalize is still running for this course. Try again in 10 minutes.');
	if (user_access('reopen any finalized course', $account))
		return TRUE;
	if (!user_access('manage course', $account))
		return t('You are not allowed to reopen courses.');
	$ts = strtotime((string) $c['c_finalized_tstamp']);
	if ($ts > 0 && $ts >= time() - 30 * 86400)
		return TRUE;
	return t('This course was finalized more than 30 days ago. Ask a DIPI administrator to reopen it.');
}

/* Reopen a finalized course for correction. c_processed is left alone, so nothing goes to dhamma.org.
   A finalize request flag still set here is a stale one (dh_course_can_reopen() refuses while a request waits): it is
   cleared, or the cron's dhamma.org update of the reopened course (it only marks courses with no request processed)
   would never mark it processed, and the course would be sent again on every run. */
function dh_course_reopen($course_id, $reason, $uid)
{
	$now = date('Y-m-d H:i:s');
	db_update('dh_course')->fields(array('c_finalized' => 0, 'c_finalize_requested' => 0, 'c_reopened' => $now, 'c_reopened_by' => $uid, 'c_updated' => $now, 'c_updated_by' => $uid))
		->condition('c_id', $course_id)->execute();
	dh_course_log_note($course_id, 'reopen_reason', $reason, $uid);
}

/* Mark applicants who already got the Attended email, from the letter log. Only fills empty dates.
   Returns how many applicants were marked.
   dh_log is huge and busy (status changes and letter sends insert into it), so it is read in short chunks by its
   primary key l_id: one autocommit statement per chunk, so row locks are held only for a moment. A single
   INSERT ... SELECT over the whole table would lock every scanned row until it ends. */
function dh_fill_attended_mail_dates()
{
	db_query('DROP TEMPORARY TABLE IF EXISTS tmp_att_mail');
	db_query('CREATE TEMPORARY TABLE tmp_att_mail (a_id BIGINT NOT NULL PRIMARY KEY, t DATETIME NOT NULL) ENGINE=InnoDB');
	$range = db_query('SELECT MIN(l_id) AS lo, MAX(l_id) AS hi FROM dh_log')->fetchAssoc();
	if ($range['lo'] !== NULL)
		for ($from = (int) $range['lo']; $from <= (int) $range['hi']; $from += 100000)
			db_query("INSERT INTO tmp_att_mail (a_id, t) SELECT l_identifier, MIN(l_tstamp) FROM dh_log
				WHERE l_id BETWEEN :f AND :t AND l_module = 'Applicant' AND l_event = 'Letter' AND l_msg LIKE 'Letter sent%- Attended%'
				GROUP BY l_identifier ON DUPLICATE KEY UPDATE t = LEAST(t, VALUES(t))", array(':f' => $from, ':t' => $from + 99999));
	$max = (int) db_query('SELECT MAX(a_id) FROM tmp_att_mail')->fetchField();
	$n = 0;
	for ($from = 0; $from <= $max; $from += 50000)
		$n += db_query('UPDATE dh_applicant a JOIN tmp_att_mail x ON x.a_id = a.a_id SET a.a_attended_mail = x.t
			WHERE a.a_attended_mail IS NULL AND x.a_id BETWEEN :f AND :t', array(':f' => $from, ':t' => $from + 49999), array('return' => Database::RETURN_AFFECTED));
	db_query('DROP TEMPORARY TABLE IF EXISTS tmp_att_mail');
	return $n;
}

/* a_attended_mail = the date the Attended email was sent or, for courses finalized before this change, the date the
   course counted as emailed. Run once at deploy, after dh_fill_attended_mail_dates(): every attended student and server
   of a finalized course with no date yet gets the course's finalize time (or its end date), because the letter log
   cannot show every past Attended email (other letter names; migrated, old and back-filled students have no log rows),
   and a reopen + Finalize again must not email them again. A course whose first finalize is still waiting
   (c_processed <= 0, no finalize time, no student record) is left alone: its students get the email when it runs.
   Only fills empty dates. In short chunks by a_id, one autocommit statement each (dh_applicant is big and busy), and the
   waiting check is made in every chunk. Returns how many applicants were marked. $from: start at this a_id (tests). */
function dh_stamp_finalized_attended_mail($from = NULL)
{
	$range = db_query('SELECT MIN(a_id) AS lo, MAX(a_id) AS hi FROM dh_applicant')->fetchAssoc();
	if ($range['lo'] === NULL)
		return 0;
	$n = 0;
	for ($f = max((int) $range['lo'], (int) $from); $f <= (int) $range['hi']; $f += 20000)
		$n += db_query("UPDATE dh_applicant a JOIN dh_course c ON c.c_id = a.a_course
			SET a.a_attended_mail = IF(c.c_finalized_tstamp >= '1900-01-01', c.c_finalized_tstamp, IF(c.c_end >= '1900-01-01', c.c_end, NOW()))
			WHERE a.a_id BETWEEN :f AND :t AND a.a_attended = 1 AND a.a_type IN ('Student', 'Sevak') AND a.a_attended_mail IS NULL AND c.c_finalized = 1
			AND NOT (c.c_processed <= 0 AND c.c_finalized_tstamp IS NULL AND NOT EXISTS (SELECT 1 FROM dh_student_course sc WHERE sc.sc_course = c.c_id))",
			array(':f' => $f, ':t' => $f + 19999), array('return' => Database::RETURN_AFFECTED));
	return $n;
}

/* dh_send_letter() calls this after it sent a letter. An Attended letter to an applicant (sent by finalize, or by hand
   from search or the application) dates a_attended_mail, so a later finalize never sends it again. Only fills an empty
   date. Never throws: the letter has gone out already. Returns 1 when it dated the applicant, else 0. */
function dh_attended_mail_note($type, $id, $event)
{
	static $attended = NULL;
	try
	{
		if ($type !== 'applicant')
			return 0;
		if ($attended === NULL)
			$attended = (string) db_query("select td_val1 from dh_type_detail where td_type = 'COURSE-SYSTEM-STATUS' and td_key = 'STATUS-ATTENDED'")->fetchField();
		if ($attended === '' || strcasecmp(trim((string) $event), $attended) !== 0)
			return 0;
		return (int) db_query('update dh_applicant set a_attended_mail = :t where a_id = :a and a_attended_mail is null',
			array(':t' => date('Y-m-d H:i:s'), ':a' => (int) $id), array('return' => Database::RETURN_AFFECTED));
	}
	catch (Throwable $e)
	{
		watchdog('Finalization Error', 'Applicant @a: the Attended email date could not be stored: @m', array('@a' => (int) $id, '@m' => $e->getMessage()), WATCHDOG_ERROR);
		return 0;
	}
}

/* Courses of a back-fill batch, oldest first. 1 = gaps, 2 = copies, 3 = migrated centres with no
   records, 4 = other courses with no records. A course that is done no longer matches. A course that
   already carries the back-fill's own Change Log row ("Back-fill <batch id> (no emails): ..." written by the
   tool) from an applied run that was not undone is skipped too: a course whose students resolve to fewer
   records, or with an erroring applicant, would otherwise be picked again on every run. The tool names such
   a course with --courses to run it again. */
function dh_backfill_courses($batch, $migrated = array(335, 343, 344))
{
	$ran = "exists (select 1 from dh_course_log l where l.crl_course = c.c_id and l.crl_field = 'finalize_summary' and l.crl_new like 'Back-fill bf-%(no emails)%'
		and not exists (select 1 from dh_course_log u where u.crl_course = c.c_id and u.crl_field = 'finalize_summary'
			and u.crl_new = concat('Back-fill batch ', substring_index(substring_index(l.crl_new, ' ', 2), ' ', -1), ' undone')))";
	$base = "from dh_course c join dh_type_detail td on td.td_id = c.c_course_type where c.c_finalized = 1 and c.c_deleted = 0 and td.td_key <> '1-DayOSC' and not $ran";
	$att = "(select count(*) from dh_applicant a where a.a_course = c.c_id and a.a_attended = 1 and a.a_type in ('Student', 'Sevak'))";
	$has = 'exists (select 1 from dh_student_course sc where sc.sc_course = c.c_id)';
	switch ((int) $batch)
	{
		case 1: $q = "select c.c_id $base and $has and $att > (select count(distinct sc.sc_student) from dh_student_course sc where sc.sc_course = c.c_id)"; break;
		case 2: $q = "select c.c_id $base and (select count(*) - count(distinct sc.sc_student) from dh_student_course sc where sc.sc_course = c.c_id) > 0"; break;
		case 3: $q = "select c.c_id $base and c.c_center in (:m) and not $has and $att > 0"; break;
		case 4: $q = "select c.c_id $base and c.c_center not in (:m) and not $has and $att > 0"; break;
		default: return array();
	}
	$args = in_array((int) $batch, array(3, 4)) ? array(':m' => $migrated) : array();
	return array_map('intval', db_query($q.' order by c.c_start, c.c_id', $args)->fetchCol());
}

/* What a back-fill would do to one course: a real sync in a transaction that is rolled back.
   $files is 'count' (photos and PDFs are only counted) or 'none'; a dry run never copies a file,
   because a copy inside S3 cannot be rolled back. */
function dh_backfill_dry_run($course_id, $files = 'count')
{
	if (!in_array($files, array('none', 'count'), TRUE))
		throw new Exception("Dry run files mode must be none or count, not: $files");
	$msg = NULL;
	// The sync's log rows (e.g. why an applicant failed) are kept here and written after the rollback, or they would be rolled back too.
	$held = array();
	$keep = function ($type, $message, $vars = array(), $severity = WATCHDOG_NOTICE) use (&$held) { $held[] = array($type, 'Back-fill dry run: '.$message, $vars, $severity); };
	$txn = db_transaction();
	try
	{
		$sum = dh_course_sync($course_id, array('mail' => FALSE, 'files' => $files, 'batch' => 'dry-run', 'uid' => dh_system_uid(), 'log_fn' => $keep));
	}
	catch (Throwable $e)
	{
		$sum = array('errors' => 1);
		$msg = $e->getMessage();
	}
	$txn->rollback();
	foreach ($held as $w)
		watchdog($w[0], $w[1], $w[2], $w[3]);
	if ($msg !== NULL)
		watchdog('Finalization Error', 'Back-fill dry run, course @c: @m', array('@c' => $course_id, '@m' => $msg));
	return $sum;
}

/* Put one row of a student table back from a JSON log row ($old = the whole row as it was). Returns 'skipped' when the row is there already. */
function dh_backfill_restore_row($table, $key, $old)
{
	if (!is_array($old) || !isset($old[$key]))
		throw new Exception("Unreadable log row for $table");
	if (db_query("select 1 from $table where $key = :i", array(':i' => $old[$key]))->fetchField())
		return 'skipped';
	db_insert($table)->fields($old)->execute();
	return 'undone';
}

/* Undo one log row. Returns 'undone' or 'skipped' (nothing to do: the value changed since, the row is gone or
   there already, or a guard keeps the student); throws when the row cannot be undone. $ctx: column types of
   dh_student and, for 'created' rows, the students that applicants or patrika rows still point to. */
function dh_backfill_undo_row($l, &$ctx)
{
	$old = in_array($l['sl_action'], array('course-removed', 'copy-removed', 'student-removed', 'sci-removed', 'course-updated', 'sci-updated')) ? json_decode((string) $l['sl_old'], TRUE) : NULL;
	switch ($l['sl_action'])
	{
		case 'course-added':
			return db_delete('dh_student_course')->condition('sc_id', (int) $l['sl_new'])->execute() ? 'undone' : 'skipped';
		case 'course-removed':
		case 'copy-removed':
			return dh_backfill_restore_row('dh_student_course', 'sc_id', $old);
		case 'student-removed':
			return dh_backfill_restore_row('dh_student', 's_id', $old);
		case 'sci-removed':
			return dh_backfill_restore_row('dh_student_course_input', 'sci_id', $old);
		case 'course-updated':
		case 'sci-updated':
			// The columns the sync changed go back to what they were, if the row still exists.
			list($table, $key) = ($l['sl_action'] === 'course-updated') ? array('dh_student_course', 'sc_id') : array('dh_student_course_input', 'sci_id');
			$new = json_decode((string) $l['sl_new'], TRUE);
			if (!is_array($old) || !is_array($new) || !isset($old[$key]))
				throw new Exception("Unreadable log row for $table");
			$set = array_intersect_key($old, $new);
			return ($set && db_update($table)->fields($set)->condition($key, $old[$key])->execute()) ? 'undone' : 'skipped';
		case 'sci-added':
			return db_delete('dh_student_course_input')->condition('sci_id', (int) $l['sl_new'])->execute() ? 'undone' : 'skipped';
		case 'patrika-unlinked':
			return db_update('dh_patrika')->fields(array('p_student' => (int) $l['sl_student']))->condition('p_id', (int) $l['sl_old'])->condition('p_student', 0)->execute() ? 'undone' : 'skipped';
		case 'updated':
		case 'name-change':
			if ($ctx['types'] === NULL)
				$ctx['types'] = db_query('desc dh_student')->fetchAllKeyed(0, 1);
			if (!isset($ctx['types'][$l['sl_field']]))
				throw new Exception('Not a student column: ' . $l['sl_field']);
			// An empty old value was NULL or ''. A date or number column refuses '' (strict mode), so it goes back as NULL.
			$value = $l['sl_old'];
			if ($value === '' && !preg_match('/char|text|enum/i', $ctx['types'][$l['sl_field']]))
				$value = NULL;
			// Only when nobody changed it since. (s_updated / s_updated_by are audit stamps and are not put back.)
			return db_update('dh_student')->fields(array($l['sl_field'] => $value))->condition('s_id', $l['sl_student'])->condition($l['sl_field'], $l['sl_new'])->execute() ? 'undone' : 'skipped';
		case 'created':
			$s = (int) $l['sl_student'];
			if (isset($ctx['pointed'][$s])
				|| db_query('select 1 from dh_student_course where sc_student = :s limit 1', array(':s' => $s))->fetchField()
				|| db_query('select 1 from dh_referral where r_student = :s limit 1', array(':s' => $s))->fetchField())
				return 'skipped';
			db_delete('dh_student_course_input')->condition('sci_student', $s)->execute();
			return db_delete('dh_student')->condition('s_id', $s)->execute() ? 'undone' : 'skipped';
	}
	return 'skipped';
}

/* Undo one back-fill batch from dh_student_log, newest change first, in chunks of 2000 log rows.
   Returns the number of log rows per action, and (with $apply) '_undone', '_skipped', '_failed', '_failed_ids' (sl_id
   of the rows that could not be undone) and '_courses' (courses with at least one undone row, each gets a Change Log note).
   Every row is undone on its own: one that fails is counted and the undo goes on. While rows go back, the session
   SQL mode lets legacy zero dates in. Not reversed: s_updated / s_updated_by (audit stamps), course statistics
   (rebuilt from applicants), files already copied to S3. */
function dh_backfill_undo($batch, $apply = FALSE)
{
	$n = array();
	$res = array('_undone' => 0, '_skipped' => 0, '_failed' => 0, '_failed_ids' => array(), '_courses' => array());
	$ctx = array('types' => NULL, 'pointed' => array());
	if ($apply)
	{
		$mode = db_query('select @@session.sql_mode')->fetchField();
		db_query("SET SESSION sql_mode = 'STRICT_ALL_TABLES,NO_ENGINE_SUBSTITUTION'");
	}
	try
	{
		$last = NULL;
		do
		{
			$args = array(':b' => $batch) + ($last === NULL ? array() : array(':last' => $last));
			$rows = db_query('select * from dh_student_log where sl_batch = :b'.($last === NULL ? '' : ' and sl_id < :last').' order by sl_id desc limit 2000', $args)->fetchAll(PDO::FETCH_ASSOC);
			$created = array();
			foreach ($rows as $l)
			{
				$n[$l['sl_action']] = (isset($n[$l['sl_action']]) ? $n[$l['sl_action']] : 0) + 1;
				$last = $l['sl_id'];
				if ($l['sl_action'] === 'created')
					$created[] = (int) $l['sl_student'];
			}
			if (!$apply)
				continue;
			// Students that a patrika subscription points to are kept (looked up once per chunk).
			// dh_applicant.a_student / a_old_student are not checked on purpose: unused, unindexed, a lookup is a full scan of 3.2 million rows.
			$ctx['pointed'] = array();
			if ($created)
				foreach (db_query('select p_student from dh_patrika where p_student in (:i)', array(':i' => $created))->fetchCol() as $p)
					$ctx['pointed'][(int) $p] = TRUE;
			foreach ($rows as $l)
			{
				try
				{
					$r = dh_backfill_undo_row($l, $ctx);
				}
				catch (Throwable $e)
				{
					$r = 'failed';
				}
				$res['_'.$r]++;
				if ($r === 'failed')
					$res['_failed_ids'][] = (int) $l['sl_id'];
				elseif ($r === 'undone' && $l['sl_course'])
					$res['_courses'][(int) $l['sl_course']] = TRUE;
			}
		}
		while (count($rows) == 2000);
	}
	finally
	{
		if ($apply)
			db_query('SET SESSION sql_mode = :m', array(':m' => $mode));
	}
	if (!$apply)
		return $n;
	$res['_courses'] = array_keys($res['_courses']);
	foreach ($res['_courses'] as $c)
		dh_course_log_note($c, 'finalize_summary', "Back-fill batch $batch undone", dh_system_uid());
	return $n + $res;
}
