<?php

/**
 * @file
 * Application tagging: per-centre tags (dh_tag) assigned to applications
 * (dh_application_tag). Tag CRUD + assign/unassign backend + admin page.
 * Search integration lives in search.inc; the dashboard block in
 * center-dashboard.inc.
 */

/* ---------------------------------------------------------------------------
 * Access helpers
 * ------------------------------------------------------------------------- */

/**
 * Does the current user hold any tag-related permission?
 */
function _dh_tag_has_any_perm()
{
	return user_access('create tag') || user_access('edit own tag') || user_access('edit all tag')
		|| user_access('delete own tag') || user_access('delete all tag') || user_access('use tag');
}

/**
 * Does the current user have access to this centre (all-centres or mapped)?
 */
function _dh_tag_center_access($centre_id)
{
	if (user_access('access all centres')) {
		return TRUE;
	}
	global $user;
	return (bool) db_query(
		"SELECT 1 FROM dh_user_center WHERE uc_user = :u AND uc_center = :c AND uc_deleted = 0",
		array(':u' => $user->uid, ':c' => $centre_id)
	)->fetchField();
}

/**
 * Is the current user an active AT confirmed to serve a live course in this
 * centre? Such an AT may manage their OWN private tags (t_owner = their uid)
 * without holding centre access. Uses the dh_atportal helper when available.
 */
function _dh_tag_at_serves_centre($centre_id)
{
	if (!function_exists('_at_view_teacher_id')) {
		return FALSE;
	}
	$tid = _at_view_teacher_id();
	if (!$tid) {
		return FALSE;
	}
	return (bool) db_query_range(
		"SELECT 1 FROM dh_course_teacher ct JOIN dh_course co ON co.c_id = ct.ct_course
		 WHERE ct.ct_teacher = :t AND co.c_center = :c AND ct.ct_status = 'Confirmed'
		   AND ct.ct_type IN ('Conducting','Assisting') AND co.c_deleted = 0 AND co.c_cancelled = 0",
		0, 1, array(':t' => $tid, ':c' => $centre_id)
	)->fetchField();
}

/**
 * Menu access: the tag admin page / CRUD handler. Centre staff (any tag perm +
 * centre access) OR an AT who serves this centre (for creating private tags).
 */
function _dh_tag_admin_access()
{
	$centre = arg(1);
	if ($centre === NULL) {
		return FALSE;
	}
	if (_dh_tag_has_any_perm() && _dh_tag_center_access($centre)) {
		return TRUE;
	}
	return user_access('use tag') && _dh_tag_at_serves_centre($centre);
}

/**
 * Menu access: assign/unassign endpoints. 'use tag' + (centre access OR the
 * user is an AT serving this centre, for their own private tags).
 */
function _dh_tag_use_access()
{
	$centre = arg(1);
	if ($centre === NULL || !user_access('use tag')) {
		return FALSE;
	}
	return _dh_tag_center_access($centre) || _dh_tag_at_serves_centre($centre);
}

/**
 * Can the current user edit/delete this specific tag ($mode = 'edit'|'delete')?
 * "all" permission covers any tag; "own" covers tags the user created.
 */
function _dh_tag_can_modify($tag_row, $mode)
{
	global $user;
	if (user_access($mode . ' all tag')) {
		return TRUE;
	}
	return user_access($mode . ' own tag') && $tag_row && $tag_row['t_created_by'] == $user->uid;
}

/* ---------------------------------------------------------------------------
 * Data helpers
 * ------------------------------------------------------------------------- */

/**
 * Preset colour palette for tags (value stored in t_color).
 */
function _dh_tag_palette()
{
	return array('#607d8b', '#e74c3c', '#e67e22', '#f1c40f', '#2ecc71', '#1abc9c', '#3498db', '#9b59b6', '#34495e', '#795548');
}

/**
 * Readable chip text colour (dark on light backgrounds, white on dark) using
 * WCAG relative luminance. Returns '#222' or '#fff'.
 */
function _dh_tag_text_color($hex)
{
	$hex = ltrim((string) $hex, '#');
	if (strlen($hex) === 3) {
		$hex = $hex[0] . $hex[0] . $hex[1] . $hex[1] . $hex[2] . $hex[2];
	}
	if (strlen($hex) !== 6) {
		return '#fff';
	}
	$lin = function ($h) {
		$c = hexdec($h) / 255;
		return $c <= 0.03928 ? $c / 12.92 : pow(($c + 0.055) / 1.055, 2.4);
	};
	$l = 0.2126 * $lin(substr($hex, 0, 2)) + 0.7152 * $lin(substr($hex, 2, 2)) + 0.0722 * $lin(substr($hex, 4, 2));
	return $l > 0.179 ? '#222' : '#fff';
}

/**
 * "Soft tint" chip style for a tag colour: a pale fill (colour mixed toward
 * white), text in a dark shade of the same hue, and a light same-hue border.
 * Reads well for any picked colour. Returns a CSS declaration fragment
 * (background/color/border) with no leading/trailing semicolon.
 */
function _dh_tag_chip_style($hex)
{
	$hex = ltrim((string) $hex, '#');
	if (strlen($hex) === 3) {
		$hex = $hex[0] . $hex[0] . $hex[1] . $hex[1] . $hex[2] . $hex[2];
	}
	if (strlen($hex) !== 6) {
		$hex = '607d8b';
	}
	$r = hexdec(substr($hex, 0, 2));
	$g = hexdec(substr($hex, 2, 2));
	$b = hexdec(substr($hex, 4, 2));
	$mix = function ($a, $t) {
		return round($a + (255 - $a) * $t);
	};
	$bg = sprintf('#%02x%02x%02x', $mix($r, 0.85), $mix($g, 0.85), $mix($b, 0.85));
	$bd = sprintf('#%02x%02x%02x', $mix($r, 0.45), $mix($g, 0.45), $mix($b, 0.45));
	$lin = function ($v) {
		$v = $v / 255;
		return $v <= 0.03928 ? $v / 12.92 : pow(($v + 0.055) / 1.055, 2.4);
	};
	$tr = $r;
	$tg = $g;
	$tb = $b;
	for ($i = 0; $i < 40; $i++) {
		$l = 0.2126 * $lin($tr) + 0.7152 * $lin($tg) + 0.0722 * $lin($tb);
		if ($l <= 0.10) {
			break;
		}
		$tr *= 0.92;
		$tg *= 0.92;
		$tb *= 0.92;
	}
	$fg = sprintf('#%02x%02x%02x', round($tr), round($tg), round($tb));
	return 'background:' . $bg . ';color:' . $fg . ';border:1px solid ' . $bd;
}

