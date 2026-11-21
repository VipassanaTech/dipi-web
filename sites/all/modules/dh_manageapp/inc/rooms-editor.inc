<?php
/**
 * Rooms page of a centre: actions on the room list (dh_rooms_action) and its JSON endpoints.
 * Routes: centre/%centre_id/rooms (page), .../rooms/data (JSON), .../rooms/save (POST JSON, token dh-rooms).
 */

function _dh_rooms_err($msg) { return array('ok' => FALSE, 'msg' => $msg); }

/** The room (non-deleted) if it belongs to the centre, else NULL. */
function _dh_rooms_room($centre_id, $id)
{
	$r = db_query("select * from dh_center_room where cr_id = :r and cr_deleted = 0", array(':r' => (int) $id))->fetchAssoc();
	return ($r && (int) $r['cr_center'] === (int) $centre_id) ? $r : NULL;
}

function _dh_rooms_places($room_id)
{
	return db_query("select crp_name from dh_center_room_place where crp_room = :r and crp_deleted = 0 order by crp_weight, crp_id", array(':r' => (int) $room_id))->fetchCol();
}

/** The room (other than $except_room) that already holds place $p in this centre/gender/section, or NULL. */
function _dh_rooms_place_owner($centre_id, $g, $sec, $p, $except_room = 0)
{
	return db_query("select r.cr_name from dh_center_room r join dh_center_room_place p on p.crp_room = r.cr_id
		where r.cr_center = :c and r.cr_gender = :g and r.cr_section = :s and r.cr_deleted = 0 and p.crp_deleted = 0 and p.crp_name = :p and r.cr_id <> :x limit 1",
		array(':c' => (int) $centre_id, ':g' => $g, ':s' => $sec, ':p' => $p, ':x' => (int) $except_room))->fetchField();
}

/** "Place X is used by <course>..." when a place of this centre/gender/section is in use, else ''.
 *  $in_use comes from dh_rooms_action(): upper-cased key => array(course label, original key). Letter case is ignored, as the DB does. */
function _dh_rooms_in_use_msg($in_use, $g, $sec, $p)
{
	$k = mb_strtoupper($g . '|' . $sec . '|' . $p);
	return isset($in_use[$k]) ? 'Place ' . $p . ' is used by ' . $in_use[$k][0] . ". It can't be changed until that course is over." : '';
}

/** TRUE when the centre has no rooms yet but still has a typed room list (old accommodation settings) to bring in. */
function dh_rooms_has_old_list($centre_id)
{
	if (dh_rooms_centre_has_rooms($centre_id))
		return FALSE;
	return (bool) db_query("select 1 from dh_center_setting_acco where csa_center = :c and csa_deleted = 0 and trim(csa_room) <> '' limit 1", array(':c' => (int) $centre_id))->fetchField();
}

/** The section's spelling already used by this centre and side (SQL match ignores letter case), else $sec as typed. */
function _dh_rooms_section_spelling($centre_id, $g, $sec)
{
	$e = db_query("select cr_section from dh_center_room where cr_center = :c and cr_gender = :g and cr_section = :s and cr_deleted = 0 order by cr_id limit 1",
		array(':c' => (int) $centre_id, ':g' => $g, ':s' => $sec))->fetchField();
	return ($e === FALSE || $e === NULL) ? $sec : $e;
}

/** The next free weight in a section: one more than the largest there (0 for an empty section). */
function _dh_rooms_next_weight($centre_id, $g, $sec)
{
	$m = db_query("select max(cr_weight) from dh_center_room where cr_center = :c and cr_gender = :g and cr_section = :s and cr_deleted = 0",
		array(':c' => (int) $centre_id, ':g' => $g, ':s' => $sec))->fetchField();
	return ($m === NULL || $m === FALSE) ? 0 : (int) $m + 1;
}

