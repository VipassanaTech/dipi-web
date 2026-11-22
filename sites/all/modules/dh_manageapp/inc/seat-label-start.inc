<?php
/**
 * Seat label start (22 Nov 2026). For one course, each grid (main / chowky) of
 * each section of the seating plans can start its labels later: style 1 from a
 * letter (I1, J1 ...), style 2 from a column number (A7, A8 ...), the numbered
 * style from a first seat number (101, 102 ...). Kept in dh_seat_label_start as
 * an offset (0 = the normal start). Design: private Claude Doc
 * "Seat Label Start and App Plan Mirror - Design".
 */

/**
 * Column letters to their index, the inverse of dh_seat_letter():
 * A = 0, Z = 25, AA = 26. FALSE unless 1 to 3 letters.
 */
function dh_seat_letter_index($text)
{
	$t = strtoupper(trim((string) $text));
	if (!preg_match('/^[A-Z]{1,3}$/', $t))
		return FALSE;
	$n = 0;
	for ($i = 0; $i < strlen($t); $i++)
		$n = $n * 26 + (ord($t[$i]) - 64);
	return $n - 1;
}

/**
 * What a person typed as a grid's start, as the offset to save. Empty = 0 (the
 * normal start). Style 1 takes a letter (I -> 8). Style 2 takes a column number
 * and the numbered style a first seat number, both 1 or more (7 -> 6, 101 -> 100).
 * FALSE when the text does not fit the style.
 */
function dh_seat_label_start_parse($conv, $text)
{
	$t = trim((string) $text);
	if ($t === '')
		return 0;
	if ((int) $conv == 1)
		return dh_seat_letter_index($t);
	if (!preg_match('/^[0-9]{1,5}$/', $t) || (int) $t < 1)
		return FALSE;
	return (int) $t - 1;
}

/** An offset as a person would type it: '' for the normal start, style 1 a letter, else a number. */
function dh_seat_label_start_text($conv, $start)
{
	$start = (int) $start;
	if ($start <= 0)
		return '';
	return ((int) $conv == 1) ? dh_seat_letter($start) : (string) ($start + 1);
}

/** What to type, for an error message after "Main grid: ". */
function dh_seat_label_start_hint($conv)
{
	$conv = (int) $conv;
	if ($conv == 1)
		return 'type a letter, for example I. Leave it empty for the normal start.';
	if ($conv == 2)
		return 'type a column number of 1 or more, for example 7. Leave it empty for the normal start.';
	return 'type a seat number of 1 or more, for example 101. Leave it empty for the normal start.';
}

/**
 * The saved starts of one course's plan ('main' or 'group'), for the plan pages
 * and the Dipi App bundle: array(section id => array('main' => n, 'chowky' => n)).
 * Only sections with a saved row are listed. A row saved under another naming
 * style than the centre's current one is ignored (that grid uses the normal start).
 */
function dh_seat_label_starts($course_id, $plan)
{
	$out = array();
	if (!db_table_exists('dh_seat_label_start'))
		return $out;
	$conv = db_query("select cs.cs_seat_naming_conv from dh_course c join dh_center_setting cs on cs.cs_center = c.c_center where c.c_id = :c",
		array(':c' => (int) $course_id))->fetchField();
	if ($conv === FALSE)
		return $out;
	$res = db_query("select sls_section, sls_grid, sls_start from dh_seat_label_start
		where sls_course = :c and sls_plan = :p and sls_conv = :v order by sls_id",
		array(':c' => (int) $course_id, ':p' => (string) $plan, ':v' => (int) $conv));
	foreach ($res as $r)
	{
		if (!in_array($r->sls_grid, array('main', 'chowky'), TRUE))
			continue;
		if (!isset($out[$r->sls_section]))
			$out[$r->sls_section] = array('main' => 0, 'chowky' => 0);
		$out[$r->sls_section][$r->sls_grid] = (int) $r->sls_start;
	}
	return $out;
}

