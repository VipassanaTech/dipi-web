<?php
/**
 * Rooms conversion: turns a centre's typed list (dh_center_setting_acco, comma separated places per
 * gender + section) into rooms and places. Places like 12A, 12B (or 12/1, 12-A, 12(L)) that share a room
 * number become one room "12"; every other place is a one-place room with its own name.
 */

/** The room a place belongs to when it is "<number><separator?><letter>", "<number><sep><1-2 digits>" or "<number>(<x>)". Else ''. */
function dh_rooms_room_key($place)
{
	$p = trim((string) $place);
	if (preg_match('/^([0-9]+)\s*[-\/. ]?\s*[A-Za-z]$/', $p, $m))
		return $m[1];
	if (preg_match('/^([0-9]+)\s*[-\/.]\s*[0-9]{1,2}$/', $p, $m))
		return $m[1];
	if (preg_match('/^([0-9]+)\s*\(\s*[A-Za-z0-9]{1,3}\s*\)$/', $p, $m))
		return $m[1];
	return '';
}

/** One section's typed places → rooms (typed order kept). Empty values dropped; repeats (case/spaces ignored) dropped and counted. */
function dh_rooms_group_places($places)
{
	$seen = array();
	$list = array();
	$dups = 0;
	foreach ($places as $p)
	{
		$p = trim((string) $p);
		if ($p === '')
			continue;
		$k = strtolower($p);
		if (isset($seen[$k])) { $dups++; continue; }
		$seen[$k] = 1;
		$list[] = $p;
	}
	$count = array();
	foreach ($list as $p)
	{
		$key = dh_rooms_room_key($p);
		if ($key !== '')
			$count[$key] = (isset($count[$key]) ? $count[$key] : 0) + 1;
	}
	$rooms = array();
	$at = array();
	foreach ($list as $p)
	{
		$key = dh_rooms_room_key($p);
		if ($key !== '' && $count[$key] >= 2)
		{
			if (!isset($at[$key])) { $at[$key] = count($rooms); $rooms[] = array('name' => $key, 'places' => array()); }
			$rooms[$at[$key]]['places'][] = $p;
		}
		else
			$rooms[] = array('name' => $p, 'places' => array($p));
	}
	return array('rooms' => $rooms, 'duplicates' => $dups);
}

/**
 * Converts one centre. $apply FALSE = dry run (writes nothing). A centre that already has rooms is skipped.
 * Places longer than 15 characters (aa_acco's size) are skipped and counted. A place a running or upcoming
 * course uses but the list lacks is added as a one-place room marked "needs review"; places only past
 * courses (last 365 days) used are counted, not added.
 */
function dh_rooms_convert_centre($centre_id, $apply = FALSE, $uid = 0)
{
	$centre_id = (int) $centre_id;
	$rep = array('centre' => $centre_id, 'skipped' => '', 'rooms' => 0, 'shared_rooms' => 0, 'places' => 0, 'duplicates' => 0, 'too_long' => 0,
		'review_added' => 0, 'past_only_missing' => 0);
	if (dh_rooms_centre_has_rooms($centre_id))
	{
		$rep['skipped'] = 'already has rooms';
		return $rep;
	}
	$plan = array();
	$known = array();
	foreach (db_query("select csa_gender, csa_section, csa_room from dh_center_setting_acco where csa_center = :c and csa_deleted = 0 order by csa_id",
		array(':c' => $centre_id)) as $r)
	{
		$g = strtoupper($r->csa_gender);
		$sec = trim((string) $r->csa_section);
		$grp = dh_rooms_group_places(explode(',', (string) $r->csa_room));
		$rep['duplicates'] += $grp['duplicates'];
		foreach ($grp['rooms'] as $room)
		{
			$fresh = array();
			foreach ($room['places'] as $p)
			{
				if (mb_strlen($p) > 15) { $rep['too_long']++; continue; }
				$k = $g . '|' . $sec . '|' . strtolower($p);
				if (isset($known[$k])) { $rep['duplicates']++; continue; }
				$known[$k] = 1;
				$fresh[] = $p;
			}
			if ($fresh)
				$plan[$g . '|' . $sec][] = array('name' => mb_substr($room['name'], 0, 40), 'places' => $fresh, 'review' => 0);
		}
	}
	foreach (dh_rooms_places_in_use($centre_id) as $k => $label)
	{
		list($g, $sec, $p) = explode('|', $k, 3);
		$lk = $g . '|' . $sec . '|' . strtolower($p);
		if (isset($known[$lk]))
			continue;
		$known[$lk] = 1;
		$plan[$g . '|' . $sec][] = array('name' => $p, 'places' => array($p), 'review' => 1);
		$rep['review_added']++;
	}
	foreach (db_query("select distinct a.a_gender, aa.aa_section, aa.aa_acco from dh_applicant a join dh_applicant_attended aa on aa.aa_applicant = a.a_id
		join dh_course c on c.c_id = a.a_course where a.a_center = :c and a.a_attended = 1 and aa.aa_acco <> ''
		and c.c_end < curdate() and c.c_end >= curdate() - interval 365 day", array(':c' => $centre_id)) as $r)
		if (!isset($known[strtoupper($r->a_gender) . '|' . $r->aa_section . '|' . strtolower($r->aa_acco)]))
			$rep['past_only_missing']++;
	foreach ($plan as $rooms)
		foreach ($rooms as $room)
		{
			$rep['rooms']++;
			$rep['places'] += count($room['places']);
			if (count($room['places']) > 1)
				$rep['shared_rooms']++;
		}
	if (!$apply)
		return $rep;
	$now = date('Y-m-d H:i:s');
	$txn = db_transaction();
	foreach ($plan as $gs => $rooms)
	{
		list($g, $sec) = explode('|', $gs, 2);
		foreach ($rooms as $w => $room)
		{
			$rid = db_insert('dh_center_room')->fields(array('cr_center' => $centre_id, 'cr_gender' => $g, 'cr_section' => $sec, 'cr_name' => $room['name'],
				'cr_toilet' => 'unset', 'cr_weight' => $w, 'cr_review' => $room['review'], 'cr_deleted' => 0, 'cr_created' => $now, 'cr_created_by' => (int) $uid,
				'cr_updated' => $now, 'cr_updated_by' => (int) $uid))->execute();
			foreach ($room['places'] as $pw => $p)
				db_insert('dh_center_room_place')->fields(array('crp_room' => $rid, 'crp_name' => $p, 'crp_weight' => $pw, 'crp_deleted' => 0))->execute();
		}
	}
	dh_rooms_log($centre_id, 'create', array('converted' => array(NULL, $rep['rooms'] . ' rooms, ' . $rep['places'] . ' places from the old room list')));
	return $rep;
}
