<?php
/**
 * Room Board of a course: the centre's rooms with who is in each place, the people not placed yet, and
 * tap-to-place actions. A placement is saved in aa_section + aa_acco, exactly like the Zero Day popup.
 */

/** The course row if it belongs to the centre, else NULL. */
function _dh_room_board_course($centre_id, $course_id)
{
	$c = db_query("select c_id, c_center, c_finalized from dh_course where c_id = :c and c_deleted = 0", array(':c' => (int) $course_id))->fetchAssoc();
	return ($c && (int) $c['c_center'] === (int) $centre_id) ? $c : NULL;
}

function _dh_room_board_sides()
{
	$s = array();
	if (user_access('access male')) $s[] = 'M';
	if (user_access('access female')) $s[] = 'F';
	return $s;
}

function dh_room_board_data($centre_id, $course_id)
{
	$c = _dh_room_board_course($centre_id, $course_id);
	if (!$c)
		return array('ok' => FALSE, 'msg' => 'This course is not in this centre.');
	$sides = _dh_room_board_sides();
	// A shared (male + female) section shows on every side the user has.
	$rooms = array_values(array_filter(dh_rooms_load($centre_id), function ($r) use ($sides) { return $sides && ($r['gender'] === 'B' || in_array($r['gender'], $sides, TRUE)); }));
	$shared = dh_rooms_shared_sections($centre_id);
	$cross = array_fill_keys($sides, array());   // [side][section][place] = 'Male'/'Female': a shared place held by the other side in this course
	$people = array();
	foreach (db_query("select a.a_id, a.a_conf_no, a.a_f_name, a.a_m_name, a.a_l_name, a.a_gender, a.a_type, a.a_old, a.a_problem_physical, TIMESTAMPDIFF(YEAR, a.a_dob, CURDATE()) as age,
		a.a_occupation, a.a_education, co.c_name as country_name, co.c_nationality, st.s_name as state_name, ae.ae_nationality,
		aa.aa_section, aa.aa_acco, aa.aa_chowky, aa.aa_chair, ae.ae_desc_physical, ae.ae_desc_special_requirement
		from dh_applicant a join dh_applicant_attended aa on aa.aa_applicant = a.a_id left join dh_applicant_extra ae on ae.ae_applicant = a.a_id
		left join dh_country co on co.c_code = a.a_country left join dh_state st on (st.s_code = a.a_state and st.s_country = a.a_country)
		where a.a_course = :c and a.a_attended = 1 order by a.a_f_name, a.a_l_name", array(':c' => (int) $course_id))->fetchAll(PDO::FETCH_ASSOC) as $r)
	{
		$g = strtoupper($r['a_gender']);
		$sec = trim((string) $r['aa_section']);
		$plc = trim((string) $r['aa_acco']);
		if ($plc !== '' && dh_rooms_is_shared($shared, $sec))
			foreach ($sides as $s)
				if ($s !== $g)
					$cross[$s][$sec][$plc] = $g === 'F' ? 'Female' : 'Male';
		if (!in_array($g, $sides, TRUE))
			continue;
		$hints = array();
		if ($r['aa_chair']) $hints[] = 'Chair';
		if ($r['aa_chowky']) $hints[] = 'Chowky';
		if (trim((string) $r['ae_desc_physical']) !== '') $hints[] = 'Physical: ' . mb_substr(trim($r['ae_desc_physical']), 0, 60);
		elseif ($r['a_problem_physical']) $hints[] = 'Physical problem';
		if (trim((string) $r['ae_desc_special_requirement']) !== '') $hints[] = 'Needs: ' . mb_substr(trim($r['ae_desc_special_requirement']), 0, 60);
		$nat = trim((string) $r['ae_nationality']);
		$people[] = array('a_id' => (int) $r['a_id'], 'conf' => trim((string) $r['a_conf_no']), 'name' => dh_day0_name($r), 'gender' => $g, 'server' => strtolower((string) $r['a_type']) === 'sevak',
			'old' => (bool) $r['a_old'], 'age' => $r['age'] === NULL ? NULL : (int) $r['age'],
			'occupation' => mb_substr(dh_day0_case((string) $r['a_occupation']), 0, 60), 'education' => mb_substr(trim((string) $r['a_education']), 0, 60),
			'state' => (string) $r['state_name'], 'country' => (string) $r['country_name'],
			'nationality' => ($nat !== '' && strcasecmp($nat, (string) $r['c_nationality']) !== 0) ? mb_substr(dh_day0_case($nat), 0, 60) : '',
			'hints' => $hints,
			'section' => $sec, 'place' => $plc);
	}
	return array('ok' => TRUE, 'course' => dh_rooms_course_label($course_id), 'optional' => dh_room_optional($course_id), 'finalized' => (bool) $c['c_finalized'],
		'sides' => $sides, 'has_rooms' => dh_rooms_centre_has_rooms($centre_id), 'rooms' => $rooms, 'people' => $people, 'cross' => $cross, 'overlap' => array_intersect_key(dh_rooms_overlap_taken($course_id), array_flip($sides)), 'toilets' => dh_room_toilet_types(),
		'log' => dh_room_board_log($course_id, $sides));
}

/**
 * The Room Board change log of a course (owner, 25 Nov 2026), newest first, for the sides this user sees: every room
 * change the board logged (dh_log 'Room' lines "Room: <old|none> → <new|none>", _dh_room_board_set). Each entry: t
 * (Y-m-d H:i:s), a_id, conf, name, gender, old ('' = placed fresh), new ('' = taken out), user.
 * Tested by tests/test-room-board-log.php.
 */
function dh_room_board_log($course_id, $sides, $limit = 300)
{
	if (!$sides)
		return array();
	$apps = array();
	foreach (db_query("select a_id, a_gender, a_conf_no, a_f_name, a_m_name, a_l_name from dh_applicant where a_course = :c", array(':c' => (int) $course_id))->fetchAll(PDO::FETCH_ASSOC) as $a)
		if (in_array(strtoupper($a['a_gender']), $sides, TRUE))
			$apps[(int) $a['a_id']] = $a;
	if (!$apps)
		return array();
	$out = array();
	$rows = db_query_range("select l.l_tstamp, l.l_identifier, l.l_msg, u.name from dh_log l force index (l_module) left join users u on u.uid = l.l_user
		where l.l_module = 'Applicant' and l.l_identifier in (:ids) and l.l_event = 'Room' order by l.l_id desc", 0, (int) $limit, array(':ids' => array_keys($apps)));
	foreach ($rows as $r)
	{
		if (!preg_match('/^Room: (.*) → (.*)$/su', (string) $r->l_msg, $m))
			continue;
		$a = $apps[(int) $r->l_identifier];
		$out[] = array('t' => (string) $r->l_tstamp, 'a_id' => (int) $r->l_identifier, 'conf' => trim((string) $a['a_conf_no']), 'name' => dh_day0_name($a),
			'gender' => strtoupper($a['a_gender']), 'old' => $m[1] === 'none' ? '' : $m[1], 'new' => $m[2] === 'none' ? '' : $m[2], 'user' => (string) $r->name);
	}
	return $out;
}

function _dh_room_board_person($course_id, $a_id)
{
	return db_query("select a.a_id, a.a_gender, a.a_f_name, a.a_m_name, a.a_l_name, trim(aa.aa_section) as aa_section, trim(aa.aa_acco) as aa_acco from dh_applicant a join dh_applicant_attended aa on aa.aa_applicant = a.a_id
		where a.a_id = :a and a.a_course = :c and a.a_attended = 1", array(':a' => (int) $a_id, ':c' => (int) $course_id))->fetchAssoc();
}

function _dh_room_board_set($centre_id, $p, $sec, $place, $uid)
{
	db_update('dh_applicant_attended')->fields(array('aa_section' => $sec, 'aa_acco' => $place, 'aa_updated' => date('Y-m-d H:i:s'), 'aa_updated_by' => (int) $uid))
		->condition('aa_applicant', $p['a_id'])->execute();
	dh_room_log_change($centre_id, $p['a_id'], $p['aa_section'], $p['aa_acco'], $sec, $place, $uid);
}

/** A room place as written in the log: "M-5A" (section-place), the place alone without a section, '' when there is no place. */
function dh_room_place_text($sec, $place)
{
	$sec = trim((string) $sec);
	$place = trim((string) $place);
	return $place === '' ? '' : ($sec === '' ? $place : $sec . '-' . $place);
}

/**
 * The "Room" log line of a room change made anywhere - Room Board, Zero Day popup, Edit Application (owner, 25 Nov
 * 2026) - so the Room Board change log (dh_room_board_log) shows every change: "Room: <old|none> → <new|none>".
 * Nothing is written when the room did not change (compared trimmed). TRUE when a line was written.
 * Tested by tests/test-room-change-log.php.
 */
function dh_room_log_change($centre_id, $app_id, $old_sec, $old_place, $new_sec, $new_place, $uid = 0)
{
	$old = dh_room_place_text($old_sec, $old_place);
	$new = dh_room_place_text($new_sec, $new_place);
	if ($old === $new)
		return FALSE;
	logit($centre_id, 'Room', $app_id, 'Room: ' . ($old === '' ? 'none' : $old) . ' → ' . ($new === '' ? 'none' : $new), $uid);
	return TRUE;
}

function dh_room_board_apply($centre_id, $course_id, $in, $uid)
{
	$c = _dh_room_board_course($centre_id, $course_id);
	if (!$c) return array('ok' => FALSE, 'msg' => 'This course is not in this centre.', 'confirm' => FALSE);
	if ($c['c_finalized']) return array('ok' => FALSE, 'msg' => "This course is finalized. Rooms can't be changed here.", 'confirm' => FALSE);
	$op = isset($in['op']) ? (string) $in['op'] : '';
	if (!dh_manageapp_lock_acquire('dh_app_update_attended', 10.0, 10))
		return array('ok' => FALSE, 'msg' => 'Someone else is saving. Please try again.', 'confirm' => FALSE);
	try
	{
		$p = _dh_room_board_person($course_id, isset($in['a_id']) ? $in['a_id'] : 0);
		if (!$p) return array('ok' => FALSE, 'msg' => 'This person has not arrived for this course.', 'confirm' => FALSE);
		$g = strtoupper($p['a_gender']);
		if (!in_array($g, _dh_room_board_sides(), TRUE)) return array('ok' => FALSE, 'msg' => 'You do not have access to this person.', 'confirm' => FALSE);
		if ($op === 'remove')
		{
			_dh_room_board_set($centre_id, $p, '', '', $uid);
			return array('ok' => TRUE, 'msg' => 'Removed from the room.', 'confirm' => FALSE);
		}
		if ($op === 'swap')
		{
			$q = _dh_room_board_person($course_id, isset($in['other']) ? $in['other'] : 0);
			if (!$q) return array('ok' => FALSE, 'msg' => 'This person has not arrived for this course.', 'confirm' => FALSE);
			if (!in_array(strtoupper($q['a_gender']), _dh_room_board_sides(), TRUE)) return array('ok' => FALSE, 'msg' => 'You do not have access to this person.', 'confirm' => FALSE);
			if (strtoupper($q['a_gender']) !== $g) return array('ok' => FALSE, 'msg' => 'Only people on the same side can swap.', 'confirm' => FALSE);
			if ($p['aa_acco'] === '' || $q['aa_acco'] === '') return array('ok' => FALSE, 'msg' => 'Both people need a place to swap.', 'confirm' => FALSE);
			$txn = db_transaction();
			try
			{
				_dh_room_board_set($centre_id, $p, $q['aa_section'], $q['aa_acco'], $uid);
				_dh_room_board_set($centre_id, $q, $p['aa_section'], $p['aa_acco'], $uid);
			}
			catch (Throwable $e)
			{
				$txn->rollback();
				throw $e;
			}
			return array('ok' => TRUE, 'msg' => 'Swapped.', 'confirm' => FALSE);
		}
		if ($op === 'place')
		{
			$sec = trim((string) (isset($in['section']) ? $in['section'] : ''));
			$place = trim((string) (isset($in['place']) ? $in['place'] : ''));
			$idx = dh_rooms_place_index($centre_id);
			if (!isset($idx[$g][$sec][$place])) return array('ok' => FALSE, 'msg' => 'There is no place ' . $place . ' in section ' . $sec . '.', 'confirm' => FALSE);
			// In a shared section a place is taken by anyone; elsewhere only by the same side.
			$is_shared = dh_rooms_is_shared(dh_rooms_shared_sections($centre_id), $sec) ? 1 : 0;
			$holder = db_query("select a.a_f_name, a.a_m_name, a.a_l_name, a.a_gender from dh_applicant a join dh_applicant_attended aa on aa.aa_applicant = a.a_id
				where a.a_course = :c and a.a_attended = 1 and (:shared = 1 or a.a_gender = :g) and trim(aa.aa_section) = :s and trim(aa.aa_acco) = :p and a.a_id <> :a limit 1",
				array(':c' => (int) $course_id, ':shared' => $is_shared, ':g' => $g, ':s' => $sec, ':p' => $place, ':a' => $p['a_id']))->fetchAssoc();
			if ($holder && strtoupper($holder['a_gender']) !== $g)
				return array('ok' => FALSE, 'msg' => 'Place ' . $place . ' is taken by ' . (strtoupper($holder['a_gender']) === 'F' ? 'a female student' : 'a male student') . ' in this course.', 'confirm' => FALSE);
			if ($holder) return array('ok' => FALSE, 'msg' => 'Place ' . $place . ' was just given to ' . dh_day0_name($holder) . '. The board has been refreshed.', 'confirm' => FALSE);
			$ov = dh_rooms_overlap_taken($course_id);
			$warn = array();
			if (isset($ov[$g][$sec][$place]))
				$warn[] = 'Place ' . $place . ' is used by ' . $ov[$g][$sec][$place] . ' at the same time.';
			// The other side in the same room, or in the section when entering it (see dh_rooms_mix_warning).
			$mix = $is_shared ? dh_rooms_mix_warning($course_id, $sec, $place, $g, $p['aa_section'], $p['aa_acco']) : '';
			if ($warn && empty($in['confirm']))
				return array('ok' => FALSE, 'msg' => implode(' ', $warn) . ($mix !== '' ? ' ' . $mix : '') . ' Use it anyway?', 'confirm' => TRUE);
			if ($mix !== '' && empty($in['confirm']))
				return array('ok' => FALSE, 'msg' => $mix . dh_rooms_mix_ask($g), 'confirm' => TRUE);
			_dh_room_board_set($centre_id, $p, $sec, $place, $uid);
			return array('ok' => TRUE, 'msg' => 'Saved.', 'confirm' => FALSE);
		}
		return array('ok' => FALSE, 'msg' => 'Unknown action.', 'confirm' => FALSE);
	}
	finally
	{
		lock_release('dh_app_update_attended');
	}
}

function dh_room_board_data_page($centre_id, $course_id)
{
	drupal_json_output(dh_room_board_data($centre_id, $course_id));
	drupal_exit();
}

function dh_room_board_save_page($centre_id, $course_id)
{
	global $user;
	if ($_SERVER['REQUEST_METHOD'] !== 'POST' || !isset($_POST['token']) || !drupal_valid_token($_POST['token'], 'dh-room-board'))
	{
		drupal_json_output(array('ok' => FALSE, 'msg' => 'Your page is out of date. Please reload it.', 'confirm' => FALSE));
		drupal_exit();
	}
	$out = dh_room_board_apply($centre_id, $course_id, $_POST, $user->uid);
	$out['data'] = dh_room_board_data($centre_id, $course_id);
	drupal_json_output($out);
	drupal_exit();
}

/** The Room Board page of a course (js/room-board.js). */
function dh_room_board_page($centre_id, $course_id)
{
	if (!_dh_room_board_course($centre_id, $course_id))
		return MENU_NOT_FOUND;
	$label = dh_rooms_course_label($course_id);
	drupal_set_title('Room Board');
	$path = drupal_get_path('module', 'dh_manageapp');
	drupal_add_css($path . '/css/rooms.css');
	drupal_add_js(array('dhRoomBoard' => array('dataUrl' => url('room-board/' . (int) $centre_id . '/' . (int) $course_id . '/data'),
		'saveUrl' => url('room-board/' . (int) $centre_id . '/' . (int) $course_id . '/save'), 'token' => drupal_get_token('dh-room-board'))), 'setting');
	// DataTables (search, sort, pages) for the room changes log under the board.
	drupal_add_css(libraries_get_path('editor') . '/DataTables/datatables.min.css');
	drupal_add_js(libraries_get_path('editor') . '/DataTables/datatables.min.js');
	drupal_add_js($path . '/js/rooms-common.js');
	drupal_add_js($path . '/js/room-board.js');
	// The log has its own box, outside the board, so the board's redraws never reset its search, sort or page.
	return '<div class="dh-rb"><p>' . l('Back to Course', 'course/' . (int) $centre_id . '/' . (int) $course_id) . ' | ' . l('Zero Day', 'zero-day/' . (int) $centre_id . '/' . (int) $course_id) . '</p>'
		. '<h2>' . check_plain($label) . '</h2><div id="dh-rb-app"><p>Loading the board…</p></div><div id="dh-rb-log"></div></div>';
}
