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