/**
 * Visibility filter for the tag scope. Centre context ($at_uid = 0) sees centre
 * tags (t_owner = 0) plus system tags; an AT context ($at_uid > 0) sees only
 * that AT's own private tags (t_owner = uid) plus system tags. $at_uid is an
 * int, inlined safely.
 */
function _dh_tag_scope_where($col, $at_uid)
{
	$at_uid = (int) $at_uid;
	if ($at_uid > 0) {
		return "({$col}t_owner = {$at_uid} OR {$col}t_system = 1)";
	}
	return "({$col}t_owner = 0 OR {$col}t_system = 1)";
}

/**
 * Tags for a centre visible in the given context, ordered by name.
 */
function _dh_tag_center_tags($centre_id, $at_uid = 0)
{
	$scope = _dh_tag_scope_where('', $at_uid);
	return db_query(
		"SELECT t_id, t_name, t_color, t_created_by, t_system, t_owner FROM dh_tag WHERE t_center = :c AND t_deleted = 0 AND $scope ORDER BY t_name",
		array(':c' => $centre_id)
	)->fetchAll(PDO::FETCH_ASSOC);
}

/**
 * Is a tag name already taken in this centre (case-insensitive, non-deleted)?
 * $exclude_id lets an edit keep its own name.
 */
function _dh_tag_name_exists($centre_id, $name, $exclude_id = 0, $owner = 0)
{
	return (bool) db_query_range(
		"SELECT 1 FROM dh_tag WHERE t_center = :c AND t_deleted = 0 AND t_name = :n AND t_id <> :x AND t_owner = :o",
		0, 1,
		array(':c' => $centre_id, ':n' => trim($name), ':x' => $exclude_id, ':o' => (int) $owner)
	)->fetchField();
}

/**
 * Tags currently on an application (non-deleted), for row/detail rendering.
 */
function _dh_tag_for_application($application_id, $at_uid = 0)
{
	$scope = _dh_tag_scope_where('t.', $at_uid);
	return db_query(
		"SELECT t.t_id, t.t_name, t.t_color, t.t_system, t.t_owner
		 FROM dh_application_tag at JOIN dh_tag t ON at.at_tag = t.t_id
		 WHERE at.at_application = :a AND at.at_deleted = 0 AND t.t_deleted = 0 AND $scope
		 ORDER BY t.t_name",
		array(':a' => $application_id)
	)->fetchAll(PDO::FETCH_ASSOC);
}

/**
 * Tag ids the given user has pinned (across centres).
 */
function _dh_tag_pinned_ids($uid)
{
	return db_query("SELECT tp_tag FROM dh_tag_pin WHERE tp_user = :u", array(':u' => $uid))->fetchCol();
}

/**
 * The user's pinned tags for a centre (non-deleted), ordered by name.
 */
function _dh_tag_pinned($uid, $centre_id)
{
	return db_query(
		"SELECT t.t_id, t.t_name, t.t_color
		 FROM dh_tag_pin p JOIN dh_tag t ON p.tp_tag = t.t_id
		 WHERE p.tp_user = :u AND t.t_center = :c AND t.t_deleted = 0
		 ORDER BY t.t_name",
		array(':u' => $uid, ':c' => $centre_id)
	)->fetchAll(PDO::FETCH_ASSOC);
}

/* ---------------------------------------------------------------------------
 * System tags ("AT Comments" / "Center Comment")
 *
 * Fixed per-centre system tags (t_system = 1, t_owner = 0) auto-applied to an
 * application when a review comment is added:
 *   - "AT Comments"    when the reviewer is a conducting/assisting AT of the
 *                       application's course.
 *   - "Center Comment" when the reviewer is centre team (anyone who is NOT the
 *                       course's AT).
 * Both are visible to everyone (centre + AT) and cannot be edited, deleted, or
 * manually (un)assigned by anyone (enforced in the CRUD/assign handlers).
 * ------------------------------------------------------------------------- */

define('DH_AT_COMMENTS_TAG', 'AT Comments');
define('DH_CENTER_COMMENT_TAG', 'Center Comment');

/**
 * Find (or lazily create) a per-centre system tag by name; returns t_id.
 */
function _dh_tag_system_id($centre_id, $name, $color)
{
	$id = db_query(
		"SELECT t_id FROM dh_tag WHERE t_center = :c AND t_system = 1 AND t_name = :n AND t_deleted = 0",
		array(':c' => $centre_id, ':n' => $name)
	)->fetchField();
	if ($id) {
		return $id;
	}
	return db_insert('dh_tag')->fields(array(
		't_center' => $centre_id,
		't_name' => $name,
		't_color' => $color,
		't_system' => 1,
		't_owner' => 0,
		't_created_by' => 0,
		't_updated_by' => 0,
	))->execute();
}

/**
 * Append a row to the tag-activity log (Phase 2). $action is 'add' | 'remove'.
 */
function _dh_tag_log($tag_id, $application_id, $action, $uid)
{
	db_insert('dh_application_tag_log')->fields(array(
		'atl_tag' => (int) $tag_id,
		'atl_application' => (int) $application_id,
		'atl_action' => $action,
		'atl_by' => (int) $uid,
		'atl_at' => date('Y-m-d H:i:s'),
	))->execute();
}

/**
 * Is a tag currently (non-deleted) assigned to an application?
 */
function _dh_tag_is_active($tag_id, $application_id)
{
	return (bool) db_query_range(
		"SELECT 1 FROM dh_application_tag WHERE at_tag = :t AND at_application = :a AND at_deleted = 0",
		0, 1, array(':t' => (int) $tag_id, ':a' => (int) $application_id)
	)->fetchField();
}

/**
 * Mark an application with a per-centre system tag (idempotent upsert). Logs an
 * 'add' only on a genuine new/reactivated assignment (not on a no-op re-apply).
 */