/** TRUE when $p is one of $list, ignoring letter case. */
function _dh_rooms_has_place($list, $p)
{
	foreach ($list as $x)
		if (mb_strtolower($x) === mb_strtolower($p))
			return TRUE;
	return FALSE;
}

/** The label of a toilet key; the raw key when it is not a known one. */
function _dh_rooms_toilet_label($key)
{
	$t = dh_room_toilet_types();
	return isset($t[$key]) ? $t[$key] : (string) $key;
}

function _dh_rooms_write_places($room_id, $places)
{
	db_update('dh_center_room_place')->fields(array('crp_deleted' => 1))->condition('crp_room', $room_id)->execute();
	foreach (array_values($places) as $w => $p)
		db_insert('dh_center_room_place')->fields(array('crp_room' => $room_id, 'crp_name' => $p, 'crp_weight' => $w, 'crp_deleted' => 0))->execute();
}

function dh_rooms_action($centre_id, $in, $uid)
{
	$centre_id = (int) $centre_id;
	$op = isset($in['op']) ? (string) $in['op'] : '';
	$toilets = dh_room_toilet_types();
	$now = date('Y-m-d H:i:s');
	$in_use = array();
	foreach (dh_rooms_places_in_use($centre_id) as $k => $label)
		$in_use[mb_strtoupper($k)] = array($label, $k);
	$g = isset($in['gender']) ? strtoupper((string) $in['gender']) : '';
	$ids = (isset($in['ids']) && is_array($in['ids'])) ? array_values(array_unique(array_map('intval', $in['ids']))) : array();

	switch ($op)
	{
	case 'save_room':
		$id = (int) (isset($in['id']) ? $in['id'] : 0);
		$sec = trim((string) (isset($in['section']) ? $in['section'] : ''));
		$name = trim((string) (isset($in['name']) ? $in['name'] : ''));
		$toilet = (string) (isset($in['toilet']) ? $in['toilet'] : 'unset');
		$places = array();
		foreach ((isset($in['places']) && is_array($in['places'])) ? $in['places'] : array() as $p)
			if (($p = trim((string) $p)) !== '' && !in_array(strtolower($p), array_map('strtolower', $places), TRUE))
				$places[] = $p;
		if ($g !== 'M' && $g !== 'F') return _dh_rooms_err('Choose Men or Women.');
		if ($sec === '' || mb_strlen($sec) > 10) return _dh_rooms_err('Section must be 1 to 10 letters.');
		if (strpos($sec, '|') !== FALSE) return _dh_rooms_err('Section can\'t contain "|".');
		if (!$id && dh_rooms_has_old_list($centre_id)) return _dh_rooms_err('This centre still uses its old room list. Use "Bring in the old room list" first.');
		$sec = _dh_rooms_section_spelling($centre_id, $g, $sec);
		if ($name === '' || mb_strlen($name) > 40) return _dh_rooms_err('Room name must be 1 to 40 letters.');
		if (!isset($toilets[$toilet])) return _dh_rooms_err('Choose a toilet type.');
		if (!$places || count($places) > 30) return _dh_rooms_err('A room needs 1 to 30 places.');
		foreach ($places as $p)
		{
			if (mb_strlen($p) > 15) return _dh_rooms_err('Place ' . $p . ' is too long (15 letters at most).');
			if (($owner = _dh_rooms_place_owner($centre_id, $g, $sec, $p, $id)) !== FALSE && $owner !== NULL) return _dh_rooms_err('Place ' . $p . ' is already in Room ' . $owner . '.');
		}
		if ($id)
		{
			$old = _dh_rooms_room($centre_id, $id);
			if (!$old) return _dh_rooms_err('This room is not in this centre.');
			$old_places = _dh_rooms_places($id);
			foreach ($old_places as $p)
				if ((mb_strtoupper($sec) !== mb_strtoupper($old['cr_section']) || $g !== $old['cr_gender'] || !in_array($p, $places, TRUE)) && ($m = _dh_rooms_in_use_msg($in_use, $old['cr_gender'], $old['cr_section'], $p)) !== '')
					return _dh_rooms_err($m);
			$txn = db_transaction();
			db_update('dh_center_room')->fields(array('cr_gender' => $g, 'cr_section' => $sec, 'cr_name' => $name, 'cr_toilet' => $toilet,
				'cr_updated' => $now, 'cr_updated_by' => (int) $uid))->condition('cr_id', $id)->execute();
			if ($old_places !== $places)
				_dh_rooms_write_places($id, $places);
			$ch = array();
			if ($old['cr_name'] !== $name) $ch['room'] = array($old['cr_name'], $name);
			if ($old['cr_section'] !== $sec || $old['cr_gender'] !== $g) $ch['section'] = array($old['cr_gender'] . ' ' . $old['cr_section'], $g . ' ' . $sec);
			if ($old['cr_toilet'] !== $toilet) $ch['toilet'] = array(_dh_rooms_toilet_label($old['cr_toilet']), $toilets[$toilet]);
			if ($old_places !== $places) $ch['places'] = array(implode(', ', $old_places), implode(', ', $places));
			dh_rooms_log($centre_id, 'update', $ch, $id);
		}
		else
		{
			$txn = db_transaction();
			$id = db_insert('dh_center_room')->fields(array('cr_center' => $centre_id, 'cr_gender' => $g, 'cr_section' => $sec, 'cr_name' => $name, 'cr_toilet' => $toilet,
				'cr_weight' => _dh_rooms_next_weight($centre_id, $g, $sec), 'cr_review' => 0, 'cr_deleted' => 0, 'cr_created' => $now, 'cr_created_by' => (int) $uid, 'cr_updated' => $now, 'cr_updated_by' => (int) $uid))->execute();
			_dh_rooms_write_places($id, $places);
			dh_rooms_log($centre_id, 'create', array('room' => array(NULL, $g . ' ' . $sec . ' ' . $name . ': ' . implode(', ', $places) . ' (' . $toilets[$toilet] . ')')), $id);
		}
		return array('ok' => TRUE, 'msg' => 'Saved.');

	case 'add_many':
		$sec = trim((string) (isset($in['section']) ? $in['section'] : ''));
		$from = (int) (isset($in['from']) ? $in['from'] : 0);
		$to = (int) (isset($in['to']) ? $in['to'] : 0);
		$toilet = (string) (isset($in['toilet']) ? $in['toilet'] : 'unset');
		$suf = array();
		foreach ((isset($in['suffixes']) && is_array($in['suffixes'])) ? $in['suffixes'] : array() as $s)
			if (($s = trim((string) $s)) !== '')
				$suf[] = $s;
		if ($g !== 'M' && $g !== 'F') return _dh_rooms_err('Choose Men or Women.');
		if ($sec === '' || mb_strlen($sec) > 10) return _dh_rooms_err('Section must be 1 to 10 letters.');
		if (strpos($sec, '|') !== FALSE) return _dh_rooms_err('Section can\'t contain "|".');
		if (dh_rooms_has_old_list($centre_id)) return _dh_rooms_err('This centre still uses its old room list. Use "Bring in the old room list" first.');
		$sec = _dh_rooms_section_spelling($centre_id, $g, $sec);
		if (!isset($toilets[$toilet])) return _dh_rooms_err('Choose a toilet type.');
		if ($from < 0 || $to < $from) return _dh_rooms_err('"To" must be the same as or after "From".');
		if ($to - $from + 1 > 200) return _dh_rooms_err('Add at most 200 rooms at once.');
		$plan = array();
		$seen = array();
		for ($n = $from; $n <= $to; $n++)
		{
			$places = $suf ? array_map(function ($s) use ($n) { return $n . $s; }, $suf) : array((string) $n);
			foreach ($places as $p)
			{
				if (mb_strlen($p) > 15) return _dh_rooms_err('Place ' . $p . ' is too long (15 letters at most).');
				if (($owner = _dh_rooms_place_owner($centre_id, $g, $sec, $p)) !== FALSE && $owner !== NULL) return _dh_rooms_err('Place ' . $p . ' is already in Room ' . $owner . '.');
				if (isset($seen[mb_strtolower($p)])) return _dh_rooms_err('Place ' . $p . ' is already in Room ' . $seen[mb_strtolower($p)] . '.');
				$seen[mb_strtolower($p)] = (string) $n;
			}
			$plan[(string) $n] = $places;
		}
		$txn = db_transaction();
		$weight = _dh_rooms_next_weight($centre_id, $g, $sec);
		foreach ($plan as $name => $places)
		{
			$id = db_insert('dh_center_room')->fields(array('cr_center' => $centre_id, 'cr_gender' => $g, 'cr_section' => $sec, 'cr_name' => (string) $name, 'cr_toilet' => $toilet,
				'cr_weight' => $weight++, 'cr_review' => 0, 'cr_deleted' => 0, 'cr_created' => $now, 'cr_created_by' => (int) $uid, 'cr_updated' => $now, 'cr_updated_by' => (int) $uid))->execute();
			_dh_rooms_write_places($id, $places);
		}
		dh_rooms_log($centre_id, 'create', array('room' => array(NULL, count($plan) . ' rooms ' . $from . '-' . $to . ' in ' . $g . ' ' . $sec . ' (' . $toilets[$toilet] . ')')));
		return array('ok' => TRUE, 'msg' => count($plan) . ' rooms added.');

	case 'import_old':
		if (dh_rooms_centre_has_rooms($centre_id)) return _dh_rooms_err('Rooms are already set up for this centre.');
		$rep = dh_rooms_convert_centre($centre_id, TRUE, (int) $uid);
		return array('ok' => TRUE, 'msg' => $rep['rooms'] . ' rooms and ' . $rep['places'] . ' places brought in from the old room list. Please check the toilet types.');

	case 'set_toilet':
	case 'clear_review':
		if (!$ids) return _dh_rooms_err('Choose one or more rooms first.');
		$toilet = (string) (isset($in['toilet']) ? $in['toilet'] : '');
		if ($op === 'set_toilet' && !isset($toilets[$toilet])) return _dh_rooms_err('Choose a toilet type.');
		foreach ($ids as $id)
			if (!_dh_rooms_room($centre_id, $id)) return _dh_rooms_err('This room is not in this centre.');
		$txn = db_transaction();
		foreach ($ids as $id)
		{
			$old = _dh_rooms_room($centre_id, $id);
			if ($op === 'set_toilet')
			{
				db_update('dh_center_room')->fields(array('cr_toilet' => $toilet, 'cr_review' => 0, 'cr_updated' => $now, 'cr_updated_by' => (int) $uid))->condition('cr_id', $id)->execute();
				if ($old['cr_toilet'] !== $toilet)
					dh_rooms_log($centre_id, 'update', array('toilet' => array(_dh_rooms_toilet_label($old['cr_toilet']), $toilets[$toilet])), $id);
			}
			else
				db_update('dh_center_room')->fields(array('cr_review' => 0, 'cr_updated' => $now, 'cr_updated_by' => (int) $uid))->condition('cr_id', $id)->execute();
		}
		return array('ok' => TRUE, 'msg' => 'Saved.');

	case 'join':
		if (count($ids) < 2) return _dh_rooms_err('Choose two or more rooms to join.');
		$rooms = array();
		foreach ($ids as $id)
		{
			if (!($r = _dh_rooms_room($centre_id, $id))) return _dh_rooms_err('This room is not in this centre.');
			$rooms[] = $r;
		}
		foreach ($rooms as $r)
			if ($r['cr_gender'] !== $rooms[0]['cr_gender'] || $r['cr_section'] !== $rooms[0]['cr_section']) return _dh_rooms_err('Only rooms of the same section can be joined.');
		$all = array();
		foreach ($rooms as $r)
			$all = array_merge($all, _dh_rooms_places($r['cr_id']));
		if (count($all) > 30) return _dh_rooms_err('A room needs 1 to 30 places.');
		$txn = db_transaction();
		_dh_rooms_write_places($rooms[0]['cr_id'], $all);
		foreach (array_slice($rooms, 1) as $r)
		{
			db_update('dh_center_room')->fields(array('cr_deleted' => 1, 'cr_updated' => $now, 'cr_updated_by' => (int) $uid))->condition('cr_id', $r['cr_id'])->execute();
			db_update('dh_center_room_place')->fields(array('crp_deleted' => 1))->condition('crp_room', $r['cr_id'])->execute();
		}
		dh_rooms_log($centre_id, 'update', array('room' => array(implode(' + ', array_map(function ($r) { return $r['cr_name']; }, $rooms)), $rooms[0]['cr_name'] . ': ' . implode(', ', $all))), $rooms[0]['cr_id']);
		return array('ok' => TRUE, 'msg' => 'Rooms joined.');

	case 'split':
		$id = (int) (isset($in['id']) ? $in['id'] : 0);
		if (!($r = _dh_rooms_room($centre_id, $id))) return _dh_rooms_err('This room is not in this centre.');
		$places = _dh_rooms_places($id);
		if (count($places) < 2) return _dh_rooms_err('This room has only one place.');
		$txn = db_transaction();
		_dh_rooms_write_places($id, array($places[0]));
		db_update('dh_center_room')->fields(array('cr_name' => $places[0], 'cr_updated' => $now, 'cr_updated_by' => (int) $uid))->condition('cr_id', $id)->execute();
		foreach (array_slice($places, 1) as $p)
		{
			$nid = db_insert('dh_center_room')->fields(array('cr_center' => $centre_id, 'cr_gender' => $r['cr_gender'], 'cr_section' => $r['cr_section'], 'cr_name' => $p,
				'cr_toilet' => $r['cr_toilet'], 'cr_weight' => (int) $r['cr_weight'], 'cr_review' => 0, 'cr_deleted' => 0, 'cr_created' => $now, 'cr_created_by' => (int) $uid,
				'cr_updated' => $now, 'cr_updated_by' => (int) $uid))->execute();
			_dh_rooms_write_places($nid, array($p));
		}
		dh_rooms_log($centre_id, 'update', array('room' => array($r['cr_name'] . ': ' . implode(', ', $places), 'split into ' . count($places) . ' rooms')), $id);
		return array('ok' => TRUE, 'msg' => 'Room split.');

	case 'remove':
		$id = (int) (isset($in['id']) ? $in['id'] : 0);
		if (!($r = _dh_rooms_room($centre_id, $id))) return _dh_rooms_err('This room is not in this centre.');
		$places = _dh_rooms_places($id);
		foreach ($places as $p)
			if (($m = _dh_rooms_in_use_msg($in_use, $r['cr_gender'], $r['cr_section'], $p)) !== '') return _dh_rooms_err($m);
		$txn = db_transaction();
		db_update('dh_center_room')->fields(array('cr_deleted' => 1, 'cr_updated' => $now, 'cr_updated_by' => (int) $uid))->condition('cr_id', $id)->execute();
		db_update('dh_center_room_place')->fields(array('crp_deleted' => 1))->condition('crp_room', $id)->execute();
		dh_rooms_log($centre_id, 'delete', array('room' => array($r['cr_gender'] . ' ' . $r['cr_section'] . ' ' . $r['cr_name'] . ': ' . implode(', ', $places), NULL)), $id);
		return array('ok' => TRUE, 'msg' => 'Room removed.');

	case 'rename_section':
		$from = trim((string) (isset($in['from']) ? $in['from'] : ''));
		$to = trim((string) (isset($in['to']) ? $in['to'] : ''));
		if ($g !== 'M' && $g !== 'F') return _dh_rooms_err('Choose Men or Women.');
		if ($to === '' || mb_strlen($to) > 10) return _dh_rooms_err('Section must be 1 to 10 letters.');
		if (strpos($to, '|') !== FALSE) return _dh_rooms_err('Section can\'t contain "|".');
		if (db_query("select 1 from dh_center_room where cr_center = :c and cr_gender = :g and cr_section = :s and cr_deleted = 0 limit 1",
			array(':c' => $centre_id, ':g' => $g, ':s' => $to))->fetchField()) return _dh_rooms_err('Section ' . $to . ' already exists.');
		foreach ($in_use as $v)
		{
			list($kg, $ks, $kp) = explode('|', $v[1], 3);
			if ($kg === $g && mb_strtoupper($ks) === mb_strtoupper($from)) return _dh_rooms_err(_dh_rooms_in_use_msg($in_use, $kg, $ks, $kp));
		}
		db_update('dh_center_room')->fields(array('cr_section' => $to, 'cr_updated' => $now, 'cr_updated_by' => (int) $uid))
			->condition('cr_center', $centre_id)->condition('cr_gender', $g)->condition('cr_section', $from)->condition('cr_deleted', 0)->execute();
		dh_rooms_log($centre_id, 'update', array('section' => array($g . ' ' . $from, $g . ' ' . $to)));
		return array('ok' => TRUE, 'msg' => 'Section renamed.');
	}
	return _dh_rooms_err('Unknown action.');
}

