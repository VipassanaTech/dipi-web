<?php

/**
 * Centre change log (table dh_center_log): who changed what on a centre, old ->
 * new, one row per changed field. Written by the code at save time - the settings
 * and accommodation tables have no "updated by" column, and config blocks need a
 * format-insensitive compare - for:
 *   'centre'   dh_center               Centre Settings form (dh_ma_centre_form_submit)
 *   'settings' dh_center_setting       same form
 *   'acco'     dh_center_setting_acco  accommodation editor (dh_acco_handler)
 *   'access'   dh_user_center          user mapping to centre (dh_manage_usermapping_handler)
 * Shown at the bottom of the Centre Settings page (dh_centre_log_viewer).
 * Changes made directly in SQL are not logged.
 */

/* Config blocks stored as INI: compared as parsed values (so re-formatting or
   re-ordering by the visual editors is not a change) and shown as changed lines. */
function _dh_centre_log_ini_fields()
{
	return array('cs_course_config', 'cs_seat_config', 'cs_cells_config', 'cs_dining_config');
}

/* The centre-table columns (c_* / cs_*) in the Centre Settings form as built for
   this user, with their label and options. The form saves only these, so a crafted
   POST cannot set hidden or admin-only columns. */
function _dh_centre_form_fields($elements, &$out = array())
{
	foreach (element_children($elements) as $key)
	{
		if (preg_match('/^cs?_/', $key) && isset($elements[$key]['#type']))
		{
			$title = isset($elements[$key]['#title']) ? trim(strip_tags(str_replace('&nbsp;', ' ', $elements[$key]['#title']))) : '';
			$title = preg_replace('/\s*\(raw\)$/i', '', $title);   // "Cells Configuration (raw)"
			$options = isset($elements[$key]['#options']) ? $elements[$key]['#options'] : NULL;
			if (!$options && $elements[$key]['#type'] == 'checkbox')
				$options = array(1 => 'Yes', 0 => 'No');
			$out[$key] = array(
				'title' => $title !== '' ? $title : $key,
				'options' => $options,
			);
		}
		_dh_centre_form_fields($elements[$key], $out);
	}
	return $out;
}

/* An INI config as "[SECTION] key = value" lines (parse order), or FALSE if it is
   not valid INI. */
function _dh_centre_log_ini_lines($value)
{
	$ini = @parse_ini_string((string) $value, TRUE, INI_SCANNER_RAW);
	if (!is_array($ini))
		return FALSE;
	$lines = array();
	foreach ($ini as $sec => $vals)
	{
		if (is_array($vals))
			foreach ($vals as $k => $v)
				$lines[] = '['.$sec.'] '.$k.' = '.(is_array($v) ? implode(',', $v) : trim($v));
		else
			$lines[] = $sec.' = '.trim($vals);
	}
	return $lines;
}

/* The value used to decide whether a field really changed: NULL and '' are the
   same, line endings and outer spaces are ignored, and INI configs are compared
   by content (sorted lines) rather than by text. */
function _dh_centre_log_norm($field, $value)
{
	$value = str_replace(array("\r\n", "\r"), "\n", trim((string) $value));
	if (in_array($field, _dh_centre_log_ini_fields()))
	{
		$lines = _dh_centre_log_ini_lines($value);
		if ($lines !== FALSE)
		{
			sort($lines);
			return implode("\n", $lines);
		}
	}
	return $value;
}

/* Fields in $fields whose value in $new (the values being saved) differs from
   $old (the row before; NULL for a new row): field => array(old, new). */
function dh_centre_log_diff($old, $new, $fields)
{
	$changes = array();
	foreach ($fields as $f)
	{
		if (!array_key_exists($f, $new))
			continue;
		$o = (is_array($old) && array_key_exists($f, $old)) ? $old[$f] : NULL;
		if (_dh_centre_log_norm($f, $o) !== _dh_centre_log_norm($f, $new[$f]))
			$changes[$f] = array($o, $new[$f]);
	}
	return $changes;
}

/* Write one log row per change, as the current user. $editor_db (a DataTables
   Editor database) writes inside the Editor's own transaction; otherwise Drupal's
   connection is used (the caller's db_transaction, if any). */
function dh_centre_log_write($centre, $area, $action, $changes, $record = NULL, $editor_db = NULL)
{
	global $user;
	foreach ($changes as $field => $pair)
	{
		$row = array(
			'cl_center' => (int) $centre,
			'cl_area' => $area,
			'cl_record' => $record,
			'cl_action' => $action,
			'cl_field' => $field,
			'cl_old' => $pair[0],
			'cl_new' => $pair[1],
			'cl_user' => (int) $user->uid,
			'cl_tstamp' => date('Y-m-d H:i:s'),
		);
		if ($editor_db)
			$editor_db->insert('dh_center_log', $row);
		else
			db_insert('dh_center_log')->fields($row)->execute();
	}
}

