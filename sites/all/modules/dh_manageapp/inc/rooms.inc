<?php
/**
 * Rooms: a centre's physical rooms (dh_center_room), the places in each (dh_center_room_place, what a
 * student's aa_acco holds) and each room's toilet type. A student's room stays aa_section + aa_acco;
 * these tables only describe the rooms.
 */

function dh_room_toilet_types()
{
	return array('indian' => 'Indian', 'western' => 'Western', 'both' => 'Both', 'none' => 'No attached toilet', 'unset' => 'Not set');
}

/** TRUE when the centre has at least one room in the new list. */
function dh_rooms_centre_has_rooms($centre_id)
{
	return (bool) db_query("select 1 from dh_center_room where cr_center = :c and cr_deleted = 0 limit 1", array(':c' => (int) $centre_id))->fetchField();
}

/** The centre's rooms (one side or both) with their places, in display order. */
function dh_rooms_load($centre_id, $gender = NULL)
{
	$args = array(':c' => (int) $centre_id);
	$where = '';
	$pwhere = '';
	if ($gender !== NULL) { $where = ' and cr_gender = :g'; $pwhere = ' and r.cr_gender = :g'; $args[':g'] = $gender; }
	$rooms = array();
	foreach (db_query("select * from dh_center_room where cr_center = :c and cr_deleted = 0" . $where, $args) as $r)
		$rooms[$r->cr_id] = array('id' => (int) $r->cr_id, 'gender' => strtoupper($r->cr_gender), 'section' => $r->cr_section, 'name' => $r->cr_name,
			'toilet' => $r->cr_toilet, 'review' => (int) $r->cr_review, 'weight' => (int) $r->cr_weight, 'places' => array());
	if ($rooms)
		foreach (db_query("select p.crp_id, p.crp_room, p.crp_name from dh_center_room_place p join dh_center_room r on r.cr_id = p.crp_room
			where r.cr_center = :c and r.cr_deleted = 0 and p.crp_deleted = 0" . $pwhere . " order by p.crp_weight, p.crp_id", $args) as $p)
			$rooms[$p->crp_room]['places'][] = array('id' => (int) $p->crp_id, 'name' => $p->crp_name);
	$rooms = array_values($rooms);
	usort($rooms, 'dh_rooms_cmp');
	return $rooms;
}

function dh_rooms_cmp($a, $b)
{
	if ($a['gender'] !== $b['gender'])
		return strcmp($a['gender'], $b['gender']);
	$s = strnatcasecmp($a['section'], $b['section']);
	if ($s)
		return $s;
	if ($a['weight'] !== $b['weight'])
		return $a['weight'] - $b['weight'];
	return strnatcasecmp($a['name'], $b['name']);
}

/** Same shape as the old dh_get_acco(): [gender][section][] = place name, in room order. */
function dh_rooms_acco($centre_id)
{
	$acco = array('M' => array(), 'F' => array());
	foreach (dh_rooms_load($centre_id) as $r)
		foreach ($r['places'] as $p)
			$acco[$r['gender']][$r['section']][] = $p['name'];
	return $acco;
}

/** Where each place is: [gender][section][place] = array('room' => room name, 'toilet' => key). */
function dh_rooms_place_index($centre_id)
{
	$idx = array('M' => array(), 'F' => array());
	foreach (dh_rooms_load($centre_id) as $r)
		foreach ($r['places'] as $p)
			$idx[$r['gender']][$r['section']][$p['name']] = array('room' => $r['name'], 'toilet' => $r['toilet']);
	return $idx;
}

/** A course's short label for messages: "3 Day Course (4-7 Dec)". */
function dh_rooms_course_label($course_id)
{
	$c = db_query("select c.c_start, c.c_end, td.td_val1 as type from dh_course c left join dh_type_detail td on td.td_id = c.c_course_type where c.c_id = :c",
		array(':c' => (int) $course_id))->fetchAssoc();
	if (!$c)
		return '';
	$s = strtotime($c['c_start']);
	$e = strtotime($c['c_end']);
	$d = (date('M', $s) === date('M', $e)) ? date('j', $s) . '-' . date('j M', $e) : date('j M', $s) . ' - ' . date('j M', $e);
	return trim($c['type'] . ' (' . $d . ')');
}

/** Other courses of the same centre running at the same time (more than a shared changeover day). */
function dh_rooms_overlapping_courses($course_id)
{
	$c = db_query("select c_center, c_start, c_end from dh_course where c_id = :c", array(':c' => (int) $course_id))->fetchAssoc();
	if (!$c)
		return array();
	return db_query("select c_id from dh_course where c_center = :cen and c_id <> :c and c_deleted = 0 and coalesce(c_cancelled, 0) = 0
		and c_start < :e and :s < c_end order by c_id", array(':cen' => $c['c_center'], ':c' => (int) $course_id, ':s' => $c['c_start'], ':e' => $c['c_end']))->fetchCol();
}

/** Places students of overlapping courses hold: [gender][section][place] = label of that course. */
function dh_rooms_overlap_taken($course_id)
{
	$taken = array('M' => array(), 'F' => array());
	$others = dh_rooms_overlapping_courses($course_id);
	if (!$others)
		return $taken;
	$labels = array();
	foreach (db_query("select a.a_course, a.a_gender, TRIM(aa.aa_section) as aa_section, TRIM(aa.aa_acco) as aa_acco from dh_applicant a join dh_applicant_attended aa on aa.aa_applicant = a.a_id
		where a.a_course in (:o) and a.a_attended = 1 and TRIM(aa.aa_acco) <> ''", array(':o' => $others)) as $r)
	{
		if (!isset($labels[$r->a_course]))
			$labels[$r->a_course] = dh_rooms_course_label($r->a_course);
		$taken[strtoupper($r->a_gender)][$r->aa_section][$r->aa_acco] = $labels[$r->a_course];
	}
	return $taken;
}

/** Places a running or upcoming course of the centre uses: "G|section|place" => label of the first such course. */
function dh_rooms_places_in_use($centre_id)
{
	$used = array();
	$labels = array();
	foreach (db_query("select a.a_course, a.a_gender, TRIM(aa.aa_section) as aa_section, TRIM(aa.aa_acco) as aa_acco from dh_course c join dh_applicant a on a.a_course = c.c_id
		join dh_applicant_attended aa on aa.aa_applicant = a.a_id where c.c_center = :c and c.c_deleted = 0 and c.c_end >= curdate() and a.a_attended = 1 and trim(aa.aa_acco) <> ''
		order by c.c_start", array(':c' => (int) $centre_id)) as $r)
	{
		$k = strtoupper($r->a_gender) . '|' . $r->aa_section . '|' . $r->aa_acco;
		if (isset($used[$k]))
			continue;
		if (!isset($labels[$r->a_course]))
			$labels[$r->a_course] = dh_rooms_course_label($r->a_course);
		$used[$k] = $labels[$r->a_course];
	}
	return $used;
}

/** One room-list change into the centre's Change Log (area "rooms"). $changes = field => array(old, new). */
function dh_rooms_log($centre_id, $action, $changes, $room_id = NULL)
{
	if ($changes)
		dh_centre_log_write((int) $centre_id, 'rooms', $action, $changes, $room_id);
}
