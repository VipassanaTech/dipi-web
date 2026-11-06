<?php
/**
 * Seating plan tools: the shared re-seat step for a grid or group change (used
 * by the Zero-Day popup and the seating plans), and the backrest / grid save
 * from the plans.
 */

/**
 * Every course that shares one seating plan with $course_id: the course, the
 * course it combines its seating with, and any course combined with it.
 * Returns sorted int ids.
 */
function dh_seat_plan_courses($course_id)
{
	$course_id = (int) $course_id;
	$ids = db_query("select c_id from dh_course where c_id = :c1 or c_combined_seat_course = :c2
		or c_id = (select c_combined_seat_course from dh_course where c_id = :c3)",
		array(':c1' => $course_id, ':c2' => $course_id, ':c3' => $course_id))->fetchCol();
	$ids = array_values(array_unique(array_map('intval', $ids)));
	sort($ids);
	return $ids;
}

/**
 * Seat fields for a student who moves to the other grid (main <-> chowky/chair)
 * or to another group: the first seat of a new row at the top (back) of the new
 * grid, the place the Zero-Day popup has always used. Every course that shares
 * the plan is counted, so the seat is never one already used by a student of a
 * combined course; an empty grid starts at the front row (row 0).
 *
 * $app_id      the student (their own seat is never counted)
 * $course_id   the student's course (a_course)
 * $gender      'M' or 'F'
 * $now_chowky  TRUE when the student is now in the chowky/chair grid
 * $main        TRUE: a new seat in the seating plan (aa_seat_row / aa_seat_col)
 * $group       NULL: leave the group-wise plan alone; else the student's (new)
 *              group: a new seat in that group's grid (aa_group_seat_row / col)
 * Returns the fields to save (empty when nothing is asked).
 */
function dh_seat_reseat_fields($app_id, $course_id, $gender, $now_chowky, $main, $group)
{
	$fields = array();
	$args = array(':c' => dh_seat_plan_courses($course_id), ':g' => $gender, ':self' => (int) $app_id);
	$grid = $now_chowky ? " and (aa.aa_chowky = 1 or aa.aa_chair = 1)" : " and aa.aa_chowky = 0 and aa.aa_chair = 0";
	$from = " from dh_applicant_attended aa join dh_applicant a on a.a_id = aa.aa_applicant
		where a.a_course in (:c) and a.a_gender = :g and a.a_type = 'Student' and a.a_attended = 1
		and a.a_id <> :self".$grid;
	if ($main)
	{
		$max = db_query("select max(aa.aa_seat_row)".$from, $args)->fetchField();
		$fields['aa_seat_row'] = ($max === NULL || $max === FALSE) ? 0 : (int) $max + 1;
		$fields['aa_seat_col'] = 0;
	}
	if ($group !== NULL)
	{
		$max = db_query("select max(aa.aa_group_seat_row)".$from." and aa.aa_group = :grp", $args + array(':grp' => $group))->fetchField();
		$fields['aa_group_seat_row'] = ($max === NULL || $max === FALSE) ? 0 : (int) $max + 1;
		$fields['aa_group_seat_col'] = 0;
	}
	return $fields;
}

/**
 * Backrest / chowky / chair change for one student, from a seating plan.
 * $set: 'none' | 'backrest' (main grid) or 'chowky' | 'chair' (chowky grid).
 * $from_grid: 'main' | 'chowky', the grid the page shows the student in; if the
 * server has the student in the other grid, someone changed them meanwhile.
 * A change of grid also gives the student a new seat in both plans (the same
 * step as the Zero-Day popup). Same lock as the Zero-Day save.
 * Returns array('status' => 'OK'|'Failed', 'msg' => ..., 'reload' => bool).
 */