/* Log every row a DataTables Editor handler creates or edits (accommodation, user
   access), with the Editor's own database so each log row commits with its change.
   The Delete buttons are edits that set $deleted_col to 1. A row whose centre
   changes is logged on both centres. */
function dh_centre_log_editor($editor, $table, $pk, $centre_col, $area, $fields, $deleted_col)
{
	$old = array();
	$load = function ($id) use ($editor, $table, $pk) {
		return $editor->db()->select($table, '*', array($pk => $id))->fetch();
	};
	$log = function ($before, $after, $id) use ($editor, $centre_col, $area, $fields, $deleted_col) {
		if (!$after)
			return;
		$action = 'update';
		if (!$before)
			$action = 'create';
		elseif (!$before[$deleted_col] && $after[$deleted_col])
			$action = 'delete';
		elseif ($before[$deleted_col] && !$after[$deleted_col])
			$action = 'restore';
		$changes = dh_centre_log_diff($before, $after, $fields);
		if ($action == 'create')
			unset($changes[$centre_col]);   // the centre is the log's own centre
		if (!$changes && $action != 'update')
			$changes[$deleted_col] = array($before ? $before[$deleted_col] : NULL, $after[$deleted_col]);
		if (!$changes)
			return;
		$centres = array($after[$centre_col]);
		if ($before && (string) $before[$centre_col] !== (string) $after[$centre_col])
			$centres[] = $before[$centre_col];
		foreach ($centres as $c)
			dh_centre_log_write($c, $area, $action, $changes, $id, $editor->db());
	};
	$editor
		->on('preEdit', function ($e, $id, $values) use (&$old, $load) {
			$old[$id] = $load($id);
		})
		->on('postEdit', function ($e, $id, $values, $row) use (&$old, $load, $log) {
			$log(isset($old[$id]) ? $old[$id] : NULL, $load($id), $id);
		})
		->on('postCreate', function ($e, $id, $values, $row) use ($load, $log) {
			$log(NULL, $load($id), $id);
		});
}

/* A logged value as display HTML: ids shown as names, option keys as their labels. */
function _dh_centre_log_value($field, $value, $fields)
{
	if ($value === NULL || trim((string) $value) === '')
		return '<em>(empty)</em>';
	switch ($field)
	{
		case 'c_city':
			$name = ctype_digit((string) $value) ? db_query("select c_name from dh_city where c_id = :i", array(':i' => $value))->fetchField() : FALSE;
			return check_plain($name ? $name : $value);
		case 'uc_user':
			$name = db_query("select name from users where uid = :u", array(':u' => $value))->fetchField();
			return check_plain($name ? $name : 'user '.$value);
		case 'uc_center':
			$name = db_query("select c_name from dh_center where c_id = :c", array(':c' => $value))->fetchField();
			return check_plain($name ? $name : 'centre '.$value);
		case 'csa_gender':
			return $value == 'F' ? 'Female' : ($value == 'M' ? 'Male' : check_plain($value));
		case 'csa_deleted':
		case 'uc_deleted':
			return $value ? 'Yes' : 'No';
	}
	if (!empty($fields[$field]['options']))
	{
		$opts = $fields[$field]['options'];
		$parts = array();
		foreach (explode(',', (string) $value) as $v)
			$parts[] = isset($opts[$v]) ? strip_tags($opts[$v]) : $v;
		return check_plain(implode(', ', $parts));
	}
	return nl2br(check_plain($value));
}

/* Old/new display for an INI config: only the lines that differ. */
function _dh_centre_log_ini_diff($old, $new)
{
	$o = _dh_centre_log_ini_lines($old);
	$n = _dh_centre_log_ini_lines($new);
	if ($o === FALSE || $n === FALSE)
		return array(nl2br(check_plain($old)), nl2br(check_plain($new)));
	$fmt = function ($lines, $cls) {
		if (!$lines)
			return '<em>(nothing)</em>';
		$out = array();
		foreach ($lines as $l)
			$out[] = '<span class="'.$cls.'">'.check_plain($l).'</span>';
		return implode('<br>', $out);
	};
	return array($fmt(array_values(array_diff($o, $n)), 'cl-del'), $fmt(array_values(array_diff($n, $o)), 'cl-add'));
}

/* The change log of a centre as an HTML table, newest first (latest 500). $form
   is the Centre Settings form, for field labels and option names. */