/** One grid's start from dh_seat_label_starts(): 0 when none is saved. */
function dh_seat_label_start_of($starts, $sec, $grid)
{
	return isset($starts[$sec][$grid]) ? (int) $starts[$sec][$grid] : 0;
}

/**
 * Save both grids of one section, from the "Seat labels" box. $texts holds what
 * was typed: array('main' => ..., 'chowky' => ...); an empty field = the normal
 * start (its row is removed). The same start is saved for every course drawn on
 * that plan (combined seating), so the plan reads the same from either course.
 * Returns array('status' => 'OK' | 'Failed', 'msg' => ...).
 */
function dh_seat_label_start_save($centre, $course_id, $plan, $sec, $texts, $uid)
{
	$centre = (int) $centre;
	$course_id = (int) $course_id;
	if ($centre <= 0 || (int) db_query("select c_center from dh_course where c_id = :c", array(':c' => $course_id))->fetchField() !== $centre)
		return array('status' => 'Failed', 'msg' => 'This course is not in this centre.');
	$p = (strlen($sec) <= 32) ? dh_seat_regen_parse($plan, $sec) : FALSE;
	if ($p !== FALSE && $plan === 'group' && !preg_match('/^g([0-9]+|Unassigned)_[MF]$/', $sec))
		$p = FALSE;
	if ($p === FALSE)
		return array('status' => 'Failed', 'msg' => 'Unknown section.');
	$block = dh_seat_label_start_blocker($course_id);
	if ($block !== '')
		return array('status' => 'Failed', 'msg' => $block);
	$conv = (int) db_query("select cs_seat_naming_conv from dh_center_setting where cs_center = :c", array(':c' => $centre))->fetchField();
	$starts = array();
	foreach (array('main' => 'Main grid', 'chowky' => 'Chowky grid') as $grid => $name)
	{
		$s = dh_seat_label_start_parse($conv, isset($texts[$grid]) ? $texts[$grid] : '');
		if ($s === FALSE)
			return array('status' => 'Failed', 'msg' => $name.': '.dh_seat_label_start_hint($conv));
		$starts[$grid] = $s;
	}
	$now = date('Y-m-d H:i:s');
	foreach (dh_seat_plan_courses($course_id) as $c)
		foreach ($starts as $grid => $s)
		{
			if ($s == 0)
				db_delete('dh_seat_label_start')->condition('sls_course', $c)->condition('sls_plan', $plan)
					->condition('sls_section', $sec)->condition('sls_grid', $grid)->execute();
			else
				db_merge('dh_seat_label_start')
					->key(array('sls_course' => $c, 'sls_plan' => $plan, 'sls_section' => $sec, 'sls_grid' => $grid))
					->fields(array('sls_conv' => $conv, 'sls_start' => $s, 'sls_updated_by' => (int) $uid, 'sls_updated' => $now))
					->execute();
		}
	$say = array();
	foreach (array('main', 'chowky') as $grid)
	{
		$t = dh_seat_label_start_text($conv, $starts[$grid]);
		$say[] = $grid.' grid '.($t === '' ? 'normal' : 'starts at '.$t);
	}
	$msg = 'Seat labels for '.$p[2].': '.implode(', ', $say);
	db_insert('dh_log')->fields(array('l_center' => $centre, 'l_module' => 'Course', 'l_identifier' => $course_id,
		'l_event' => 'Seat Labels', 'l_msg' => $msg, 'l_user' => (int) $uid))->execute();
	return array('status' => 'OK', 'msg' => $msg);
}

/** The JSON route's work: the page token first, then the save. $post: the POST values. */
function dh_seat_label_start_request($centre, $course_id, $post)
{
	global $user;
	if (!isset($post['token']) || !drupal_valid_token((string) $post['token'], 'seat-label-start'))
		return array('status' => 'Failed', 'msg' => 'Session expired. Reload the page and try again.');
	return dh_seat_label_start_save($centre, $course_id,
		isset($post['plan']) ? (string) $post['plan'] : '',
		isset($post['sec']) ? (string) $post['sec'] : '',
		array('main' => isset($post['main']) ? (string) $post['main'] : '', 'chowky' => isset($post['chowky']) ? (string) $post['chowky'] : ''),
		$user->uid);
}

