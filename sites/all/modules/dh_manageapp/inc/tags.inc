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
 * Menu access: the tag admin page / CRUD handler. Any tag permission + centre access.
 */
function _dh_tag_admin_access()
{
	$centre = arg(1);
	return $centre !== NULL && _dh_tag_has_any_perm() && _dh_tag_center_access($centre);
}

/**
 * Menu access: assign/unassign endpoints. 'use tag' + centre access.
 */
function _dh_tag_use_access()
{
	$centre = arg(1);
	return $centre !== NULL && user_access('use tag') && _dh_tag_center_access($centre);
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
 * All non-deleted tags for a centre, ordered by name.
 */
function _dh_tag_center_tags($centre_id)
{
	return db_query(
		"SELECT t_id, t_name, t_color, t_created_by FROM dh_tag WHERE t_center = :c AND t_deleted = 0 ORDER BY t_name",
		array(':c' => $centre_id)
	)->fetchAll(PDO::FETCH_ASSOC);
}

/**
 * Is a tag name already taken in this centre (case-insensitive, non-deleted)?
 * $exclude_id lets an edit keep its own name.
 */
function _dh_tag_name_exists($centre_id, $name, $exclude_id = 0)
{
	return (bool) db_query_range(
		"SELECT 1 FROM dh_tag WHERE t_center = :c AND t_deleted = 0 AND t_name = :n AND t_id <> :x",
		0, 1,
		array(':c' => $centre_id, ':n' => trim($name), ':x' => $exclude_id)
	)->fetchField();
}

/**
 * Tags currently on an application (non-deleted), for row/detail rendering.
 */
function _dh_tag_for_application($application_id)
{
	return db_query(
		"SELECT t.t_id, t.t_name, t.t_color
		 FROM dh_application_tag at JOIN dh_tag t ON at.at_tag = t.t_id
		 WHERE at.at_application = :a AND at.at_deleted = 0 AND t.t_deleted = 0
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

	if ($op === 'list') {
		_dh_tag_json(array('status' => 1, 'tags' => _dh_tag_center_tags($centre_id)));
	}

	if ($op === 'create') {
		if (!user_access('create tag')) {
			_dh_tag_json(array('status' => 0, 'msg' => t('You do not have permission to create tags.')));
		}
		$name = trim($_POST['name']);
		$color = in_array($_POST['color'], _dh_tag_palette()) ? $_POST['color'] : '#607d8b';
		if ($name === '') {
			_dh_tag_json(array('status' => 0, 'msg' => t('Tag name is required.')));
		}
		if (drupal_strlen($name) > 100) {
			_dh_tag_json(array('status' => 0, 'msg' => t('Tag name is too long (max 100).')));
		}
		if (_dh_tag_name_exists($centre_id, $name)) {
			_dh_tag_json(array('status' => 0, 'msg' => t('A tag with that name already exists.')));
		}
		$id = db_insert('dh_tag')->fields(array(
			't_center' => $centre_id, 't_name' => $name, 't_color' => $color,
			't_created_by' => $user->uid, 't_updated_by' => $user->uid,
		))->execute();
		_dh_tag_json(array('status' => 1, 'tag' => array('t_id' => $id, 't_name' => $name, 't_color' => $color, 't_created_by' => $user->uid)));
	}

	if ($op === 'update') {
		$tag = db_query("SELECT * FROM dh_tag WHERE t_id = :id AND t_center = :c AND t_deleted = 0",
			array(':id' => $_POST['t_id'], ':c' => $centre_id))->fetchAssoc();
		if (!$tag) {
			_dh_tag_json(array('status' => 0, 'msg' => t('Tag not found.')));
		}
		if (!_dh_tag_can_modify($tag, 'edit')) {
			_dh_tag_json(array('status' => 0, 'msg' => t('You do not have permission to edit this tag.')));
		}
		$name = trim($_POST['name']);
		$color = in_array($_POST['color'], _dh_tag_palette()) ? $_POST['color'] : $tag['t_color'];
		if ($name === '') {
			_dh_tag_json(array('status' => 0, 'msg' => t('Tag name is required.')));
		}
		if (_dh_tag_name_exists($centre_id, $name, $tag['t_id'])) {
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
		if (!_dh_tag_can_modify($tag, 'delete')) {
			_dh_tag_json(array('status' => 0, 'msg' => t('You do not have permission to delete this tag.')));
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

	if ($op === 'for_app') {
		$aid = (int) (isset($_GET['aid']) ? $_GET['aid'] : $_POST['aid']);
		_dh_tag_json(array('status' => 1, 'tags' => _dh_tag_for_application($aid), 'all' => _dh_tag_center_tags($centre_id)));
	}

	// Validate the tag exists in this centre for the mutating ops.
	if (in_array($op, array('assign', 'unassign', 'bulk', 'pin', 'unpin'))) {
		$tag_id = (int) $_POST['tag'];
		$tag_ok = db_query("SELECT 1 FROM dh_tag WHERE t_id = :id AND t_center = :c AND t_deleted = 0",
			array(':id' => $tag_id, ':c' => $centre_id))->fetchField();
		if (!$tag_ok) {
			_dh_tag_json(array('status' => 0, 'msg' => t('Tag not found in this centre.')));
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
			if ($aid <= 0 || !_dh_tag_application_in_centre($aid, $centre_id)) {
				continue; // silently skip apps outside this centre
			}
			// Upsert: re-activate a soft-deleted assignment, else insert.
			db_query(
				"INSERT INTO dh_application_tag (at_tag, at_application, at_created_by, at_created)
				 VALUES (:t, :a, :u, NOW())
				 ON DUPLICATE KEY UPDATE at_deleted = 0, at_created_by = :u, at_created = NOW()",
				array(':t' => $tag_id, ':a' => $aid, ':u' => $user->uid)
			);
			$done++;
		}
		_dh_tag_json(array('status' => 1, 'count' => $done));
	}

	if ($op === 'unassign') {
		$aid = (int) $_POST['application'];
		db_update('dh_application_tag')->fields(array('at_deleted' => 1))
			->condition('at_tag', $tag_id)->condition('at_application', $aid)->execute();
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
function _dh_tag_widget($app_id, $centre_id)
{
	if (!user_access('use tag') || !_dh_tag_center_access($centre_id)) {
		return '';
	}
	drupal_add_css(drupal_get_path('module', 'dh_manageapp') . '/css/tags.css');

	$assigned = _dh_tag_for_application($app_id);
	$assigned_ids = array();
	$chips = '';
	foreach ($assigned as $t) {
		$assigned_ids[$t['t_id']] = 1;
		$chips .= '<span class="tag-chip" data-tid="' . $t['t_id'] . '" style="background:' . check_plain($t['t_color']) . '">'
			. check_plain($t['t_name']) . '<span class="tag-x" title="Remove">&times;</span></span>';
	}
	$all = _dh_tag_center_tags($centre_id);
	$opts = '<option value="">+ ' . t('add tag') . '&hellip;</option>';
	foreach ($all as $t) {
		if (!isset($assigned_ids[$t['t_id']])) {
			$opts .= '<option value="' . $t['t_id'] . '">' . check_plain($t['t_name']) . '</option>';
		}
	}
	$html = '<div class="app-tags app-tags-widget" data-aid="' . (int) $app_id . '">'
		. '<strong>' . t('Tags:') . '</strong> ' . $chips
		. '<select class="tag-add-select">' . $opts . '</select> '
		. '<a href="' . url('tags/' . $centre_id) . '" target="_blank" class="tag-manage-link">' . t('manage') . '</a></div>';

	$centre_tags_json = json_encode(_dh_tag_center_tags($centre_id));
	$app_tag_url = url('app-tag/' . $centre_id . '/handler');
	drupal_add_js('
	(function ($) {
		$(document).ready(function () {
			var byId = {}; (' . $centre_tags_json . ').forEach(function (t) { byId[t.t_id] = t; });
			var tagUrl = "' . $app_tag_url . '";
			function esc(s) { return $("<span>").text(s).html(); }
			function chip(t) { return $("<span class=\"tag-chip\" data-tid=\"" + t.t_id + "\" style=\"background:" + t.t_color + "\">" + esc(t.t_name) + "<span class=\"tag-x\" title=\"Remove\">&times;</span></span>"); }
			$(document).on("change", ".app-tags-widget .tag-add-select", function () {
				var sel = $(this), box = sel.closest(".app-tags-widget"), aid = box.data("aid"), tid = String(sel.val());
				if (!tid) { return; }
				$.post(tagUrl, { op: "assign", tag: tid, application: aid }, function (j) {
					if (j.status) { sel.before(chip(byId[tid])); sel.find("option[value=\"" + tid + "\"]").remove(); sel.val(""); }
					else { alert(j.msg); }
				}, "json");
			});
			$(document).on("click", ".app-tags-widget .tag-x", function () {
				var c = $(this).closest(".tag-chip"), box = c.closest(".app-tags-widget"), aid = box.data("aid"), tid = String(c.data("tid"));
				$.post(tagUrl, { op: "unassign", tag: tid, application: aid }, function (j) {
					if (j.status) { c.remove(); var t = byId[tid]; if (t) { box.find(".tag-add-select").append($("<option>").attr("value", tid).text(t.t_name)); } }
					else { alert(j.msg); }
				}, "json");
			});
		});
	})(jQuery);
	', 'inline');

	return $html;
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
		$can_edit = _dh_tag_can_modify($t, 'edit') ? 1 : 0;
		$can_del = _dh_tag_can_modify($t, 'delete') ? 1 : 0;
		$actions = '';
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
			. '<td>' . $pin . '<span class="tag-chip" style="background:' . check_plain($t['t_color']) . '">' . check_plain($t['t_name']) . '</span></td>'
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