function _dh_tag_mark_system($application_id, $centre_id, $name, $color, $uid = 0)
{
	$tag_id = _dh_tag_system_id($centre_id, $name, $color);
	$was_active = _dh_tag_is_active($tag_id, $application_id);
	// Upsert: re-activate a soft-deleted assignment, else insert (UNIQUE at_tag,at_application).
	db_query(
		"INSERT INTO dh_application_tag (at_tag, at_application, at_created_by, at_created)
		 VALUES (:t, :a, :u, NOW())
		 ON DUPLICATE KEY UPDATE at_deleted = 0",
		array(':t' => $tag_id, ':a' => (int) $application_id, ':u' => (int) $uid)
	);
	if (!$was_active) {
		_dh_tag_log($tag_id, $application_id, 'add', $uid);
	}
}

/** "AT Comments": reviewer is a conducting/assisting AT of the course. */
function _dh_tag_mark_at_comment($application_id, $centre_id, $uid = 0)
{
	_dh_tag_mark_system($application_id, $centre_id, DH_AT_COMMENTS_TAG, '#16a085', $uid);
}

/** "Center Comment": reviewer is centre team (not the course's AT). */
function _dh_tag_mark_center_comment($application_id, $centre_id, $uid = 0)
{
	_dh_tag_mark_system($application_id, $centre_id, DH_CENTER_COMMENT_TAG, '#8e44ad', $uid);
}

/* ---------------------------------------------------------------------------
 * JSON handlers
 * ------------------------------------------------------------------------- */

function _dh_tag_json($data)
{
	drupal_json_output($data);
	drupal_exit();
}

/**
 * Tag CRUD handler: tags/%centre_id/handler  (POST op = list|create|update|delete).
 */
function dh_tags_handler($centre_id)
{
	$op = isset($_POST['op']) ? $_POST['op'] : (isset($_GET['op']) ? $_GET['op'] : 'list');
	global $user;

	// AT context: an AT serving this centre works only with their OWN private
	// tags (t_owner = uid). The client passes scope=at. Centre staff omit it and
	// work with centre tags (t_owner = 0).
	$at_uid = _dh_tag_at_serves_centre($centre_id) ? $user->uid : 0;
	$at_mode = (isset($_POST['scope']) && $_POST['scope'] === 'at') && $at_uid > 0;

	if ($op === 'list') {
		_dh_tag_json(array('status' => 1, 'tags' => _dh_tag_center_tags($centre_id, $at_mode ? $at_uid : 0)));
	}

	if ($op === 'create') {
		if ($at_mode) {
			$owner = $at_uid; // AT creates a private tag
		}
		else {
			if (!user_access('create tag')) {
				_dh_tag_json(array('status' => 0, 'msg' => t('You do not have permission to create tags.')));
			}
			$owner = 0;
		}
		$name = trim($_POST['name']);
		$color = in_array($_POST['color'], _dh_tag_palette()) ? $_POST['color'] : '#607d8b';
		if ($name === '') {
			_dh_tag_json(array('status' => 0, 'msg' => t('Tag name is required.')));
		}
		if (drupal_strlen($name) > 100) {
			_dh_tag_json(array('status' => 0, 'msg' => t('Tag name is too long (max 100).')));
		}
		if (_dh_tag_name_exists($centre_id, $name, 0, $owner)) {
			_dh_tag_json(array('status' => 0, 'msg' => t('A tag with that name already exists.')));
		}
		$id = db_insert('dh_tag')->fields(array(
			't_center' => $centre_id, 't_name' => $name, 't_color' => $color, 't_owner' => $owner,
			't_created_by' => $user->uid, 't_updated_by' => $user->uid,
		))->execute();
		_dh_tag_json(array('status' => 1, 'tag' => array('t_id' => $id, 't_name' => $name, 't_color' => $color, 't_owner' => $owner, 't_created_by' => $user->uid)));
	}

	if ($op === 'update') {
		$tag = db_query("SELECT * FROM dh_tag WHERE t_id = :id AND t_center = :c AND t_deleted = 0",
			array(':id' => $_POST['t_id'], ':c' => $centre_id))->fetchAssoc();
		if (!$tag) {
			_dh_tag_json(array('status' => 0, 'msg' => t('Tag not found.')));
		}
		if (!empty($tag['t_system'])) {
			_dh_tag_json(array('status' => 0, 'msg' => t('This is a system tag and cannot be edited.')));
		}
		if ($at_mode) {
			if ($tag['t_owner'] != $at_uid) {
				_dh_tag_json(array('status' => 0, 'msg' => t('You can only edit your own tags.')));
			}
		}
		else {
			if ($tag['t_owner'] != 0) {
				_dh_tag_json(array('status' => 0, 'msg' => t('This tag is private and cannot be edited here.')));
			}
			if (!_dh_tag_can_modify($tag, 'edit')) {
				_dh_tag_json(array('status' => 0, 'msg' => t('You do not have permission to edit this tag.')));
			}
		}
		$name = trim($_POST['name']);
		$color = in_array($_POST['color'], _dh_tag_palette()) ? $_POST['color'] : $tag['t_color'];
		if ($name === '') {
			_dh_tag_json(array('status' => 0, 'msg' => t('Tag name is required.')));
		}
		if (_dh_tag_name_exists($centre_id, $name, $tag['t_id'], $tag['t_owner'])) {
			_dh_tag_json(array('status' => 0, 'msg' => t('A tag with that name already exists.')));
		}
		db_update('dh_tag')->fields(array(
			't_name' => $name, 't_color' => $color, 't_updated' => date('Y-m-d H:i:s'), 't_updated_by' => $user->uid,
		))->condition('t_id', $tag['t_id'])->execute();
		_dh_tag_json(array('status' => 1, 'tag' => array('t_id' => $tag['t_id'], 't_name' => $name, 't_color' => $color)));
	}

	if ($op === 'delete') {
		$tag = db_query("SELECT * FROM dh_tag WHERE t_id = :id AND t_center = :c AND t_deleted = 0",
			array(':id' => $_POST['t_id'], ':c' => $centre_id))->fetchAssoc();
		if (!$tag) {
			_dh_tag_json(array('status' => 0, 'msg' => t('Tag not found.')));
		}
		if (!empty($tag['t_system'])) {
			_dh_tag_json(array('status' => 0, 'msg' => t('This is a system tag and cannot be deleted.')));
		}
		if ($at_mode) {
			if ($tag['t_owner'] != $at_uid) {
				_dh_tag_json(array('status' => 0, 'msg' => t('You can only delete your own tags.')));
			}
		}
		else {
			if ($tag['t_owner'] != 0) {
				_dh_tag_json(array('status' => 0, 'msg' => t('This tag is private and cannot be deleted here.')));
			}
			if (!_dh_tag_can_modify($tag, 'delete')) {
				_dh_tag_json(array('status' => 0, 'msg' => t('You do not have permission to delete this tag.')));
			}
		}
		// Soft-delete the tag; its assignments are hidden by the join filters.
		db_update('dh_tag')->fields(array('t_deleted' => 1, 't_updated' => date('Y-m-d H:i:s'), 't_updated_by' => $user->uid))
			->condition('t_id', $tag['t_id'])->execute();
		_dh_tag_json(array('status' => 1));
	}

	_dh_tag_json(array('status' => 0, 'msg' => t('Unknown operation.')));
}