function dh_centre_log_viewer($centre_id, $form)
{
	$fields = _dh_centre_form_fields($form);
	// Labels for columns that are not on this viewer's form: admin-only and add-only
	// fields (a non-admin's form lacks them), the seat INI and the row-level areas.
	$fields += array(
		'c_vri' => array('title' => 'Enable VRI Schedule', 'options' => array(1 => 'Yes', 0 => 'No')),
		'c_type' => array('title' => 'Location-Type', 'options' => array('center' => 'Center', 'noncenter' => 'NonCenter')),
		'c_subdomain' => array('title' => 'Dhamma.org Identifier', 'options' => NULL),
		'c_dana' => array('title' => 'Enable Dana?', 'options' => array(1 => 'Yes', 0 => 'No')),
		'cs_seat_config' => array('title' => 'Seating Layout', 'options' => NULL),
		'csa_gender' => array('title' => 'Gender', 'options' => NULL),
		'csa_section' => array('title' => 'Section', 'options' => NULL),
		'csa_room' => array('title' => 'Rooms', 'options' => NULL),
		'uc_user' => array('title' => 'User', 'options' => NULL),
		'uc_center' => array('title' => 'Centre', 'options' => NULL),
	);
	$areas = array('centre' => 'Centre details', 'settings' => 'Settings', 'acco' => 'Accommodation', 'access' => 'User mapping to centre');

	$res = db_query("select l.*, u.name from dh_center_log l left join users u on u.uid = l.cl_user
		where l.cl_center = :c order by l.cl_tstamp desc, l.cl_id desc limit 501", array(':c' => $centre_id))->fetchAll();
	if (!$res)
		return '<p>No changes recorded yet.</p>';
	$more = count($res) > 500;
	$res = array_slice($res, 0, 500);

	// Context for row-level areas: the accommodation row's gender/section, the
	// user of a user-access row (rows are only ever soft-deleted, so still there).
	$acco = array(); $access = array();
	foreach ($res as $r)
	{
		if ($r->cl_area == 'acco') $acco[$r->cl_record] = $r->cl_record;
		if ($r->cl_area == 'access') $access[$r->cl_record] = $r->cl_record;
	}
	if ($acco)
		$acco = db_query("select csa_id, csa_gender, csa_section, csa_room from dh_center_setting_acco where csa_id in (:ids)", array(':ids' => array_values($acco)))->fetchAllAssoc('csa_id');
	if ($access)
		$access = db_query("select uc.uc_id, u.name from dh_user_center uc left join users u on u.uid = uc.uc_user where uc.uc_id in (:ids)", array(':ids' => array_values($access)))->fetchAllKeyed();

	$rows = array();
	foreach ($res as $r)
	{
		$label = isset($fields[$r->cl_field]) ? $fields[$r->cl_field]['title'] : $r->cl_field;
		$old = _dh_centre_log_value($r->cl_field, $r->cl_old, $fields);
		$new = _dh_centre_log_value($r->cl_field, $r->cl_new, $fields);
		if (in_array($r->cl_field, _dh_centre_log_ini_fields()))
			list($old, $new) = _dh_centre_log_ini_diff($r->cl_old, $r->cl_new);

		$what = check_plain($label);
		if ($r->cl_area == 'centre' && $r->cl_action == 'create')
			$what = 'Centre created';
		elseif ($r->cl_area == 'acco')
		{
			$a = isset($acco[$r->cl_record]) ? $acco[$r->cl_record] : NULL;
			$ctx = $a ? ($a->csa_gender == 'F' ? 'Female' : 'Male').' &middot; '.check_plain($a->csa_section) : 'row '.(int) $r->cl_record;
			if ($r->cl_action == 'delete' || $r->cl_action == 'restore')
			{
				$what = $ctx.' &mdash; '.$r->cl_action.'d';
				$old = $a ? nl2br(check_plain($a->csa_room)) : '';
				$new = '';
			}
			else
				$what = $ctx.' &mdash; '.$what.($r->cl_action == 'create' ? ' (added)' : '');
		}
		elseif ($r->cl_area == 'access')
		{
			$who = isset($access[$r->cl_record]) ? check_plain($access[$r->cl_record]) : 'mapping '.(int) $r->cl_record;
			if ($r->cl_action == 'create')
				$what = 'Mapped to this centre';
			elseif ($r->cl_action == 'delete' || $r->cl_action == 'restore')
			{
				$what = $r->cl_action == 'delete' ? 'Removed from this centre' : 'Mapped again';
				$old = $new = '';
			}
			else
				$what = 'Mapping changed: '.$what;
			$what .= ' &mdash; '.$who;
		}

		$rows[] = array(
			date('d M Y H:i', strtotime($r->cl_tstamp)),
			isset($areas[$r->cl_area]) ? $areas[$r->cl_area] : check_plain($r->cl_area),
			$what,
			array('data' => $old, 'class' => array('cl-val')),
			array('data' => $new, 'class' => array('cl-val')),
			check_plain($r->name ? $r->name : 'user '.$r->cl_user),
		);
	}
	$out = theme('table', array('header' => array('When', 'Section', 'What', 'Old', 'New', 'By'), 'rows' => $rows, 'attributes' => array('class' => array('dh-centre-log'))));
	if ($more)
		$out .= '<p>Showing the latest 500 changes.</p>';
	return $out;
}