function dh_seat_special_apply($course, $app_id, $set, $from_grid, $uid)
{
	$out = array('status' => 'Failed', 'msg' => '', 'reload' => FALSE);
	$sets = array(
		'none'     => array('aa_chowky' => 0, 'aa_chair' => 0, 'aa_backrest' => 0),
		'backrest' => array('aa_chowky' => 0, 'aa_chair' => 0, 'aa_backrest' => 1),
		'chowky'   => array('aa_chowky' => 1, 'aa_chair' => 0, 'aa_backrest' => 0),
		'chair'    => array('aa_chowky' => 0, 'aa_chair' => 1, 'aa_backrest' => 0),
	);
	if (!isset($sets[$set]) || !in_array($from_grid, array('main', 'chowky'), TRUE) || (int) $app_id <= 0)
	{
		$out['msg'] = 'Invalid value.';
		return $out;
	}
	if (!dh_manageapp_lock_acquire('dh_app_update_attended', 10.0, 10))
	{
		$out['msg'] = 'Unable to get lock. Please try again.';
		return $out;
	}
	try
	{
		$r = db_query("select a.a_id, a.a_course, a.a_gender, aa.aa_chowky, aa.aa_chair, aa.aa_group
			from dh_applicant a join dh_applicant_attended aa on aa.aa_applicant = a.a_id
			where a.a_id = :a and a.a_course in (:c) and a.a_attended = 1 and a.a_type = 'Student'",
			array(':a' => (int) $app_id, ':c' => dh_seat_plan_courses($course)))->fetchAssoc();
		if (!$r)
			$out['msg'] = 'Student not found in this course.';
		else
		{
			$was = ($r['aa_chowky'] || $r['aa_chair']) ? 'chowky' : 'main';
			$now = ($set == 'chowky' || $set == 'chair') ? 'chowky' : 'main';
			if ($was !== $from_grid)
				$out['msg'] = 'This student was changed by someone else. Please reload the page.';
			else
			{
				$fields = $sets[$set] + array('aa_updated_by' => (int) $uid, 'aa_updated' => date('Y-m-d H:i:s'));
				if ($now !== $was)
					$fields = array_merge($fields, dh_seat_reseat_fields($r['a_id'], $r['a_course'], $r['a_gender'], $now == 'chowky', TRUE, $r['aa_group']));
				db_update('dh_applicant_attended')->fields($fields)->condition('aa_applicant', (int) $r['a_id'])->execute();
				$out = array('status' => 'OK', 'msg' => '', 'reload' => ($now !== $was));
			}
		}
	}
	finally
	{
		lock_release('dh_app_update_attended');
	}
	return $out;
}

/**
 * Save handler for the student menu on the seating plans (route
 * seat-special/%centre_id/%course_id, POST, JSON). The loaders have already
 * checked the user's centre access; the token stops another site posting here.
 */
function dh_seat_special_update($centre, $course)
{
	global $user;
	if (!isset($_POST['token']) || !drupal_valid_token($_POST['token'], 'seat-special'))
		$out = array('status' => 'Failed', 'msg' => 'Session expired. Reload the page and try again.', 'reload' => FALSE);
	else
		$out = dh_seat_special_apply($course,
			isset($_POST['a']) ? (int) $_POST['a'] : 0,
			isset($_POST['set']) ? (string) $_POST['set'] : '',
			isset($_POST['grid']) ? (string) $_POST['grid'] : '',
			$user->uid);
	drupal_json_output($out);
	drupal_exit();
}

/**
 * New seat fields for a stored student whose special seating or group is being
 * saved, shared by the Zero-Day popup and the application edit form. A move to
 * the other grid (main <-> chowky/chair) gives a new seat in both plans; a new
 * group alone gives a new seat in the group-wise plan; otherwise nothing.
 * $prev: the stored row (aa_chowky, aa_chair, aa_group).
 */