/**
 * Assign/unassign handler: app-tag/%centre_id/handler
 * (POST op = assign|unassign|bulk|for_app). All require 'use tag' + centre access
 * (enforced by the menu access callback) and validate the tag belongs to the centre.
 */
function dh_app_tag_handler($centre_id)
{
	global $user;
	$op = isset($_POST['op']) ? $_POST['op'] : (isset($_GET['op']) ? $_GET['op'] : '');

	// AT context (scope=at): operate only on the AT's own private tags and only
	// on applications in their confirmed courses. Centre staff omit scope.
	$scope = isset($_POST['scope']) ? $_POST['scope'] : (isset($_GET['scope']) ? $_GET['scope'] : '');
	$at_uid = _dh_tag_at_serves_centre($centre_id) ? $user->uid : 0;
	$at_mode = ($scope === 'at') && $at_uid > 0;

	if ($op === 'for_app') {
		$aid = (int) (isset($_GET['aid']) ? $_GET['aid'] : $_POST['aid']);
		_dh_tag_json(array('status' => 1, 'tags' => _dh_tag_for_application($aid, $at_mode ? $at_uid : 0), 'all' => _dh_tag_center_tags($centre_id, $at_mode ? $at_uid : 0)));
	}

	// Validate the tag exists in this centre for the mutating ops.
	if (in_array($op, array('assign', 'unassign', 'bulk', 'pin', 'unpin'))) {
		$tag_id = (int) $_POST['tag'];
		$tag_row = db_query("SELECT t_system, t_owner FROM dh_tag WHERE t_id = :id AND t_center = :c AND t_deleted = 0",
			array(':id' => $tag_id, ':c' => $centre_id))->fetchAssoc();
		if (!$tag_row) {
			_dh_tag_json(array('status' => 0, 'msg' => t('Tag not found in this centre.')));
		}
		// System tags cannot be manually assigned or removed by anyone.
		if (!empty($tag_row['t_system']) && in_array($op, array('assign', 'unassign', 'bulk'))) {
			_dh_tag_json(array('status' => 0, 'msg' => t('This is a system tag and cannot be added or removed manually.')));
		}
		// Scope: an AT may only touch their OWN private tag; centre staff may only
		// touch centre tags (never another user's private tag).
		if (in_array($op, array('assign', 'unassign', 'bulk'))) {
			if ($at_mode && $tag_row['t_owner'] != $at_uid) {
				_dh_tag_json(array('status' => 0, 'msg' => t('You can only apply your own tags.')));
			}
			if (!$at_mode && $tag_row['t_owner'] != 0) {
				_dh_tag_json(array('status' => 0, 'msg' => t('That is a private tag and cannot be changed here.')));
			}
		}
	}

	if ($op === 'pin') {
		db_query("INSERT IGNORE INTO dh_tag_pin (tp_user, tp_tag, tp_created) VALUES (:u, :t, NOW())",
			array(':u' => $user->uid, ':t' => $tag_id));
		_dh_tag_json(array('status' => 1, 'pinned' => 1));
	}
	if ($op === 'unpin') {
		db_query("DELETE FROM dh_tag_pin WHERE tp_user = :u AND tp_tag = :t",
			array(':u' => $user->uid, ':t' => $tag_id));
		_dh_tag_json(array('status' => 1, 'pinned' => 0));
	}

	if ($op === 'assign' || $op === 'bulk') {
		$apps = $op === 'bulk' ? array_map('intval', (array) $_POST['applications']) : array((int) $_POST['application']);
		$done = 0;
		foreach ($apps as $aid) {
			// AT: app must be in one of their confirmed courses. Centre: app must
			// belong to this centre.
			$ok = $at_mode
				? (function_exists('_at_view_applicant_in_my_courses') && _at_view_applicant_in_my_courses($aid))
				: _dh_tag_application_in_centre($aid, $centre_id);
			if ($aid <= 0 || !$ok) {
				continue; // silently skip apps outside scope
			}
			$was_active = _dh_tag_is_active($tag_id, $aid);
			// Upsert: re-activate a soft-deleted assignment, else insert.
			db_query(
				"INSERT INTO dh_application_tag (at_tag, at_application, at_created_by, at_created)
				 VALUES (:t, :a, :u, NOW())
				 ON DUPLICATE KEY UPDATE at_deleted = 0, at_created_by = :u, at_created = NOW()",
				array(':t' => $tag_id, ':a' => $aid, ':u' => $user->uid)
			);
			if (!$was_active) {
				_dh_tag_log($tag_id, $aid, 'add', $user->uid);
			}
			$done++;
		}
		_dh_tag_json(array('status' => 1, 'count' => $done));
	}

	if ($op === 'unassign') {
		$aid = (int) $_POST['application'];
		$ok = $at_mode
			? (function_exists('_at_view_applicant_in_my_courses') && _at_view_applicant_in_my_courses($aid))
			: _dh_tag_application_in_centre($aid, $centre_id);
		if (!$ok) {
			_dh_tag_json(array('status' => 0, 'msg' => t('No access to this application.')));
		}
		$was_active = _dh_tag_is_active($tag_id, $aid);
		db_update('dh_application_tag')->fields(array('at_deleted' => 1))
			->condition('at_tag', $tag_id)->condition('at_application', $aid)->execute();
		if ($was_active) {
			_dh_tag_log($tag_id, $aid, 'remove', $user->uid);
		}
		_dh_tag_json(array('status' => 1));
	}

	_dh_tag_json(array('status' => 0, 'msg' => t('Unknown operation.')));
}