/** GET JSON: the centre's rooms, places in use, toilet types. */
function dh_rooms_data_page($centre_id)
{
	drupal_json_output(array('ok' => TRUE, 'rooms' => dh_rooms_load($centre_id), 'in_use' => array_keys(dh_rooms_places_in_use($centre_id)), 'toilets' => dh_room_toilet_types(), 'old_list' => dh_rooms_has_old_list($centre_id)));
	drupal_exit();
}

/** POST JSON: one action, then the fresh room list. */
function dh_rooms_save_page($centre_id)
{
	global $user;
	if ($_SERVER['REQUEST_METHOD'] !== 'POST' || !isset($_POST['token']) || !drupal_valid_token($_POST['token'], 'dh-rooms'))
	{
		drupal_json_output(_dh_rooms_err('Your page is out of date. Please reload it.'));
		drupal_exit();
	}
	$out = dh_rooms_action($centre_id, $_POST, $user->uid);
	$out['rooms'] = dh_rooms_load($centre_id);
	$out['in_use'] = array_keys(dh_rooms_places_in_use($centre_id));
	$out['old_list'] = dh_rooms_has_old_list($centre_id);
	drupal_json_output($out);
	drupal_exit();
}

/** The Rooms page: sections, rooms and places of one centre, edited in place (js/rooms-editor.js). */
function dh_rooms_page($centre_id)
{
	$name = (string) db_query("select c_name from dh_center where c_id = :c", array(':c' => (int) $centre_id))->fetchField();
	drupal_set_title('Rooms');
	$path = drupal_get_path('module', 'dh_manageapp');
	drupal_add_css($path . '/css/rooms.css');
	drupal_add_js(array('dhRooms' => array('centre' => (int) $centre_id, 'dataUrl' => url('centre/' . (int) $centre_id . '/rooms/data'),
		'saveUrl' => url('centre/' . (int) $centre_id . '/rooms/save'), 'token' => drupal_get_token('dh-rooms'), 'toilets' => dh_room_toilet_types())), 'setting');
	drupal_add_js($path . '/js/rooms-editor.js');
	return '<div class="dh-rooms"><p>' . l('Back to Centre settings', 'centre/' . (int) $centre_id . '/edit') . ' | ' . l('Back to Dashboard', 'centre/' . (int) $centre_id) . '</p>'
		. '<h2>' . check_plain($name) . '</h2><div id="dh-rooms-app"><p>Loading rooms…</p></div></div>';
}
