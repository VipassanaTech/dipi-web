<?php
/**
 * Course change log viewer.
 *
 * dh_course_log holds one row per changed course field (old -> new, who, when), written by
 * the database triggers dh_course_au / dh_course_ai (deploy/2026-10-08-course-change-log.sql)
 * plus the rows converted from the old text log (crl_dh_log_id set). This file only reads it:
 * labels and readable values for the "Change Log" section on the course page.
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

/* Number of logged changes (rows) for one course - shown in the section heading. */
function dh_course_log_count($course_id)
{
	if (!db_table_exists('dh_course_log'))
		return 0;
	return (int) db_query("select count(*) from dh_course_log where crl_course = :c", array(':c' => $course_id))->fetchField();
}

/**
 * The "Change Log" table for one course: latest first, one line per changed field.
 */
function dh_course_log_viewer($course_id)
{
	if (!db_table_exists('dh_course_log'))
		return '<p>The change log is not set up yet.</p>';
	$res = db_query("select l.*, u.name from dh_course_log l left join users u on u.uid = l.crl_user
		where l.crl_course = :c order by l.crl_tstamp desc, l.crl_id asc limit 1001", array(':c' => $course_id))->fetchAll();
	if (!$res)
		return '<p>No changes recorded yet.</p>';
	$more = count($res) > 1000;
	$res = array_slice($res, 0, 1000);

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
			date('d M Y H:i', strtotime($r->crl_tstamp)),
			$what,
			array('data' => $r->crl_action == 'create' ? '' : _dh_course_log_value($f[1], $r->crl_old, $names), 'class' => array('cl-val')),
			array('data' => _dh_course_log_value($f[1], $r->crl_new, $names), 'class' => array('cl-val')),
			check_plain($r->name ? $r->name : ($r->crl_user ? 'user '.$r->crl_user : 'unknown')),
		);
	}
	$out = theme('table', array('header' => array('When', 'What', 'Old', 'New', 'By'), 'rows' => $rows, 'attributes' => array('class' => array('dh-centre-log', 'dh-course-log'))));
	if ($more)
		$out .= '<p class="cl-note">Showing the latest 1,000 changes.</p>';
	return $out;
}