function dh_seat_moved_fields($app_id, $course_id, $gender, $prev, $now_chowky, $new_group)
{
	$was_chowky = ($prev['aa_chowky'] || $prev['aa_chair']) ? TRUE : FALSE;
	$grid_changed = ($was_chowky != (bool) $now_chowky);
	$group_changed = ($prev['aa_group'] != $new_group);
	return dh_seat_reseat_fields($app_id, $course_id, $gender, $now_chowky, $grid_changed,
		($grid_changed || $group_changed) ? $new_group : NULL);
}

/**
 * The two buttons for each section's green bar on the seating plans:
 * "Swap / Move seats" (js/seat-tools.js) and "Store Seat Changes" (the plan's
 * own save, the same as the button at the top of the page).
 */
function dh_seat_section_tools_html()
{
	return ' <button type="button" class="dh-swap-toggle" title="Click a student, then a seat: the two swap, or the student moves into an empty seat">&#8644; Swap / Move seats</button>'
		.' <button type="button" class="store-seat-changes dh-sec-store">Store Seat Changes</button>'
		.' <span class="dh-swap-status"></span>';
}

/**
 * Script tags for js/seat-tools.js and its settings (save URL and token for the
 * student menu). The plans print their own HTML, not the Drupal page, so the
 * caller appends this; '' after the first call.
 */
function dh_seat_tools_js($centre, $course)
{
	$added = &drupal_static(__FUNCTION__);
	if ($added)
		return '';
	$added = TRUE;
	$path = drupal_get_path('module', 'dh_manageapp').'/js/seat-tools.js';
	return '<script>window.dhSeatToolsCfg = '.drupal_json_encode(array(
			'url' => url('seat-special/'.(int) $centre.'/'.(int) $course),
			'token' => drupal_get_token('seat-special'),
		)).';</script>'
		.'<script src="'.$GLOBALS['base_url'].'/'.$path.'?v='.(file_exists($path) ? filemtime($path) : 0).'"></script>';
}

/**
 * A plan section id as the plans print it: main plan 'M' / 'F'; group plan
 * 'g<group>_<M|F>' ('gUnassigned_M' for students with no group).
 * Returns array(gender, group or NULL, label), or FALSE for anything else.
 */
function dh_seat_regen_parse($plan, $sec)
{
	$names = array('M' => 'Male', 'F' => 'Female');
	$sec = (string) $sec;
	if ($plan == 'main')
		return isset($names[$sec]) ? array($sec, NULL, $names[$sec]) : FALSE;
	if ($plan == 'group' && preg_match('/^g(.+)_([MF])$/', $sec, $m) && trim($m[1]) !== '')
		return array($m[2], $m[1], 'Group '.$m[1].' - '.$names[$m[2]]);
	return FALSE;
}

/** The page anchor of a section (the id on its .seat-section div). */
function dh_seat_sec_anchor($sec)
{
	return 'dh-sec-'.preg_replace('/[^A-Za-z0-9_-]/', '-', (string) $sec);
}

/**
 * Re-pack one section of a plan from the configuration: its main grid and its
 * chowky grid, in the same order as the full Regenerate. Every other section
 * keeps its seats, and the other plan is not touched.
 * $course: the course id, or the ids of the courses sharing the plan.
 * Returns the section label, or FALSE for an unknown section.
 */
