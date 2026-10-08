<?php
/**
 * Course change log viewer.
 *
 * dh_course_log holds one row per changed course field (old -> new, who, when), written by
 * the database triggers dh_course_au / dh_course_ai (deploy/2026-10-08-course-change-log.sql)
 * plus the rows converted from the old text log (crl_dh_log_id set). Teacher assignments
 * come from the Course-Teacher lines in dh_log. This file only reads them: labels and
 * readable values for the "Change Log" section on the course page.
 */

/* Field => array(label, kind). Labels follow the course editor; kind picks the formatting. */
function dh_course_log_fields()
{
	return array(
		'c_course_type'          => array('Course Type', 'coursetype'),
		'c_center'               => array('Centre', 'centre'),
		'c_start'                => array('Start Date', 'date'),
		'c_end'                  => array('End Date', 'date'),
		'c_enrol_date'           => array('Enrollment Date', 'date'),
		'c_cancelled'            => array('Cancelled', 'yesno'),
		'c_status'               => array('Course Status', 'text'),
		'c_status_nm'            => array('Status New Male', 'text'),
		'c_status_om'            => array('Status Old Male', 'text'),
		'c_status_nf'            => array('Status New Female', 'text'),
		'c_status_of'            => array('Status Old Female', 'text'),
		'c_status_svr_m'         => array('Status Male Server', 'text'),
		'c_status_svr_f'         => array('Status Female Server', 'text'),
		'c_list_only'            => array('List Only', 'yesno'),
		'c_date_change'          => array('Date Change', 'yesno'),
		'c_comments'             => array('Comments', 'longtext'),
		'c_description'          => array('Course Title', 'text'),
		'c_at_m_count'           => array('Male Teacher Count', 'text'),
		'c_at_f_count'           => array('Female Teacher Count', 'text'),
		'c_at_m_conf'            => array('Male Teachers Confirmed', 'text'),
		'c_at_f_conf'            => array('Female Teachers Confirmed', 'text'),
		'c_combined_seat_course' => array('Combine Seating with Course', 'course'),
		'c_form_langs'           => array('Course Form Langs', 'text'),
		'c_deleted'              => array('Deleted', 'yesno'),
		'c_finalized'            => array('Finalized', 'yesno'),
		'c_auto_confirm'         => array('Auto Confirm', 'yesno'),
		'c_cell_plan'            => array('Cell Plan', 'plan'),
		'c_dining_plan'          => array('Dining Plan', 'plan'),
		'c_seat_plan'            => array('Seating Plan', 'plan'),
		'c_dashboard_pin'        => array('Pinned on Dashboard', 'yesno'),
	);
}

/* One stored value as readable HTML ($names: id => name lookups for type / centre / course). */
function _dh_course_log_value($kind, $value, $names)
{
	if ($value === NULL || $value === '')
		return '<span class="cl-empty">(empty)</span>';
	switch ($kind)
	{
		case 'yesno':
			return ((string) $value === '1') ? 'Yes' : (((string) $value === '0') ? 'No' : check_plain($value));
		case 'plan':
			return ((string) $value === '1') ? 'Group' : 'Main';
		case 'date':
			$t = strtotime($value);
			return ($t && substr($value, 0, 4) !== '0000') ? date('d M Y', $t) : check_plain($value);
		case 'coursetype':
		case 'centre':
		case 'course':
			if ((string) $value === '0' && $kind === 'course')
				return 'None';
			return isset($names[$kind][$value]) ? check_plain($names[$kind][$value]) : check_plain($value);
		case 'longtext':
			return nl2br(check_plain($value));
		default:
			return check_plain($value);
	}
}

/**
 * Course setting rows of the Change Log, latest first, as display rows. The "Teachers
 * Confirmed" counts are left out: they only follow the teacher assignments, which are
 * shown with names by _dh_course_teacher_log_rows().
 */
