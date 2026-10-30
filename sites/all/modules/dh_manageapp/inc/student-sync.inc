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
	if (!$old)
		throw new Exception("Student $sid not found");
	/* Own record: this course is the only course the student has, and the student was made from this
	   applicant (a course record of this course from this applicant or with no applicant noted, or a
	   'created' log row for this applicant). Anything else, an orphan with no course record at all
	   included, is a returning student whose name, gender, date of birth and IDs are protected. */
	$own = FALSE;
	if (!db_query('select 1 from dh_student_course where sc_student = :s and sc_course <> :c limit 1', array(':s' => $sid, ':c' => $ctx['course']))->fetchField())
		$own = (bool) db_query('select 1 from dh_student_course where sc_student = :s and sc_course = :c and (sc_applicant = :a or sc_applicant is null) limit 1',
				array(':s' => $sid, ':c' => $ctx['course'], ':a' => $ctx['applicant']))->fetchField()
			|| (bool) db_query("select 1 from dh_student_log where sl_student = :s and sl_action = 'created' and sl_applicant = :a limit 1",
				array(':s' => $sid, ':a' => $ctx['applicant']))->fetchField();
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
	if ($sum['errors']) $parts[] = $sum['errors'].' failed (see the Finalization Error log)';
	return $sum['students'].' students: '.implode(', ', $parts);
}

/* Photo and application PDF into the final folders. Returns the new paths to store. */
function dh_course_sync_files($r, $sid, $mode, &$sum)
{
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
		$out[$k] = 'private://'.$to;
		$ret = s3_get_file($bucket, $from, 'file');
		if (!$ret['success']) { $sum['files_missing']++; watchdog('S3Photo', $ret['message']); continue; }
		$put = s3_put_file($bucket, $ret['data'], $to);
		@unlink($ret['data']);
		if ($put['success']) $sum['files_copied']++; else watchdog('S3Photo', $put['message']);
	}
	return $out;
}

/* Remove a student record that has no course left and no referral pointing to it. */
function dh_student_remove_if_empty($sid, $log, &$sum)
{
	if (db_query('select 1 from dh_student_course where sc_student = :s limit 1', array(':s' => $sid))->fetchField()
		|| db_query('select 1 from dh_referral where r_student = :s limit 1', array(':s' => $sid))->fetchField())
		return;
	foreach (db_query('select * from dh_student_course_input where sci_student = :s', array(':s' => $sid))->fetchAll(PDO::FETCH_ASSOC) as $sci)
		dh_student_log(array('sl_student' => $sid, 'sl_action' => 'sci-removed', 'sl_old' => json_encode($sci)) + $log);
	db_delete('dh_student_course_input')->condition('sci_student', $sid)->execute();
	$row = db_query('select * from dh_student where s_id = :s', array(':s' => $sid))->fetchAssoc();
	dh_student_log(array('sl_student' => $sid, 'sl_action' => 'student-removed', 'sl_old' => json_encode($row)) + $log);
	db_delete('dh_student')->condition('s_id', $sid)->execute();
	$sum['students_removed']++;
}