/**
 * Route seat-label-start/%centre_id/%course_id (POST, JSON), from the box on the
 * seating plans. The loaders have already checked the user's centre access.
 */
function dh_seat_label_start_update($centre, $course)
{
	drupal_json_output(dh_seat_label_start_request($centre, $course, $_POST));
	drupal_exit();
}

/**
 * "Seat labels..." button for one section's tool bar on the seating plans (only
 * with the edit tools). It carries the section and the starts saved now, as the
 * person would type them; js/seat-label-start.js opens the box.
 */
function dh_seat_label_start_button($plan, $sec, $label, $conv, $starts, $blocked = '')
{
	if ($blocked !== '')
		return ' <span class="dh-sls-note no-print">&#9998; '.check_plain($blocked).'</span>';
	return ' <button type="button" class="dh-sls-btn no-print"'
		.' data-plan="'.check_plain($plan).'" data-sec="'.check_plain($sec).'" data-label="'.check_plain($label).'"'
		.' data-anchor="'.check_plain(dh_seat_sec_anchor($sec)).'"'
		.' data-main="'.check_plain(dh_seat_label_start_text($conv, dh_seat_label_start_of($starts, $sec, 'main'))).'"'
		.' data-chowky="'.check_plain(dh_seat_label_start_text($conv, dh_seat_label_start_of($starts, $sec, 'chowky'))).'"'
		.' title="Start the seat labels of this section from another letter or number (this course only)">&#9998; Seat labels&hellip;</button>';
}

/**
 * Script tags for js/seat-label-start.js and its settings (save URL, token,
 * naming style). The plans print their own HTML, so the caller appends this;
 * '' after the first call.
 */
function dh_seat_label_start_js($centre, $course, $conv)
{
	$added = &drupal_static(__FUNCTION__);
	if ($added)
		return '';
	$added = TRUE;
	$path = drupal_get_path('module', 'dh_manageapp').'/js/seat-label-start.js';
	return '<script>window.dhSeatLabelCfg = '.drupal_json_encode(array(
			'url' => url('seat-label-start/'.(int) $centre.'/'.(int) $course),
			'token' => drupal_get_token('seat-label-start'),
			'conv' => (int) $conv,
		)).';</script>'
		.'<script src="'.$GLOBALS['base_url'].'/'.$path.'?v='.(file_exists($path) ? filemtime($path) : 0).'"></script>';
}

/** The courses a plan page of $course_id draws: the course and its own combined course, as the seating dispatcher does. */
function dh_seat_label_page_courses($course_id)
{
	$course_id = (int) $course_id;
	$other = (int) db_query("select c_combined_seat_course from dh_course where c_id = :c", array(':c' => $course_id))->fetchField();
	return $other > 0 ? array($course_id, $other) : array($course_id);
}

/**
 * '' when the plan page of $course_id draws every course that shares its plan,
 * so a save there (and the page reload after it) refreshes every saved label.
 * Otherwise the sentence to show instead of the button, naming the course
 * whose plan page draws them all, if there is one.
 */
function dh_seat_label_start_blocker($course_id)
{
	$all = dh_seat_plan_courses($course_id);
	if (!array_diff($all, dh_seat_label_page_courses($course_id)))
		return '';
	foreach ($all as $c)
		if (!array_diff($all, dh_seat_label_page_courses($c)))
			return 'This seating plan is shared with course '.$c.'. Change its seat labels from the seating plan of course '.$c.'.';
	return 'This seating plan is shared by courses '.implode(', ', $all).'. Its seat labels cannot be changed here.';
}
