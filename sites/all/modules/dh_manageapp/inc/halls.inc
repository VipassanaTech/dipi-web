<?php
/**
 * Dhamma Halls: a centre's halls (code + name), the usual hall of each student group (1-9) and a course's own
 * changes. Only centres with 2 or more halls use them. With 0 or 1 hall every helper answers "no hall", so every
 * group label stays exactly as it was (the one-hall rule).
 * Tables: dh_center_hall, dh_center_hall_group, dh_course_hall_group (deploy/2026-11-23-halls-schema.sql).
 */

/** Clears the per-request caches (after a save, and in tests). */
function dh_halls_reset()
{
	drupal_static_reset('dh_halls_list');
	drupal_static_reset('dh_halls_course_map');
}

/** The centre's halls in order: ch_id => array('id' => int, 'code' => string, 'name' => string). Removed ones left out. */
function dh_halls_list($centre_id)
{
	$cache = &drupal_static(__FUNCTION__, array());
	$centre_id = (int) $centre_id;
	if (!isset($cache[$centre_id]))
	{
		$cache[$centre_id] = array();
		foreach (db_query("select ch_id, ch_code, ch_name from dh_center_hall where ch_center = :c and ch_deleted = 0 order by ch_weight, ch_id",
			array(':c' => $centre_id)) as $r)
			$cache[$centre_id][(int) $r->ch_id] = array('id' => (int) $r->ch_id, 'code' => (string) $r->ch_code, 'name' => (string) $r->ch_name);
	}
	return $cache[$centre_id];
}

/** TRUE when the centre has 2 or more halls (the feature is on). */
function dh_halls_on($centre_id)
{
	return count(dh_halls_list($centre_id)) >= 2;
}

/** The usual hall id of each group 1..9 (unset, or a removed hall -> the first hall). array() when the feature is off. */
function dh_halls_usual($centre_id)
{
	$halls = dh_halls_list($centre_id);
	if (count($halls) < 2)
		return array();
	reset($halls);
	$first = key($halls);
	$out = array();
	for ($g = 1; $g <= 9; $g++)
		$out[$g] = $first;
	foreach (db_query("select chg_group, chg_hall from dh_center_hall_group where chg_center = :c", array(':c' => (int) $centre_id)) as $r)
		if (isset($out[(int) $r->chg_group]) && isset($halls[(int) $r->chg_hall]))
			$out[(int) $r->chg_group] = (int) $r->chg_hall;
	return $out;
}

/** The course whose halls a course follows: itself, or the owner of its shared seating plan (the course the others point to). */
function dh_halls_owner_course($course_id)
{
	$course_id = (int) $course_id;
	$all = dh_seat_plan_courses($course_id);
	if (count($all) < 2)
		return $course_id;
	$own = db_query("select c_id from dh_course where c_id in (:ids) and (c_combined_seat_course is null or c_combined_seat_course = 0) order by c_id",
		array(':ids' => $all))->fetchCol();
	return $own ? (int) $own[0] : (int) min($all);
}

/** The hall of each group 1..9 on a course: group => array('id','code','name'). array() when the centre has 0 or 1 hall. */
function dh_halls_course_map($course_id)
{
	$cache = &drupal_static(__FUNCTION__, array());
	$course_id = (int) $course_id;
	if (isset($cache[$course_id]))
		return $cache[$course_id];
	$owner = dh_halls_owner_course($course_id);
	$centre = (int) db_query("select c_center from dh_course where c_id = :c", array(':c' => $owner))->fetchField();
	$halls = dh_halls_list($centre);
	$map = array();
	if (count($halls) >= 2)
	{
		$ids = dh_halls_usual($centre);
		foreach (db_query("select cg_group, cg_hall from dh_course_hall_group where cg_course = :c", array(':c' => $owner)) as $r)
			if (isset($ids[(int) $r->cg_group]) && isset($halls[(int) $r->cg_hall]))
				$ids[(int) $r->cg_group] = (int) $r->cg_hall;
		foreach ($ids as $g => $id)
			$map[$g] = $halls[$id];
	}
	return $cache[$course_id] = $map;
}

