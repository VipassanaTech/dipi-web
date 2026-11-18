<?php
/* Referral person details live on the referral (r_* columns); the form keeps its old field names (s_*, sci_*).
   Nothing in this file writes dh_student or dh_student_course_input. r_student is a read-only link to the person's
   real student record (0 = none). Design: "Referral Person Details: Keep on the Referral, Link to the Student". */

// Form field => dh_referral column: the 23 person fields and the 10 course counts.
function dh_referral_person_map()
{
	return array(
		's_f_name' => 'r_f_name', 's_m_name' => 'r_m_name', 's_l_name' => 'r_l_name', 's_gender' => 'r_gender', 's_dob' => 'r_dob',
		's_email' => 'r_email', 's_phone_home' => 'r_phone_home', 's_phone_mobile' => 'r_phone_mobile', 's_phone_office' => 'r_phone_office',
		's_address' => 'r_address', 's_city' => 'r_city', 's_state' => 'r_state', 's_country' => 'r_country', 's_zip' => 'r_zip',
		's_education' => 'r_education', 's_occupation' => 'r_occupation', 's_company' => 'r_company', 's_department' => 'r_department',
		's_designation' => 'r_designation', 's_aadhar' => 'r_aadhar', 's_pancard' => 'r_pancard', 's_voter_id' => 'r_voter_id', 's_passport' => 'r_passport',
		'sci_teen' => 'r_cnt_teen', 'sci_10d' => 'r_cnt_10d', 'sci_stp' => 'r_cnt_stp', 'sci_spl' => 'r_cnt_spl', 'sci_20d' => 'r_cnt_20d',
		'sci_30d' => 'r_cnt_30d', 'sci_45d' => 'r_cnt_45d', 'sci_60d' => 'r_cnt_60d', 'sci_tsc' => 'r_cnt_tsc', 'sci_service' => 'r_cnt_service');
}

// Every column the change log covers, with the label people see.
function dh_referral_log_labels()
{
	return array(
		'r_f_name' => 'First Name', 'r_m_name' => 'Middle Name', 'r_l_name' => 'Last Name', 'r_gender' => 'Gender', 'r_dob' => 'Date of Birth',
		'r_email' => 'Email', 'r_phone_home' => 'Phone (Home)', 'r_phone_mobile' => 'Mobile', 'r_phone_office' => 'Phone (Office)',
		'r_address' => 'Address', 'r_city' => 'City', 'r_state' => 'State', 'r_country' => 'Country', 'r_zip' => 'Pin Code',
		'r_education' => 'Education', 'r_occupation' => 'Occupation', 'r_company' => 'Company', 'r_department' => 'Department',
		'r_designation' => 'Designation', 'r_aadhar' => 'Aadhar', 'r_pancard' => 'Pancard', 'r_voter_id' => 'National ID', 'r_passport' => 'Passport',
		'r_cnt_teen' => 'Teen courses', 'r_cnt_10d' => '10-Day courses', 'r_cnt_stp' => 'STP courses', 'r_cnt_spl' => '10D-SPL courses',
		'r_cnt_20d' => '20-Day courses', 'r_cnt_30d' => '30-Day courses', 'r_cnt_45d' => '45-Day courses', 'r_cnt_60d' => '60-Day courses',
		'r_cnt_tsc' => 'TSC courses', 'r_cnt_service' => 'Seva given',
		'r_listed_10d' => 'Listed for 10-Day', 'r_listed_stp' => 'Listed for STP', 'r_listed_spl' => 'Listed for 10D-SPL', 'r_listed_20d' => 'Listed for 20-Day',
		'r_listed_30d' => 'Listed for 30-Day', 'r_listed_45d' => 'Listed for 45-Day', 'r_listed_60d' => 'Listed for 60-Day', 'r_listed_tsc' => 'Listed for TSC',
		'r_listed_10d_seva' => 'Listed for 10-Day Seva', 'r_listed_lc_seva' => 'Listed for LC Seva', 'r_listed_longseva' => 'Listed for Long-term Seva',
		'r_special' => 'Special List', 'r_start' => 'Start Date', 'r_end' => 'End Date', 'r_reason' => 'Referral Reason/Remarks', 'r_listed_by' => 'Referred By',
		'r_teacher' => 'Referring AT', 'r_course_center' => 'Referral Center', 'r_course_type' => 'Referral Course Type', 'r_course_start' => 'Referral Course Start');
}

// The referral row with the form's keys added (s_* and sci_* from r_*), for dh_referral_form().
function dh_referral_form_row(array $ref)
{
	foreach (dh_referral_person_map() as $f => $c)
		$ref[$f] = isset($ref[$c]) ? $ref[$c] : NULL;
	return $ref;
}