function _dh_course_setting_log_rows($course_id)
{
	if (!db_table_exists('dh_course_log'))
		return array();
	$res = db_query("select l.*, u.name from dh_course_log l left join users u on u.uid = l.crl_user
		where l.crl_course = :c and l.crl_field not in ('c_at_m_conf', 'c_at_f_conf')
		order by l.crl_tstamp desc, l.crl_id asc limit 1001", array(':c' => $course_id))->fetchAll();

	$fields = dh_course_log_fields();
	// Names for ids shown in the log: course types, centres, combined courses.
	$ids = array('coursetype' => array(), 'centre' => array(), 'course' => array());
	foreach ($res as $r)
		if (isset($fields[$r->crl_field]) && isset($ids[$fields[$r->crl_field][1]]))
			foreach (array($r->crl_old, $r->crl_new) as $v)
				if ($v !== NULL && $v !== '' && ctype_digit((string) $v))
					$ids[$fields[$r->crl_field][1]][$v] = (int) $v;
	$names = array('coursetype' => array(), 'centre' => array(), 'course' => array());
	if ($ids['coursetype'])
		$names['coursetype'] = db_query("select td_id, td_val1 from dh_type_detail where td_id in (:ids)", array(':ids' => array_values($ids['coursetype'])))->fetchAllKeyed();
	if ($ids['centre'])
		$names['centre'] = db_query("select c_id, c_name from dh_center where c_id in (:ids)", array(':ids' => array_values($ids['centre'])))->fetchAllKeyed();
	if ($ids['course'])
		$names['course'] = db_query("select c_id, c_name from dh_course where c_id in (:ids)", array(':ids' => array_values($ids['course'])))->fetchAllKeyed();

	$actions = array('create' => 'Created', 'update' => '', 'delete' => 'Deleted', 'restore' => 'Restored');
	$rows = array();
	foreach ($res as $r)
	{
		$f = isset($fields[$r->crl_field]) ? $fields[$r->crl_field] : array($r->crl_field, 'text');
		$what = check_plain($f[0]);
		if (!empty($actions[$r->crl_action]))
			$what = '<span class="cl-act">'.$actions[$r->crl_action].'</span> &middot; '.$what;
		$rows[] = array(
			'ts' => $r->crl_tstamp,
			'ord' => (int) $r->crl_id,
			'what' => $what,
			'old' => $r->crl_action == 'create' ? '' : _dh_course_log_value($f[1], $r->crl_old, $names),
			'new' => _dh_course_log_value($f[1], $r->crl_new, $names),
			'by' => check_plain($r->name ? $r->name : ($r->crl_user ? 'user '.$r->crl_user : 'unknown')),
		);
	}
	return $rows;
}

/**
 * Teacher assignment rows of the Change Log ("<name> Confirmed by <user>"), read from the
 * Course-Teacher lines in dh_log (log_course_teacher) and the Course-Teacher-Del lines
 * of removed trainees (del_trainee_teacher). Those lines are free text, one per
 * event; this turns them into Old -> New rows. The old status is the one the previous
 * line for the same assignment left. Email notices and Mailgun failure notes are skipped.
 */
function _dh_course_teacher_log_rows($course_id)
{
	$cts = db_query("select ct.ct_id, ct.ct_type, t.t_f_name, t.t_l_name, t.t_gender from dh_course_teacher ct
		left join dh_teacher t on t.t_id = ct.ct_teacher where ct.ct_course = :c", array(':c' => $course_id))->fetchAllAssoc('ct_id');
	// Removed trainees: their row is gone; the removal line (logged under the course id by
	// del_trainee_teacher) gives the assignment and teacher ids.
	$removed = db_query("select l.l_id, l.l_identifier, l.l_msg, l.l_tstamp, l.l_user, u.name from dh_log l
		left join users u on u.uid = l.l_user
		where l.l_module = 'Course-Teacher-Del' and l.l_identifier = :c", array(':c' => $course_id))->fetchAll();
	foreach ($removed as $k => $l)
	{
		if (!preg_match('/\(assignment (\d+), teacher (\d+)\)$/', $l->l_msg, $m))
		{
			unset($removed[$k]);
			continue;
		}
		$removed[$k]->l_identifier = $m[1];
		if (!isset($cts[$m[1]]))
		{
			$t = db_query("select t_f_name, t_l_name, t_gender from dh_teacher where t_id = :t", array(':t' => $m[2]))->fetchObject();
			$cts[$m[1]] = (object) array('ct_id' => $m[1], 'ct_type' => 'Training',
				't_f_name' => $t ? $t->t_f_name : '', 't_l_name' => $t ? $t->t_l_name : '', 't_gender' => $t ? $t->t_gender : '');
		}
	}
	if (!$cts)
		return array();

	$lines = db_query("select l.l_id, l.l_identifier, l.l_msg, l.l_tstamp, l.l_user, u.name from dh_log l
		left join users u on u.uid = l.l_user
		where l.l_module = 'Course-Teacher' and l.l_identifier in (:ids)",
		array(':ids' => array_keys($cts)))->fetchAll();
	$lines = array_merge($lines, $removed);
	usort($lines, function ($a, $b) { return $a->l_id - $b->l_id; });
	// Changes made through the AT app are logged under the app's API account.
	$app_uids = array_flip(db_query("select ur.uid from users_roles ur join role r on r.rid = ur.rid where r.name = 'APP API'")->fetchCol());

	$types = array('Training' => 'Trainee');
	$genders = array('M' => 'Male', 'F' => 'Female');
	$status = array();
	$rows = array();
	foreach ($lines as $l)
	{
		$ct = $cts[$l->l_identifier];
		$prev = isset($status[$ct->ct_id]) ? $status[$ct->ct_id] : NULL;
		$text = trim(strip_tags(preg_replace('/<br\s*\/?>/i', "\n", $l->l_msg)));
		$comment = '';
		if (preg_match('/\nComments:(.*)$/s', $text, $m))
		{
			$comment = trim($m[1]);
			$text = trim(substr($text, 0, -strlen($m[0])));
		}
		$old = $prev;
		if (preg_match('/^Application Received/', $text) || preg_match('/^ReApplying/', $text))
			list($event, $new) = array('Applied', 'Received');
		elseif (preg_match('/^(Center added (AT Application|Trainee)|Added application) as (\w+)/', $text, $m))
			list($event, $old, $new) = array('Added', NULL, $m[3]);
		elseif (preg_match('/^Status updated to (\w+)/', $text, $m))
			list($event, $new) = array('Status', $m[1]);
		elseif (preg_match('/^Type changed from (.*) to (.*)$/', $text, $m))
			list($event, $old, $new) = array('Type', $m[1], $m[2]);
		elseif (preg_match('/^Group changed from (.*) to (.*)$/', $text, $m))
			list($event, $old, $new) = array('Group', $m[1], $m[2]);
		elseif (preg_match('/^Trainee removed:/', $text))
			list($event, $new) = array('Removed', '(removed)');
		elseif (strpos($text, 'Email Notification sent') !== FALSE || !$l->l_user)
			continue;   // status emails, and Mailgun delivery failures (logged by the webhook, no user)
		else
			list($event, $old, $new) = array('Note', NULL, $text);
		if (in_array($event, array('Applied', 'Added', 'Status')))
			$status[$ct->ct_id] = $new;

		$name = trim($ct->t_f_name.' '.$ct->t_l_name);
		$about = array();
		if (isset($genders[$ct->t_gender]))
			$about[] = $genders[$ct->t_gender];
		if ($ct->ct_type)
			$about[] = isset($types[$ct->ct_type]) ? $types[$ct->ct_type] : $ct->ct_type;
		$what = '<span class="cl-act">Teacher '.$event.'</span> &middot; '.check_plain($name !== '' ? $name : 'assignment '.$ct->ct_id)
			.($about ? ' <span class="cl-sub">('.check_plain(implode(', ', $about)).')</span>' : '');
		$rows[] = array(
			'ts' => $l->l_tstamp,
			'ord' => (int) $l->l_id,
			'what' => $what,
			'old' => $old === NULL ? '' : check_plain($old),
			'new' => check_plain($new).($comment !== '' ? '<div class="cl-cmt">'.nl2br(check_plain($comment)).'</div>' : ''),
			'by' => isset($app_uids[$l->l_user]) ? 'AT app' : check_plain($l->name ? $l->name : ($l->l_user ? 'user '.$l->l_user : 'unknown')),
		);
	}
	return $rows;
}

/**
 * The "Change Log" section on the course page (a closed panel with the Centre Settings +/-),
 * or '' for a user who may see none of it. Course setting changes need 'access centre
 * settings'; teacher assignments need 'at scheduling' (the Assign Teachers page).
 */
function dh_course_log_section($course_id)
{
	$see_course = user_access('access centre settings');
	$see_teachers = user_access('at scheduling');
	if (!$see_course && !$see_teachers)
		return '';
	$rows = array_merge($see_course ? _dh_course_setting_log_rows($course_id) : array(),
		$see_teachers ? _dh_course_teacher_log_rows($course_id) : array());
	// Latest first; lines from the same moment keep the order they were written in.
	usort($rows, function ($a, $b) { return ($a['ts'] == $b['ts']) ? $a['ord'] - $b['ord'] : strcmp($b['ts'], $a['ts']); });
	$more = count($rows) > 1000;
	$rows = array_slice($rows, 0, 1000);

	if (!$rows)
		$html = '<p>No changes recorded yet.</p>';
	else
	{
		$table = array();
		foreach ($rows as $r)
			$table[] = array(
				date('d M Y H:i', strtotime($r['ts'])),
				$r['what'],
				array('data' => $r['old'], 'class' => array('cl-val')),
				array('data' => $r['new'], 'class' => array('cl-val')),
				$r['by'],
			);
		$html = theme('table', array('header' => array('When', 'What', 'Old', 'New', 'By'), 'rows' => $table, 'attributes' => array('class' => array('dh-centre-log', 'dh-course-log'))));
		if ($more)
			$html .= '<p class="cl-note">Showing the latest 1,000 changes.</p>';
	}
	$n = count($rows);
	$log = array('#type' => 'fieldset', '#id' => 'course-change-log',
		'#title' => 'Change Log ('.($more ? '1,000+' : $n).' '.($n == 1 ? 'change' : 'changes').')',
		'#collapsible' => TRUE, '#collapsed' => TRUE,
		'log' => array('#markup' => $html));
	return drupal_render($log);
}
