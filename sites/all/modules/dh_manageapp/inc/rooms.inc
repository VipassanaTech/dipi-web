<?php
/**
 * Rooms: a centre's physical rooms (dh_center_room), the places in each (dh_center_room_place, what a
 * student's aa_acco holds) and each room's toilet type. A student's room stays aa_section + aa_acco;
 * these tables only describe the rooms.
 * cr_gender is the side a section is used by: 'M' men, 'F' women, 'B' both (a shared section). A shared
 * section's name is not used by a men's or women's section of the same centre, so a student's saved
 * section + place still points to exactly one room. In a shared section a place is taken by anyone,
 * man or woman.
 */

function dh_room_toilet_types()
{
	return array('indian' => 'Indian', 'western' => 'Western', 'both' => 'Both', 'none' => 'No attached toilet', 'unset' => 'Not set');
}

/** "Men", "Women" or "Shared" (for messages and the Change Log). */
function dh_rooms_side_label($g)
{
	$l = array('M' => 'Men', 'F' => 'Women', 'B' => 'Shared');
	return isset($l[$g]) ? $l[$g] : (string) $g;
}

/** The centre's shared (men + women) sections: upper-cased name => name as spelled in the room list. */
function dh_rooms_shared_sections($centre_id)
{
	$out = array();
	foreach (db_query("select distinct cr_section from dh_center_room where cr_center = :c and cr_gender = 'B' and cr_deleted = 0", array(':c' => (int) $centre_id))->fetchCol() as $s)
		$out[mb_strtoupper($s)] = $s;
	return $out;
}