/**
 * The hall added to a group label, e.g. ' · DH3' ($kind 'code') or ' · Dhamma Hall 3' ($kind 'name').
 * '' when the feature is off, or the group is not 1-9 ("No Group"). Not HTML-escaped.
 */
function dh_halls_tail($course_id, $group, $kind = 'code')
{
	$t = trim((string) $group);
	if ($t === '' || !ctype_digit($t))
		return '';
	$g = (int) $t;
	if ($g < 1 || $g > 9)
		return '';
	$map = dh_halls_course_map($course_id);
	if (!isset($map[$g]))
		return '';
	return ' · ' . ($kind === 'name' ? $map[$g]['name'] : $map[$g]['code']);
}

/** Hall code of each group for page scripts: array('1' => 'DH1', ...); array() when the feature is off. */
function dh_halls_js_codes($course_id)
{
	$out = array();
	foreach (dh_halls_course_map($course_id) as $g => $h)
		$out[(string) $g] = $h['code'];
	return $out;
}

/** Usual hall codes in force for a check: the form's choices, or (form had none) the saved ones. group => code. */
function _dh_halls_usual_codes($centre_id, $usual)
{
	if ($usual)
		return $usual;
	$halls = dh_halls_list($centre_id);
	$out = array();
	foreach (dh_halls_usual($centre_id) as $g => $id)
		$out[$g] = $halls[$id]['code'];
	return $out;
}