/* Bring the student records of one course in line with its attended students and servers. */
function dh_course_sync($course_id, $opts = array())
{
	global $user;
	$opts += array('mail' => TRUE, 'mail_fn' => 'dh_send_letter', 'files' => 'copy', 'batch' => NULL, 'uid' => (int) $user->uid);
	$sum = array_fill_keys(array('students', 'added', 'updated', 'removed', 'copies', 'students_removed', 'emails', 'errors', 'files_copied', 'files_missing', 'bytes'), 0);
	$attended = db_query("select td_val1 from dh_type_detail where td_type = 'COURSE-SYSTEM-STATUS' and td_key = 'STATUS-ATTENDED'")->fetchField();
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
			}
			$sid = dh_student_apply($sid, $r, $strong, $ctx);

			$c = array('sc_student' => $sid, 'sc_applicant' => (int) $r['a_id'], 'sc_center' => $r['a_center'], 'sc_course' => $r['a_course'],
				'sc_teacher' => isset($teacher[$r['a_gender']][$r['aa_group']]) ? $teacher[$r['a_gender']][$r['aa_group']] : NULL,
				'sc_group' => $r['aa_group'], 'sc_acco' => $r['aa_section'].'-'.$r['aa_acco'], 'sc_laundry' => $r['aa_laundry'], 'sc_valuable' => $r['aa_valuable'],
				'sc_cell' => $r['aa_cell'], 'sc_chowky' => $r['aa_chowky'], 'sc_chair' => $r['aa_chair'], 'sc_seat_row' => $r['aa_seat_row'], 'sc_seat_col' => $r['aa_seat_col'],
				'sc_comment' => $r['aa_comment'], 'sc_desc_physical' => $r['ae_desc_physical'], 'sc_desc_mental' => $r['ae_desc_mental'],
				'sc_addiction_past' => $r['ae_desc_addiction_past'], 'sc_addiction_current' => $r['ae_desc_addiction_current'],
				'sc_desc_other_technique' => $r['ae_desc_other_technique'], 'sc_desc_medication' => $r['ae_desc_medication'], 'sc_teach_other_details' => $r['ae_teach_other_details']);
			$files = dh_course_sync_files($r, $sid, $opts['files'], $sum);
			if ($files['sc_uri']) $c['sc_uri'] = $files['sc_uri'];
			if ($files['s_photo']) db_update('dh_student')->fields(array('s_photo' => $files['s_photo']))->condition('s_id', $sid)->execute();
			dh_finalize_fit('dh_student_course', $c, $r['a_id']);
			if ($sc)
			{
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
			if ($sci_id) db_update('dh_student_course_input')->fields($sci)->condition('sci_id', $sci_id)->execute();
			else db_insert('dh_student_course_input')->fields($sci + array('sci_student' => $sid))->execute();

			if ($opts['mail'] && empty($r['a_attended_mail']))
			{
				$fn = $opts['mail_fn'];
				$ret = $fn('applicant', $r['a_id'], $attended);
				db_update('dh_applicant')->fields(array('a_attended_mail' => date('Y-m-d H:i:s')))->condition('a_id', $r['a_id'])->execute();
				if (empty($ret['error'])) $sum['emails']++;   // a letter that could not go out (no email or mobile) is not counted
			}
		}
		catch (Throwable $t)
		{
			$sum['errors']++;
			watchdog('Finalization Error', "Applicant @a, course @c: @m\n@trace", array('@a' => $r['a_id'], '@c' => $course_id, '@m' => $t->getMessage(), '@trace' => $t->getTraceAsString()));
		}
	}

	if ($sum['errors'])
		watchdog('Finalization', 'Course @c: removals skipped because @n applicant(s) failed.', array('@c' => $course_id, '@n' => $sum['errors']), WATCHDOG_WARNING);
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
			dh_student_remove_if_empty((int) $g['sc_student'], $log, $sum);
		}
	}

	db_delete('dh_course_stat')->condition('cs_course', $course_id)->execute();
	foreach (db_query('select a_gender, a_status, a_old, count(a_id) tot from dh_applicant where a_course = :c group by a_gender, a_status, a_old', array(':c' => $course_id)) as $s)
		db_insert('dh_course_stat')->fields(array('cs_course' => $course_id, 'cs_old' => $s->a_old, 'cs_gender' => $s->a_gender, 'cs_status' => $s->a_status, 'cs_count' => $s->tot))->execute();
	return $sum;
}

/* TRUE, or a message saying why this user cannot reopen this course. */
function dh_course_can_reopen($course_id, $account = NULL)
{
	global $user;
	$account = $account ? $account : $user;
	$c = db_query('select c_finalized, c_finalized_tstamp, c_finalize_requested, c_deleted from dh_course where c_id = :c', array(':c' => $course_id))->fetchAssoc();
	if (!$c || $c['c_deleted'])
		return t('Course not found.');
	if (!$c['c_finalized'])
		return t('This course is not finalized.');
	if ($c['c_finalize_requested'])
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

/* Reopen a finalized course for correction. c_processed is left alone, so nothing goes to dhamma.org. */
function dh_course_reopen($course_id, $reason, $uid)
{
	$now = date('Y-m-d H:i:s');
	db_update('dh_course')->fields(array('c_finalized' => 0, 'c_reopened' => $now, 'c_reopened_by' => $uid, 'c_updated' => $now, 'c_updated_by' => $uid))
		->condition('c_id', $course_id)->execute();
	dh_course_log_note($course_id, 'reopen_reason', $reason, $uid);
}

/* Mark applicants who already got the Attended email, from the letter log. Only fills empty dates.
   Returns how many applicants were marked. */
function dh_fill_attended_mail_dates()
{
	db_query('DROP TEMPORARY TABLE IF EXISTS tmp_att_mail');
	db_query('CREATE TEMPORARY TABLE tmp_att_mail (a_id BIGINT NOT NULL PRIMARY KEY, t DATETIME NOT NULL) ENGINE=InnoDB');
	db_query("INSERT INTO tmp_att_mail (a_id, t) SELECT l_identifier, MIN(l_tstamp) FROM dh_log
		WHERE l_module = 'Applicant' AND l_event = 'Letter' AND l_msg LIKE 'Letter sent%- Attended%' GROUP BY l_identifier");
	$max = (int) db_query('SELECT MAX(a_id) FROM tmp_att_mail')->fetchField();
	$n = 0;
	for ($from = 0; $from <= $max; $from += 50000)
		$n += db_query('UPDATE dh_applicant a JOIN tmp_att_mail x ON x.a_id = a.a_id SET a.a_attended_mail = x.t
			WHERE a.a_attended_mail IS NULL AND x.a_id BETWEEN :f AND :t', array(':f' => $from, ':t' => $from + 49999), array('return' => Database::RETURN_AFFECTED));
	db_query('DROP TEMPORARY TABLE IF EXISTS tmp_att_mail');
	return $n;
}