function dh_seat_regen_section($centre, $course, $plan, $sec)
{
	$p = dh_seat_regen_parse($plan, $sec);
	if ($p === FALSE)
		return FALSE;
	list($gender, $group, $label) = $p;
	if (!is_array($course))
		$course = array($course);
	if ($plan == 'main')
	{
		dh_auto_allocate_seats($centre, $course, $gender);
		return $label;
	}
	// Group plan: clear this section's group seats, then fill in the students
	// without one - the same step the page runs on every view.
	$args = array(':c' => $course, ':gen' => $gender);
	if ($group === 'Unassigned')
		$in_group = "trim(ifnull(aa.aa_group, '')) = ''";
	else
	{
		$in_group = 'trim(aa.aa_group) = :g';
		$args[':g'] = $group;
	}
	db_query("update dh_applicant_attended aa join dh_applicant a on a.a_id = aa.aa_applicant
		set aa.aa_group_seat_row = NULL, aa.aa_group_seat_col = NULL
		where a.a_course in (:c) and a.a_attended = 1 and a.a_type = 'Student' and upper(a.a_gender) = :gen and $in_group", $args);
	dh_auto_allocate_group_seats($centre, $course, FALSE);
	return $label;
}

/**
 * A plan page opened with ?r=1: regenerate the whole plan, or one section
 * (?sec=..., only with the section link's token). Returns where to go next,
 * array(path, options): the plain plan address, so a browser refresh does not
 * regenerate again, with a note for the page ('done' / 'failed').
 * $plan: 'main' or 'group'. $get: the request's query values.
 */
function dh_seat_regen_run($centre, $course_id, $course, $plan, $get)
{
	$path = ($plan == 'group' ? 'group-seating/' : 'seating/').(int) $centre.'/'.(int) $course_id;
	$sec = isset($get['sec']) ? (string) $get['sec'] : '';
	if ($sec === '')
	{
		if ($plan == 'group')
			dh_auto_allocate_group_seats($centre, $course, TRUE);
		else
			dh_auto_allocate_seats($centre, $course);
		return array($path, array('query' => array('done' => 'all')));
	}
	$ok = isset($get['token']) && drupal_valid_token((string) $get['token'], 'seat-regen')
		&& dh_seat_regen_section($centre, $course, $plan, $sec) !== FALSE;
	return array($path, array('query' => array(($ok ? 'done' : 'failed') => $sec), 'fragment' => dh_seat_sec_anchor($sec)));
}

/**
 * "Regenerate this section" link for a section's tool bar. It asks first, and
 * warns about seat changes not yet stored.
 */
function dh_seat_regen_link($plan, $centre, $course_id, $sec, $label)
{
	$path = ($plan == 'group' ? 'group-seating/' : 'seating/').(int) $centre.'/'.(int) $course_id;
	$msg = 'Regenerate will re-pack all seats of '.$label.' and DESTROY its current saved seats. Other sections are not changed.';
	$js = 'var n = window.dhSeatPendingCount ? window.dhSeatPendingCount() : 0;'
		.' return confirm('.drupal_json_encode($msg)
		.' + (n > 0 ? "\n\nYou have " + n + (n == 1 ? " unsaved seat change" : " unsaved seat changes") + " on this page; they will be lost." : "")'
		.' + "\n\nContinue?");';
	return ' '.l('&#8635; Regenerate this section', $path, array('html' => TRUE,
		'query' => array('r' => 1, 'sec' => $sec, 'token' => drupal_get_token('seat-regen')),
		'attributes' => array('class' => array('dh-sec-regen'), 'title' => 'Re-pack only this section from the Seating Configuration', 'onclick' => $js)));
}

/**
 * The note after a regenerate, for one section ($sec) or the whole plan ('all'):
 * '' unless this page was opened by that regenerate. The first note also drops
 * the note from the address bar.
 */
function dh_seat_regen_notice($sec)
{
	$sec = (string) $sec;
	if (isset($_GET['done']) && (string) $_GET['done'] === $sec)
		$note = ($sec === 'all') ? '&#10003; All sections regenerated.' : '&#10003; Regenerated';
	elseif (isset($_GET['failed']) && (string) $_GET['failed'] === $sec && $sec !== 'all')
		$note = 'This section was not regenerated (the link was old). Please use its Regenerate link again.';
	else
		return '';
	$class = isset($_GET['done']) ? 'dh-regen-done' : 'dh-regen-failed';
	$out = ' <span class="'.$class.' no-print">'.$note.'</span>';
	$once = &drupal_static(__FUNCTION__);
	if (!$once)
	{
		$once = TRUE;
		$out .= '<script>if (window.history && history.replaceState) history.replaceState(null, "", location.pathname + location.hash);</script>';
	}
	return $out;
}