/** TRUE when $sec (a saved section, maybe with spaces or other letter case) is one of $shared (from dh_rooms_shared_sections). */
function dh_rooms_is_shared($shared, $sec)
{
	return isset($shared[mb_strtoupper(trim((string) $sec))]);
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
		$rooms[$r->cr_id] = array('id' => (int) $r->cr_id, 'gender' => strtoupper($r->cr_gender), 'section' => $r->cr_section, 'section_name' => (string) $r->cr_section_name, 'name' => $r->cr_name,
			'toilet' => $r->cr_toilet, 'review' => (int) $r->cr_review, 'weight' => (int) $r->cr_weight, 'places' => array());
	if ($rooms)
		foreach (db_query("select p.crp_id, p.crp_room, p.crp_name from dh_center_room_place p join dh_center_room r on r.cr_id = p.crp_room
			where r.cr_center = :c and r.cr_deleted = 0 and p.crp_deleted = 0" . $pwhere . " order by p.crp_weight, p.crp_id", $args) as $p)
			$rooms[$p->crp_room]['places'][] = array('id' => (int) $p->crp_id, 'name' => $p->crp_name);
	$rooms = array_values($rooms);
	usort($rooms, 'dh_rooms_cmp');
	return $rooms;
}

/** "CODE — Full name" for pages and the print; just the code when the section has no name. */
function dh_rooms_section_label($sec, $name)
{
	$name = trim((string) $name);
	return $name === '' ? (string) $sec : $sec . ' — ' . $name;
}

/** Section labels of a centre per side: [gender][section] = "CODE — Name". A shared section is under both sides. */
function dh_rooms_section_labels($centre_id)
{
	$out = array('M' => array(), 'F' => array());
	foreach (dh_rooms_load($centre_id) as $r)
		foreach (_dh_rooms_sides_of($r['gender']) as $g)
			$out[$g][$r['section']] = dh_rooms_section_label($r['section'], $r['section_name']);
	return $out;
}

/** Display order: section (natural, so a shared section sits among the side's own), then side, then room. */
function dh_rooms_cmp($a, $b)
{
	$s = strnatcasecmp($a['section'], $b['section']);
	if ($s)
		return $s;
	if ($a['gender'] !== $b['gender'])
		return strcmp($a['gender'], $b['gender']);
	if ($a['weight'] !== $b['weight'])
		return $a['weight'] - $b['weight'];
	return strnatcasecmp($a['name'], $b['name']);
}

/** The sides a room's section is listed under: a shared section under both. */
function _dh_rooms_sides_of($gender)
{
	return $gender === 'B' ? array('M', 'F') : array($gender);
}

/** Same shape as the old dh_get_acco(): [gender][section][] = place name, in room order. A shared section is under both. */
function dh_rooms_acco($centre_id)
{
	$acco = array('M' => array(), 'F' => array());
	foreach (dh_rooms_load($centre_id) as $r)
		foreach (_dh_rooms_sides_of($r['gender']) as $g)
			foreach ($r['places'] as $p)
				$acco[$g][$r['section']][] = $p['name'];
	return $acco;
}

/** Where each place is: [gender][section][place] = array('room' => room name, 'toilet' => key). A shared section is under both. */
function dh_rooms_place_index($centre_id)
{
	$idx = array('M' => array(), 'F' => array());
	foreach (dh_rooms_load($centre_id) as $r)
		foreach (_dh_rooms_sides_of($r['gender']) as $g)
			foreach ($r['places'] as $p)
				$idx[$g][$r['section']][$p['name']] = array('room' => $r['name'], 'toilet' => $r['toilet']);
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

/** Places students of overlapping courses hold: [gender][section][place] = label of that course.
 *  A place in a shared section is taken for both sides, whoever holds it. */
function dh_rooms_overlap_taken($course_id)
{
	$taken = array('M' => array(), 'F' => array());
	$others = dh_rooms_overlapping_courses($course_id);
	if (!$others)
		return $taken;
	$shared = dh_rooms_shared_sections(db_query("select c_center from dh_course where c_id = :c", array(':c' => (int) $course_id))->fetchField());
	$labels = array();
	foreach (db_query("select a.a_course, a.a_gender, TRIM(aa.aa_section) as aa_section, TRIM(aa.aa_acco) as aa_acco from dh_applicant a join dh_applicant_attended aa on aa.aa_applicant = a.a_id
		where a.a_course in (:o) and a.a_attended = 1 and TRIM(aa.aa_acco) <> ''", array(':o' => $others)) as $r)
	{
		if (!isset($labels[$r->a_course]))
			$labels[$r->a_course] = dh_rooms_course_label($r->a_course);
		foreach (dh_rooms_is_shared($shared, $r->aa_section) ? array('M', 'F') : array(strtoupper($r->a_gender)) as $g)
			if (!isset($taken[$g][$r->aa_section][$r->aa_acco]))
				$taken[$g][$r->aa_section][$r->aa_acco] = $labels[$r->a_course];
	}
	return $taken;
}

/** Where people of the other side are in shared section $sec while course $course_id runs (this course or one at
 *  the same time): '' when nowhere or $sec is not shared, 'this course', or the label of the other course. */
function dh_rooms_shared_mix($course_id, $sec, $g)
{
	$centre = db_query("select c_center from dh_course where c_id = :c", array(':c' => (int) $course_id))->fetchField();
	if (!$centre || !dh_rooms_is_shared(dh_rooms_shared_sections($centre), $sec))
		return '';
	$courses = array_merge(array((int) $course_id), array_map('intval', dh_rooms_overlapping_courses($course_id)));
	$hit = db_query("select a.a_course from dh_applicant a join dh_applicant_attended aa on aa.aa_applicant = a.a_id
		where a.a_course in (:cs) and a.a_attended = 1 and a.a_gender in ('M', 'F') and a.a_gender <> :g and trim(aa.aa_section) = :s and trim(aa.aa_acco) <> ''
		order by (a.a_course = :this) desc limit 1", array(':cs' => $courses, ':g' => $g, ':s' => trim((string) $sec), ':this' => (int) $course_id))->fetchField();
	if (!$hit)
		return '';
	return (int) $hit === (int) $course_id ? 'this course' : dh_rooms_course_label($hit);
}

/** "Section C has women in this course." (or "... in 3 Day Course (4-7 Dec) at the same time.") for dh_rooms_shared_mix(). */
function dh_rooms_mix_sentence($sec, $g, $where)
{
	return 'Section ' . trim((string) $sec) . ' has ' . ($g === 'M' ? 'women' : 'men') . ($where === 'this course' ? ' in this course.' : ' in ' . $where . ' at the same time.');
}

/** The "other side is here" warning for putting a person of side $g into place $place of shared section $sec, as a
 *  sentence, or '' (also '' when the section is not shared). $prev_sec / $prev_place = where the person is now.
 *  Room level first: people of the other side in another place of the same room (a man and a woman in one room),
 *  asked even when moving inside the section but not when moving inside that same room. Then section level,
 *  only when entering the section (from another section or from no place). */
function dh_rooms_mix_warning($course_id, $sec, $place, $g, $prev_sec, $prev_place)
{
	$centre = db_query("select c_center from dh_course where c_id = :c", array(':c' => (int) $course_id))->fetchField();
	if (!$centre || !dh_rooms_is_shared(dh_rooms_shared_sections($centre), $sec))
		return '';
	$sec = trim((string) $sec);
	$place = trim((string) $place);
	$entering = trim((string) $prev_place) === '' || mb_strtoupper(trim((string) $prev_sec)) !== mb_strtoupper($sec);
	$room = db_query("select r.cr_id, r.cr_name from dh_center_room r join dh_center_room_place p on p.crp_room = r.cr_id
		where r.cr_center = :c and r.cr_gender = 'B' and r.cr_section = :s and r.cr_deleted = 0 and p.crp_deleted = 0 and p.crp_name = :p limit 1",
		array(':c' => (int) $centre, ':s' => $sec, ':p' => $place))->fetchAssoc();
	if ($room)
	{
		$all = db_query("select crp_name from dh_center_room_place where crp_room = :r and crp_deleted = 0", array(':r' => $room['cr_id']))->fetchCol();
		$in_room = !$entering && in_array(mb_strtoupper(trim((string) $prev_place)), array_map('mb_strtoupper', $all), TRUE);
		$others = array_values(array_filter($all, function ($x) use ($place) { return mb_strtoupper($x) !== mb_strtoupper($place); }));
		if (!$in_room && $others)
		{
			$courses = array_merge(array((int) $course_id), array_map('intval', dh_rooms_overlapping_courses($course_id)));
			$hit = db_query("select a.a_course from dh_applicant a join dh_applicant_attended aa on aa.aa_applicant = a.a_id
				where a.a_course in (:cs) and a.a_attended = 1 and a.a_gender in ('M', 'F') and a.a_gender <> :g and trim(aa.aa_section) = :s and trim(aa.aa_acco) in (:pl)
				order by (a.a_course = :this) desc limit 1", array(':cs' => $courses, ':g' => $g, ':s' => $sec, ':pl' => $others, ':this' => (int) $course_id))->fetchField();
			if ($hit)
				return 'Room ' . $room['cr_name'] . ' has ' . ($g === 'M' ? 'women' : 'men')
					. ((int) $hit === (int) $course_id ? ' in this course.' : ' in ' . dh_rooms_course_label($hit) . ' at the same time.');
		}
	}
	if ($entering && ($where = dh_rooms_shared_mix($course_id, $sec, $g)) !== '')
		return dh_rooms_mix_sentence($sec, $g, $where);
	return '';
}

/** " Place a man here anyway?" — the question after a mix warning. */
function dh_rooms_mix_ask($g)
{
	return ' Place ' . ($g === 'M' ? 'a man' : 'a woman') . ' here anyway?';
}

/** The full question for a mix warning alone: "... Place a man here anyway?" */
function dh_rooms_mix_question($sec, $g, $where)
{
	return dh_rooms_mix_sentence($sec, $g, $where) . dh_rooms_mix_ask($g);
}

/** Places a running or upcoming course of the centre uses: "G|section|place" => label of the first such course.
 *  A place of a shared section is also listed as "B|section|place" (whoever holds it). */
function dh_rooms_places_in_use($centre_id)
{
	$used = array();
	$labels = array();
	$shared = dh_rooms_shared_sections($centre_id);
	foreach (db_query("select a.a_course, a.a_gender, TRIM(aa.aa_section) as aa_section, TRIM(aa.aa_acco) as aa_acco from dh_course c join dh_applicant a on a.a_course = c.c_id
		join dh_applicant_attended aa on aa.aa_applicant = a.a_id where c.c_center = :c and c.c_deleted = 0 and c.c_end >= curdate() and a.a_attended = 1 and trim(aa.aa_acco) <> ''
		order by c.c_start", array(':c' => (int) $centre_id)) as $r)
	{
		if (!isset($labels[$r->a_course]))
			$labels[$r->a_course] = dh_rooms_course_label($r->a_course);
		$keys = array(strtoupper($r->a_gender) . '|' . $r->aa_section . '|' . $r->aa_acco);
		if (dh_rooms_is_shared($shared, $r->aa_section))
			$keys[] = 'B|' . $r->aa_section . '|' . $r->aa_acco;
		foreach ($keys as $k)
			if (!isset($used[$k]))
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