/** Checks a Centre Settings halls save. Returns error texts (simple English); array() = fine. See dh_halls_centre_save(). */
function dh_halls_centre_validate($centre_id, $rows, $usual)
{
	$centre_id = (int) $centre_id;
	$errors = array();
	$mine = db_query("select ch_id, ch_code from dh_center_hall where ch_center = :c and ch_deleted = 0", array(':c' => $centre_id))->fetchAllKeyed();
	$kept = array();      // upper code => code
	$removed = array();   // id => code
	foreach ($rows as $r)
	{
		$id = (int) $r['id'];
		$code = trim((string) $r['code']);
		$name = trim((string) $r['name']);
		if ($id === 0 && $code === '' && $name === '')
			continue;
		if ($id && !isset($mine[$id]))
		{
			$errors[] = 'This hall is not part of this centre.';
			continue;
		}
		if ($id && !empty($r['remove']))
		{
			$removed[$id] = $mine[$id];
			continue;
		}
		if (!preg_match('/^[A-Za-z0-9]{1,6}$/', $code))
			$errors[] = 'Hall code must be 1 to 6 letters or digits: ' . $code;
		elseif (mb_strlen($name) < 1 || mb_strlen($name) > 40)
			$errors[] = 'Hall name must be 1 to 40 characters: ' . $code;
		elseif (isset($kept[strtoupper($code)]))
			$errors[] = 'Hall code ' . $kept[strtoupper($code)] . ' is used twice.';
		else
			$kept[strtoupper($code)] = $code;
	}
	foreach ($usual as $g => $code)
		if (trim((string) $code) !== '' && !isset($kept[strtoupper(trim((string) $code))]))
			$errors[] = 'Group ' . (int) $g . ': choose one of this centre\'s halls.';
	if ($errors)
		return $errors;
	$codes = _dh_halls_usual_codes($centre_id, $usual);
	foreach ($removed as $id => $code)
	{
		foreach ($codes as $g => $uc)
			if (strtoupper((string) $uc) === strtoupper((string) $code))
			{
				$errors[] = 'Hall ' . $code . ' is still the usual hall of Group ' . (int) $g . '. Move that group first.';
				continue 2;
			}
		$k = db_query("select cg.cg_course from dh_course_hall_group cg join dh_course c on c.c_id = cg.cg_course
			where cg.cg_hall = :h and c.c_deleted = 0 and c.c_end >= curdate() order by cg.cg_course limit 1", array(':h' => $id))->fetchField();
		if ($k)
			$errors[] = 'Hall ' . $code . ' is used by course ' . (int) $k . '. Change that course first.';
	}
	return $errors;
}

/**
 * Saves a centre's halls from the Centre Settings form, writing Change Log lines (area 'halls').
 * $rows: list in display order of array('id' => int (0 = new), 'code', 'name', 'remove' => bool); a new row with empty
 * code and name is skipped. $usual: group (1-9) => hall code ('' = first hall); empty = keep the saved usual halls.
 * Returns error texts; nothing is saved when there is one.
 */
function dh_halls_centre_save($centre_id, $rows, $usual, $uid)
{
	$centre_id = (int) $centre_id;
	$errors = dh_halls_centre_validate($centre_id, $rows, $usual);
	if ($errors)
		return $errors;
	$now = date('Y-m-d H:i:s');
	$old = db_query("select ch_id, ch_code, ch_name, ch_weight from dh_center_hall where ch_center = :c and ch_deleted = 0", array(':c' => $centre_id))->fetchAllAssoc('ch_id', PDO::FETCH_ASSOC);
	$old_usual = dh_halls_usual($centre_id);   // group => hall id before this save; array() when fewer than 2 halls
	$txn = db_transaction();
	$w = 0;
	foreach ($rows as $r)
	{
		$id = (int) $r['id'];
		$code = trim((string) $r['code']);
		$name = trim((string) $r['name']);
		if ($id === 0 && $code === '' && $name === '')
			continue;
		if ($id && !empty($r['remove']))
		{
			db_update('dh_center_hall')->fields(array('ch_deleted' => 1, 'ch_updated' => $now, 'ch_updated_by' => (int) $uid))
				->condition('ch_id', $id)->condition('ch_center', $centre_id)->execute();
			dh_centre_log_write($centre_id, 'halls', 'delete', array('hall' => array($old[$id]['ch_code'] . ' ' . $old[$id]['ch_name'], NULL)), $id);
			continue;
		}
		if ($id)
		{
			$o = $old[$id];
			if ($o['ch_code'] !== $code || $o['ch_name'] !== $name || (int) $o['ch_weight'] !== $w)
				db_update('dh_center_hall')->fields(array('ch_code' => $code, 'ch_name' => $name, 'ch_weight' => $w, 'ch_updated' => $now, 'ch_updated_by' => (int) $uid))
					->condition('ch_id', $id)->condition('ch_center', $centre_id)->execute();
			if ($o['ch_code'] !== $code || $o['ch_name'] !== $name)
				dh_centre_log_write($centre_id, 'halls', 'update', array('hall' => array($o['ch_code'] . ' ' . $o['ch_name'], $code . ' ' . $name)), $id);
		}
		else
		{
			$id = (int) db_insert('dh_center_hall')->fields(array('ch_center' => $centre_id, 'ch_code' => $code, 'ch_name' => $name, 'ch_weight' => $w,
				'ch_deleted' => 0, 'ch_created' => $now, 'ch_created_by' => (int) $uid, 'ch_updated' => $now, 'ch_updated_by' => (int) $uid))->execute();
			dh_centre_log_write($centre_id, 'halls', 'create', array('hall' => array(NULL, $code . ' ' . $name)), $id);
		}
		$w++;
	}
	dh_halls_reset();
	$halls = dh_halls_list($centre_id);
	if (count($halls) >= 2 && $usual)
	{
		$by_code = array();
		foreach ($halls as $id => $h)
			$by_code[strtoupper($h['code'])] = $id;
		reset($halls);
		$first = key($halls);
		for ($g = 1; $g <= 9; $g++)
		{
			$want = isset($usual[$g]) ? strtoupper(trim((string) $usual[$g])) : '';
			$id = ($want !== '' && isset($by_code[$want])) ? $by_code[$want] : $first;
			db_merge('dh_center_hall_group')->key(array('chg_center' => $centre_id, 'chg_group' => $g))->fields(array('chg_hall' => $id))->execute();
			$was = isset($old_usual[$g]) ? (int) $old_usual[$g] : 0;
			if ($was !== $id)   // compare halls, not codes: renaming the usual hall is not a change of hall
				dh_centre_log_write($centre_id, 'halls', 'update', array('hall_group' => array('Group ' . $g . ': ' . ($was ? $old[$was]['ch_code'] : ''),
					'Group ' . $g . ': ' . $halls[$id]['code'])), $id);
		}
	}
	dh_halls_reset();
	return array();
}