// The referral's person in the shape dh_student_find() reads (an applicant row's a_* keys). NULL becomes ''.
function dh_referral_who_row(array $ref)
{
	$out = array();
	foreach (array('f_name', 'l_name', 'dob', 'email', 'phone_mobile', 'aadhar', 'pancard', 'voter_id', 'passport') as $k)
		$out["a_$k"] = isset($ref["r_$k"]) ? trim((string) $ref["r_$k"]) : '';
	return $out;
}

// How a value shows in a log line.
function dh_referral_log_value($col, $v)
{
	$v = trim((string) $v);
	if (strpos($col, 'r_listed_') === 0 && $col !== 'r_listed_by' || $col === 'r_special')
		return $v ? 'Yes' : 'No';
	if ($v === '')
		return '(empty)';
	if ($col === 'r_city')
		return (string) db_query('select c_name from dh_city where c_id = :c', array(':c' => $v))->fetchField();
	if ($col === 'r_teacher')
		return (string) db_query("select concat(t_f_name, ' ', t_l_name, ' (', t_code, ')') from dh_teacher where t_id = :t", array(':t' => $v))->fetchField();
	if ($col === 'r_course_type')
		return (string) db_query('select td_val1 from dh_type_detail where td_id = :t', array(':t' => $v))->fetchField();
	if (strpos($col, 'r_cnt_') === 0)
		return $v;
	return "'$v'";
}

// One 'Edited' line per logged column whose value changed (NULL and '' are the same; compared as text). Returns the line count.
function dh_referral_log_changes($ref_id, array $old, array $new)
{
	$n = 0;
	foreach (dh_referral_log_labels() as $col => $label)
	{
		if (!array_key_exists($col, $new))
			continue;
		$o = isset($old[$col]) ? trim((string) $old[$col]) : '';
		$v = trim((string) $new[$col]);
		if ($o === $v)
			continue;
		add_referral_activity($ref_id, 'Edited', $label.': '.dh_referral_log_value($col, $o).' -> '.dh_referral_log_value($col, $v));
		$n++;
	}
	return $n;
}

// Set the referral's link to a student record (0 = none). Writes one 'Link' log line when it changes.
function dh_referral_set_link($ref_id, $sid, $how)
{
	$cur = (int) db_query('select r_student from dh_referral where r_id = :r', array(':r' => $ref_id))->fetchField();
	$sid = (int) $sid;
	if ($sid === $cur)
		return FALSE;
	db_update('dh_referral')->fields(array('r_student' => $sid))->condition('r_id', $ref_id)->execute();
	add_referral_activity($ref_id, 'Link', $cur
		? ($sid ? "Link switched from student $cur to student $sid ($how)" : "Link to student $cur removed ($how)")
		: "Linked to student $sid ($how)");
	return TRUE;
}

// May the logged-in user edit this referral (and so read its history)? The edit routes' permissions plus the referral's scope:
// main list = 'manage referral list'; centre = 'manage center referral list' and the user's own centre; AT Portal = the AT is its teacher.
function dh_referral_can_edit($ref_id)
{
	global $user;
	$r = db_query('select r_center, r_teacher from dh_referral where r_id = :r', array(':r' => (int) $ref_id))->fetchAssoc();
	if (!$r)
		return FALSE;
	if ((int) $r['r_center'] === 0)
	{
		if (user_access('manage referral list'))
			return TRUE;
		$at = user_access('add referral list') ? _dh_referral_at_teacher_id() : FALSE;
		return $at && (int) $r['r_teacher'] === (int) $at;
	}
	if (!user_access('manage center referral list'))
		return FALSE;
	if (user_access('access all centres'))
		return TRUE;
	return (bool) db_query('select 1 from dh_user_center where uc_user = :u and uc_center = :c and uc_deleted = 0',
		array(':u' => (int) $user->uid, ':c' => (int) $r['r_center']))->fetchField();
}

// "Student record found: 3 courses in DIPI, last: <course>, 05 Mar 2025", or '' when there is no link.
function dh_referral_link_summary($sid)
{
	if (!$sid)
		return '';
	$n = (int) db_query('select count(*) from dh_student_course where sc_student = :s', array(':s' => $sid))->fetchField();
	if (!$n)
		return t('Student record found: no courses in DIPI yet');
	$last = db_query('select c.c_name, c.c_start from dh_student_course sc join dh_course c on c.c_id = sc.sc_course
		where sc.sc_student = :s order by c.c_start desc, c.c_id desc limit 1', array(':s' => $sid))->fetchAssoc();
	return t('Student record found: @n in DIPI, last: @c, @d', array('@n' => format_plural($n, '1 course', '@count courses'),
		'@c' => $last['c_name'], '@d' => date('d M Y', strtotime($last['c_start']))));
}

// The person's real student record, found with finalize's own rule (dh_student_find), or 0.
// Open referral copies are never found (see dh_student_find's $exclude).
function dh_referral_link_find(array $a, $exclude = NULL)
{
	$a += array('a_f_name' => '', 'a_l_name' => '');
	$m = dh_student_find($a, $exclude);
	return (int) $m['sid'];
}