/**
 * Is this application part of the given centre? Guards against tagging
 * applications the user's centre shouldn't touch. dh_applicant carries a_center
 * directly, so no course join is needed.
 */
function _dh_tag_application_in_centre($application_id, $centre_id)
{
	return (bool) db_query_range(
		"SELECT 1 FROM dh_applicant WHERE a_id = :a AND a_center = :c",
		0, 1,
		array(':a' => $application_id, ':c' => $centre_id)
	)->fetchField();
}

/* ---------------------------------------------------------------------------
 * Detail-page tag widget
 * ------------------------------------------------------------------------- */

/**
 * Self-contained tags widget for the application detail page: current tag chips
 * (each removable) plus an inline "+ add tag" select of the centre's unassigned
 * tags. Uses a plain <select> (no modal dependency). Returns '' if the user
 * can't use tags or lacks access to the application's centre.
 */
function _dh_tag_widget($app_id, $centre_id, $at_uid = 0)
{
	$at_uid = (int) $at_uid;
	if (!user_access('use tag')) {
		return '';
	}
	if ($at_uid > 0) {
		if (!_dh_tag_at_serves_centre($centre_id)) {
			return '';
		}
	}
	elseif (!_dh_tag_center_access($centre_id)) {
		return '';
	}
	drupal_add_css(drupal_get_path('module', 'dh_manageapp') . '/css/tags.css');
	// The "+ tag" picker uses jquery-confirm, which the detail page does not load
	// on its own (the search page does). Load it here so the widget works.
	drupal_add_css(drupal_get_path('module', 'dh_manageapp') . '/css/jquery-confirm.min.css');
	drupal_add_js(drupal_get_path('module', 'dh_manageapp') . '/js/jquery-confirm.min.js');

	$assigned = _dh_tag_for_application($app_id, $at_uid);
	$assigned_ids = array();
	$chips = '';
	foreach ($assigned as $t) {
		$assigned_ids[$t['t_id']] = 1;
		$sys = !empty($t['t_system']);
		$x = $sys ? '' : '<span class="tag-x" title="Remove">&times;</span>';
		$chips .= '<span class="tag-chip' . ($sys ? ' tag-system' : '') . '" data-tid="' . $t['t_id']
			. '"' . ($sys ? ' title="' . check_plain(t('System tag (auto-applied)')) . '"' : '')
			. ' style="' . _dh_tag_chip_style($t['t_color']) . '">'
			. check_plain($t['t_name']) . $x . '</span>';
	}
	// "+ tag" button opens the same picker used on the search-results list;
	// no <select> and no "manage" link on the detail view.
	$can_create_tag = ($at_uid > 0) ? 1 : (user_access('create tag') ? 1 : 0);
	$html = '<div class="app-tags app-tags-widget" data-aid="' . (int) $app_id . '">'
		. '<strong>' . t('Tags:') . '</strong> ' . $chips
		. '<span class="tag-add-btn" title="' . check_plain(t('Add tag')) . '">+ ' . t('tag') . '</span></div>';

	$centre_tags_json = json_encode(_dh_tag_center_tags($centre_id, $at_uid));
	$app_tag_url = url('app-tag/' . $centre_id . '/handler');
	$tags_handler_url = url('tags/' . $centre_id . '/handler');
	$palette_json = json_encode(_dh_tag_palette());
	$scope_js = $at_uid > 0 ? '"at"' : '""';
	drupal_add_js('
	(function ($) {
		$(document).ready(function () {
			var centreTags = ' . $centre_tags_json . ';
			var byId = {}; centreTags.forEach(function (t) { byId[t.t_id] = t; });
			var tagUrl = "' . $app_tag_url . '";
			var tagsHandlerUrl = "' . $tags_handler_url . '";
			var tagScope = ' . $scope_js . ';
			var canCreateTag = ' . $can_create_tag . ';
			var palette = ' . $palette_json . ';
			function esc(s) { return $("<span>").text(s).html(); }
			function chipStyle(bg){ bg=(bg||"").replace("#",""); if(bg.length===3){bg=bg[0]+bg[0]+bg[1]+bg[1]+bg[2]+bg[2];} if(bg.length!==6){bg="607d8b";} var r=parseInt(bg.substr(0,2),16),g=parseInt(bg.substr(2,2),16),b=parseInt(bg.substr(4,2),16); function hx(x){x=Math.max(0,Math.min(255,Math.round(x)));var s=x.toString(16);return s.length<2?"0"+s:s;} function mix(a,t){return a+(255-a)*t;} function lin(v){v/=255;return v<=0.03928?v/12.92:Math.pow((v+0.055)/1.055,2.4);} var back="#"+hx(mix(r,0.85))+hx(mix(g,0.85))+hx(mix(b,0.85)); var bord="#"+hx(mix(r,0.45))+hx(mix(g,0.45))+hx(mix(b,0.45)); var tr=r,tg=g,tb=b; for(var i=0;i<40;i++){var L=0.2126*lin(tr)+0.7152*lin(tg)+0.0722*lin(tb);if(L<=0.10)break;tr*=0.92;tg*=0.92;tb*=0.92;} var fore="#"+hx(tr)+hx(tg)+hx(tb); return "background:"+back+";color:"+fore+";border:1px solid "+bord; }
				function chip(t) { return $("<span class=\"tag-chip\" data-tid=\"" + t.t_id + "\" style=\"" + chipStyle(t.t_color) + "\">" + esc(t.t_name) + "<span class=\"tag-x\" title=\"Remove\">&times;</span></span>"); }
			$(document).on("click", ".app-tags-widget .tag-add-btn", function (e) {
				e.stopPropagation();
				var box = $(this).closest(".app-tags-widget"), aid = box.data("aid"), assigned = {};
				box.find(".tag-chip").each(function () { assigned[$(this).data("tid")] = 1; });
				function assignTag(t) {
					$.post(tagUrl, { op: "assign", tag: t.t_id, application: aid, scope: tagScope }, function (j) {
						if (j.status) { box.find(".tag-add-btn").before(chip(t)); } else { ($.alert ? $.alert(j.msg) : alert(j.msg)); }
					}, "json");
				}
				var avail = centreTags.filter(function (t) { return !assigned[t.t_id] && !(+t.t_system); });
				var chips = avail.map(function (t) {
					return "<span class=\"tag-chip tag-pick-opt\" data-tid=\"" + t.t_id + "\" data-name=\"" + esc(t.t_name) + "\" data-color=\"" + t.t_color + "\" style=\"" + chipStyle(t.t_color) + ";cursor:pointer\">" + esc(t.t_name) + "</span>";
				}).join(" ");
				var content = "<div class=\"tag-pick\">" + (chips || "<em>No unused tags.</em>") + "</div>"
					+ (canCreateTag ? "<div class=\"tag-new-wrap\"><button type=\"button\" class=\"tag-new-btn\">+ New tag</button></div>" : "");
				if (!$.confirm) { return; }
				$.confirm({
					title: "Add tag",
					content: content,
					buttons: { cancel: function () {} },
					onContentReady: function () {
						var self = this;
						self.$content.on("click", ".tag-pick-opt", function () {
							assignTag({ t_id: $(this).data("tid"), t_name: $(this).data("name"), t_color: $(this).data("color") });
							self.close();
						});
						self.$content.on("click", ".tag-new-btn", function () {
							var sw = palette.map(function (c) { return "<span class=\"tag-swatch\" data-color=\"" + c + "\" style=\"background:" + c + "\"></span>"; }).join("");
							var selColor = palette[0];
							$.confirm({
								title: "New tag",
								content: "<input type=\"text\" id=\"new-tag-name\" placeholder=\"Tag name\" maxlength=\"100\" style=\"width:100%;padding:4px 6px\"><div class=\"tag-swatches\" style=\"margin-top:12px\">" + sw + "</div>",
								buttons: {
									create: { text: "Create", btnClass: "btn-blue", action: function () {
										var d2 = this, name = $.trim(d2.$content.find("#new-tag-name").val());
										if (!name) { d2.$content.find("#new-tag-name").focus(); return false; }
										var cc = d2.$content.find(".tag-swatch.sel").data("color") || selColor;
										$.post(tagsHandlerUrl, { op: "create", name: name, color: cc, scope: tagScope }, function (j) {
											if (j.status) { var t = { t_id: j.tag.t_id, t_name: j.tag.t_name, t_color: j.tag.t_color }; centreTags.push(t); byId[t.t_id] = t; assignTag(t); d2.close(); self.close(); }
											else { ($.alert ? $.alert(j.msg) : alert(j.msg)); }
										}, "json");
										return false;
									} },
									cancel: function () {}
								},
								onContentReady: function () {
									var d = this;
									d.$content.find(".tag-swatch").first().addClass("sel");
									d.$content.on("click", ".tag-swatch", function () { d.$content.find(".tag-swatch").removeClass("sel"); $(this).addClass("sel"); selColor = $(this).data("color"); });
									d.$content.find("#new-tag-name").focus();
								}
							});
						});
					}
				});
			});
			$(document).on("click", ".app-tags-widget .tag-x", function () {
				var c = $(this).closest(".tag-chip"), box = c.closest(".app-tags-widget"), aid = box.data("aid"), tid = String(c.data("tid"));
				var t0 = byId[tid], name = t0 ? t0.t_name : "";
				var doRemove = function () {
					$.post(tagUrl, { op: "unassign", tag: tid, application: aid, scope: tagScope }, function (j) {
						if (j.status) { c.remove(); }
						else { alert(j.msg); }
					}, "json");
				};
				if ($.confirm) { $.confirm({ title: "Remove tag", content: "Remove tag \"" + name + "\" from this applicant?", buttons: { remove: { text: "Remove", btnClass: "btn-red", action: doRemove }, cancel: function () {} } }); }
				else if (confirm("Remove tag \"" + name + "\" from this applicant?")) { doRemove(); }
			});
		});
	})(jQuery);
	', 'inline');

	return $html;
}

/**
 * "Tag activity" panel for the application detail page (expanded by default):
 * who applied or removed each tag, and when. Scoped to the tags visible in this context
 * (centre context: centre + system; AT context: own private + system). Returns
 * '' when there is no activity. Uses a native <details> element (no JS).
 */
function _dh_tag_activity($app_id, $at_uid = 0)
{
	$app_id = (int) $app_id;
	$scope = _dh_tag_scope_where('t.', $at_uid);
	$rows = db_query(
		"SELECT l.atl_action, l.atl_at, l.atl_by, t.t_name, t.t_color, u.name AS uname
		 FROM dh_application_tag_log l
		 JOIN dh_tag t ON t.t_id = l.atl_tag
		 LEFT JOIN users u ON u.uid = l.atl_by
		 WHERE l.atl_application = :a AND t.t_deleted = 0 AND $scope
		 ORDER BY l.atl_at DESC, l.atl_id DESC",
		array(':a' => $app_id)
	)->fetchAll(PDO::FETCH_ASSOC);
	// Detail view: expanded by default (list stays collapsed to keep rows compact).
	return _dh_tag_activity_render($rows, TRUE);
}

/**
 * Batched activity panels for a set of applications (search results list): one
 * query for all app ids, grouped and rendered per app. Returns an array keyed
 * by application id => collapsed <details> HTML (apps with no activity are
 * absent). Avoids the N+1 of calling _dh_tag_activity() per row.
 */
function _dh_tag_activity_bulk(array $app_ids, $at_uid = 0)
{
	$app_ids = array_values(array_unique(array_map('intval', $app_ids)));
	if (!$app_ids) {
		return array();
	}
	$scope = _dh_tag_scope_where('t.', $at_uid);
	$res = db_query(
		"SELECT l.atl_application, l.atl_action, l.atl_at, l.atl_by, t.t_name, t.t_color, u.name AS uname
		 FROM dh_application_tag_log l
		 JOIN dh_tag t ON t.t_id = l.atl_tag
		 LEFT JOIN users u ON u.uid = l.atl_by
		 WHERE l.atl_application IN (:aids) AND t.t_deleted = 0 AND $scope
		 ORDER BY l.atl_at DESC, l.atl_id DESC",
		array(':aids' => $app_ids)
	)->fetchAll(PDO::FETCH_ASSOC);
	$grouped = array();
	foreach ($res as $r) {
		$grouped[$r['atl_application']][] = $r;
	}
	$out = array();
	foreach ($grouped as $aid => $rows) {
		$out[$aid] = _dh_tag_activity_render($rows);
	}
	return $out;
}

/**
 * Render one application's log rows into the collapsed "Tag activity" panel.
 * Shared by the detail view (single fetch) and the list (bulk fetch). Rows are
 * already scope-filtered and newest-first. Returns '' for an empty set.
 */
function _dh_tag_activity_render($rows, $open = FALSE)
{
	if (!$rows) {
		return '';
	}
	drupal_add_css(drupal_get_path('module', 'dh_manageapp') . '/css/tags.css');

	$body = '';
	foreach ($rows as $r) {
		$who = ($r['atl_by'] == 0) ? t('System') : ($r['uname'] !== NULL && $r['uname'] !== '' ? $r['uname'] : t('User #@u', array('@u' => $r['atl_by'])));
		$act = ($r['atl_action'] === 'remove') ? t('removed') : t('added');
		$cls = ($r['atl_action'] === 'remove') ? 'tal-remove' : 'tal-add';
		$body .= '<tr>'
			. '<td class="tal-tag"><span class="tag-dot" style="background:' . check_plain($r['t_color']) . '"></span>' . check_plain($r['t_name']) . '</td>'
			. '<td class="tal-act ' . $cls . '">' . $act . '</td>'
			. '<td class="tal-who">' . check_plain($who) . '</td>'
			. '<td class="tal-when">' . check_plain($r['atl_at']) . '</td>'
			. '</tr>';
	}
	$caret = '<span class="tag-activity-caret" aria-hidden="true">&#9656;</span> ';
	return '<details class="tag-activity"' . ($open ? ' open' : '') . '><summary>' . $caret . t('Tag activity (@n)', array('@n' => count($rows))) . '</summary>'
		. '<table class="tal-table"><thead><tr>'
		. '<th>' . t('Tag') . '</th><th>' . t('Action') . '</th><th>' . t('By') . '</th><th>' . t('When') . '</th>'
		. '</tr></thead><tbody>' . $body . '</tbody></table></details>';
}

/* ---------------------------------------------------------------------------
 * Tagged-applications search (dashboard Present/Past links)
 * ------------------------------------------------------------------------- */

/**
 * search-tag/%centre_id/<tag>/<period> : list a tag's applications, split by the
 * application's course end date. present = c_end >= today, past = c_end < today.
 * Reuses the main search renderer. Results are scoped to the centre and to the
 * viewer's gender access; the tag subquery inherently limits to that centre's tag.
 */
function _search_tag()
{
	$centre_id = arg(1);
	$tag_id = (int) arg(2);
	$period = arg(3);
	if (!in_array($period, array('upcoming', 'all', 'past'), TRUE)) {
		$period = 'all';
	}

	$tag_name = db_query("SELECT t_name FROM dh_tag WHERE t_id = :t AND t_center = :c AND t_deleted = 0",
		array(':t' => $tag_id, ':c' => $centre_id))->fetchField();
	if ($tag_name === FALSE) {
		return MENU_NOT_FOUND;
	}

	// Upcoming / Past split by the application's course START date; All = no filter.
	$where = "a_center = " . (int) $centre_id
		. " AND a_id IN (SELECT at_application FROM dh_application_tag WHERE at_tag = " . $tag_id . " AND at_deleted = 0)";
	if ($period === 'upcoming') {
		$where .= " AND a_course IN (SELECT c_id FROM dh_course WHERE c_start >= CURDATE())";
	}
	elseif ($period === 'past') {
		$where .= " AND a_course IN (SELECT c_id FROM dh_course WHERE c_start < CURDATE())";
	}

	// Scope to the viewer's gender access (no cross-gender leak).
	if ( !(user_access('access male') && user_access('access female')) ) {
		$g = user_access('access male') ? 'M' : 'F';
		$where .= " AND a_gender = '$g'";
	}

	// Guard against a huge result set: the shared search builds every matching row
	// in memory, so count first and, if it exceeds the cap, render only the first
	// N application ids. ($where here is built from validated ints + fixed literals.)
	$max = (int) variable_get('tag_list_max', 500);
	$total = (int) db_query("SELECT COUNT(*) FROM dh_applicant WHERE $where")->fetchField();
	$notice = '';
	if ($total > $max)
	{
		$ids = db_query("SELECT a_id FROM dh_applicant WHERE $where ORDER BY a_id LIMIT $max")->fetchCol();
		$where = 'a_id IN (' . ($ids ? implode(',', array_map('intval', $ids)) : '0') . ')';
		$notice = '<div class="messages warning">'
			. t('This tag has @total applications for this view. Showing the first @max — narrow with Upcoming / Past, or use Export.', array('@total' => $total, '@max' => $max))
			. '</div>';
	}

	$out = '<h3>' . t('Tag: @t (@p) &middot; @n total', array('@t' => $tag_name, '@p' => ucfirst($period), '@n' => $total))
		. ' &nbsp;|&nbsp; ' . l(t('Back to Dashboard'), 'centre/' . $centre_id) . '</h3>' . $notice;
	$fname = preg_replace('/[^A-Za-z0-9_-]/', '_', $tag_name) . '-' . $period;
	$out .= dh_manageapp_search_results('applicant', $where, $centre_id, FALSE, $fname);
	return $out;
}

/* ---------------------------------------------------------------------------
 * Admin page
 * ------------------------------------------------------------------------- */

/**
 * Tag admin page: tags/%centre_id — list + create/rename/recolor/delete.
 */
function dh_tags_admin($centre_id)
{
	drupal_add_css(drupal_get_path('module', 'dh_manageapp') . '/css/manageapp.css');
	drupal_add_css(drupal_get_path('module', 'dh_manageapp') . '/css/tags.css');
	$palette = _dh_tag_palette();
	$tags = _dh_tag_center_tags($centre_id);

	global $user;
	$pinned_ids = array_flip(_dh_tag_pinned_ids($user->uid));
	$can_use = user_access('use tag') ? 1 : 0;
	$can_create = user_access('create tag') ? 1 : 0;

	$swatches = '';
	foreach ($palette as $c) {
		$swatches .= '<span class="tag-swatch" data-color="' . $c . '" style="background:' . $c . '"></span>';
	}

	$rows = '';
	foreach ($tags as $t) {
		$is_system = !empty($t['t_system']);
		$can_edit = (!$is_system && _dh_tag_can_modify($t, 'edit')) ? 1 : 0;
		$can_del = (!$is_system && _dh_tag_can_modify($t, 'delete')) ? 1 : 0;
		$actions = $is_system ? '<em title="' . check_plain(t('Auto-applied when an Assistant Teacher comments; cannot be edited or deleted.')) . '">' . t('system') . '</em> ' : '';
		if ($can_edit) {
			$actions .= '<a href="#" class="tag-edit" data-id="' . $t['t_id'] . '" data-name="' . check_plain($t['t_name']) . '" data-color="' . check_plain($t['t_color']) . '">Edit</a> ';
		}
		if ($can_del) {
			$actions .= '<a href="#" class="tag-delete" data-id="' . $t['t_id'] . '" data-name="' . check_plain($t['t_name']) . '">Delete</a>';
		}
		$pin = '';
		if ($can_use) {
			$is_pinned = isset($pinned_ids[$t['t_id']]);
			$pin = '<a href="#" class="tag-admin-pin' . ($is_pinned ? '' : ' off') . '" data-id="' . $t['t_id'] . '" title="' . t('Pin for me') . '">' . ($is_pinned ? '&#9733;' : '&#9734;') . '</a> ';
		}
		$rows .= '<tr data-id="' . $t['t_id'] . '">'
			. '<td>' . $pin . '<span class="tag-chip" style="' . _dh_tag_chip_style($t['t_color']) . '">' . check_plain($t['t_name']) . '</span></td>'
			. '<td>' . $actions . '</td></tr>';
	}

	$create_block = '';
	if ($can_create) {
		$create_block = '<div class="tag-create">'
			. '<input type="text" id="tag-new-name" placeholder="New tag name" maxlength="100" />'
			. '<span class="tag-swatches">' . $swatches . '</span>'
			. '<button id="tag-add-btn" class="btn btn-blue">Add Tag</button></div>';
	}

	$out = '<h2>' . l('&laquo; Back to Centre', 'centre/' . $centre_id, array('html' => TRUE)) . '</h2>'
		. '<h3>' . t('Application Tags') . '</h3>'
		. $create_block
		. '<table class="table tag-table"><thead><tr><th>Tag</th><th>Actions</th></tr></thead>'
		. '<tbody>' . ($rows ?: '<tr><td colspan="2"><em>No tags yet.</em></td></tr>') . '</tbody></table>';

	$handler = url('tags/' . $centre_id . '/handler');
	$app_tag_handler = url('app-tag/' . $centre_id . '/handler');
	drupal_add_js('
	(function ($) {
		$(document).ready(function () {
			var selColor = "' . $palette[0] . '";
			var pinUrl = "' . $app_tag_handler . '";
			$(".tag-swatches .tag-swatch").first().addClass("sel");
			$(document).on("click", ".tag-swatches .tag-swatch", function () {
				$(this).closest(".tag-swatches").find(".tag-swatch").removeClass("sel");
				$(this).addClass("sel"); selColor = $(this).data("color");
			});
			$(document).on("click", ".tag-admin-pin", function (e) {
				e.preventDefault();
				var a = $(this), tid = String(a.data("id")), pinned = !a.hasClass("off");
				$.post(pinUrl, { op: pinned ? "unpin" : "pin", tag: tid }, function (j) {
					if (j.status) { a.toggleClass("off"); a.html(a.hasClass("off") ? "\\u2606" : "\\u2605"); }
					else { ($.alert ? $.alert(j.msg) : alert(j.msg)); }
				}, "json");
			});
			$("#tag-add-btn").on("click", function () {
				var name = $.trim($("#tag-new-name").val());
				if (!name) { $.alert ? $.alert("Please enter a tag name.") : alert("Please enter a tag name."); return; }
				$.post("' . $handler . '", { op: "create", name: name, color: selColor }, function (j) {
					if (j.status) { location.reload(); } else { ($.alert ? $.alert(j.msg) : alert(j.msg)); }
				}, "json");
			});
			$(document).on("click", ".tag-delete", function (e) {
				e.preventDefault();
				var id = $(this).data("id"), name = $(this).data("name");
				var go = function () {
					$.post("' . $handler . '", { op: "delete", t_id: id }, function (j) {
						if (j.status) { location.reload(); } else { ($.alert ? $.alert(j.msg) : alert(j.msg)); }
					}, "json");
				};
				if ($.confirm) { $.confirm({ title: "Delete tag", content: "Delete tag \"" + name + "\"? It will be removed from all applications.", buttons: { del: { text: "Delete", btnClass: "btn-red", action: go }, cancel: function () {} } }); }
				else if (confirm("Delete tag " + name + "?")) { go(); }
			});
			$(document).on("click", ".tag-edit", function (e) {
				e.preventDefault();
				var id = $(this).data("id"), name = $(this).data("name"), color = $(this).data("color");
				var sw = ""; ' . json_encode($palette) . '.forEach(function (c) { sw += "<span class=\"tag-swatch" + (c===color?" sel":"") + "\" data-color=\"" + c + "\" style=\"background:" + c + "\"></span>"; });
				if (!$.confirm) { var nn = prompt("Rename tag", name); if (nn===null) return; $.post("' . $handler . '", { op: "update", t_id: id, name: nn, color: color }, function (j) { if (j.status) location.reload(); else alert(j.msg); }, "json"); return; }
				$.confirm({
					title: "Edit tag",
					content: "<input type=\"text\" id=\"tag-edit-name\" value=\"" + name + "\" style=\"width:100%\" maxlength=\"100\"><div class=\"tag-swatches edit-sw\" style=\"margin-top:8px\">" + sw + "</div>",
					buttons: { save: { text: "Save", btnClass: "btn-blue", action: function () {
						var self = this; var nn = $.trim($("#tag-edit-name").val()); var cc = this.$content.find(".tag-swatch.sel").data("color") || color;
						$.post("' . $handler . '", { op: "update", t_id: id, name: nn, color: cc }, function (j) { if (j.status) location.reload(); else { ($.alert ? $.alert(j.msg) : alert(j.msg)); } }, "json");
						return false;
					} }, cancel: function () {} },
					onContentReady: function () { var self = this; this.$content.on("click", ".tag-swatch", function () { self.$content.find(".tag-swatch").removeClass("sel"); $(this).addClass("sel"); }); }
				});
			});
		});
	})(jQuery);
	', 'inline');

	return $out;
}
