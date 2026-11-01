<?php
/**
 * Letter change log viewer.
 *
 * dh_letter_log holds one row per changed letter or merge field (old -> new, who, when), written by
 * the triggers of deploy/2026-11-01-letter-change-log.php. This file only reads it: labels, readable
 * values and the "Change Log" section on the Letters page and on each letter's page.
 */

/* Field => array(label, kind). Labels follow the letter form and the Manage Fields page. */
function dh_letter_log_fields()
{
	return array(
		'l_name'        => array('Letter Name', 'text'),
		'l_event'       => array('Status', 'text'),
		'l_course_type' => array('Course Type', 'coursetype'),
		'l_subject'     => array('Subject', 'text'),
		'l_body'        => array('Letter Body', 'html'),
		'l_sms'         => array('SMS Text', 'lines'),
		'l_attachment'  => array('Attachment', 'file'),
		'l_center'      => array('Centre', 'centre'),
		'l_deleted'     => array('In use', 'inactive'),
		'lf_name'       => array('Field Name', 'text'),
		'lf_value'      => array('Field Value', 'lines'),
		'lf_center'     => array('Centre', 'centre'),
		'lf_deleted'    => array('In use', 'removed'),
	);
}

/* A stored text as plain lines: HTML becomes text (one line per paragraph, break, list item, row
   or heading), entities are decoded, spaces tidied and empty lines dropped. */
function _dh_letter_log_lines($value, $html)
{
	$value = (string) $value;
	if ($html)
	{
		$value = preg_replace('#<br\s*/?>|</(p|div|li|tr|h[1-6]|table|ul|ol|blockquote)>#i', "\n", $value);
		$value = html_entity_decode(strip_tags($value), ENT_QUOTES, 'UTF-8');
	}
	$out = array();
	foreach (preg_split('/\r\n|\r|\n/', $value) as $l)
	{
		$l = trim((string) preg_replace('/[ \t\x{00A0}]+/u', ' ', $l));
		if ($l !== '')
			$out[] = $l;
	}
	return $out;
}

/* Old and new text as HTML showing only the lines that changed: removed lines (red) on the old
   side, added lines (green) on the new side. A change of spacing, formatting or order says so. */
function _dh_letter_log_diff($old, $new, $html)
{
	$o = _dh_letter_log_lines($old, $html);
	$n = _dh_letter_log_lines($new, $html);
	$del = array_values(array_diff($o, $n));
	$add = array_values(array_diff($n, $o));
	if (!$del && !$add)
	{
		$same = '<em>(only spacing, formatting or the order of lines changed)</em>';
		return array($same, $same);
	}
	$fmt = function ($lines, $cls) {
		if (!$lines)
			return '<em>(nothing)</em>';
		$out = array();
		foreach ($lines as $l)
			$out[] = '<span class="'.$cls.'">'.check_plain($l).'</span>';
		return implode('<br>', $out);
	};
	return array($fmt($del, 'cl-del'), $fmt($add, 'cl-add'));
}

/* One stored value as readable HTML. $names: 'coursetype' and 'centre', each id => name. */
function _dh_letter_log_value($kind, $value, $names)
{
	if ($value === NULL || $value === '')
		return '<span class="cl-empty">(empty)</span>';
	switch ($kind)
	{
		case 'coursetype':
			if ((string) $value === '0')
				return 'All';
			return isset($names['coursetype'][$value]) ? check_plain($names['coursetype'][$value]) : check_plain($value);
		case 'centre':
			return isset($names['centre'][$value]) ? check_plain($names['centre'][$value]) : check_plain($value);
		case 'inactive':
			return (string) $value === '1' ? 'Inactive' : 'Active';
		case 'removed':
			return (string) $value === '1' ? 'Removed' : 'In use';
		case 'file':
			return check_plain(drupal_basename($value));
		case 'html':
		case 'lines':
			$n = count(_dh_letter_log_lines($value, $kind == 'html'));
			return '<span class="cl-empty">'.format_plural($n, 'text, 1 line', 'text, @count lines').'</span>';
		default:
			return check_plain($value);
	}
}

/**
 * The Change Log of a centre's letters and merge fields, or of one letter ($letter_id), newest first,
 * as display rows (ts, ord, item, what, old, new, by). A new letter or field (its 'create' rows) and
 * a letter removed from the database (its 'delete' rows) become one row each. At most 1,000 log rows
 * are used; $more says whether there were more.
 */
function dh_letter_log_rows($centre_id, $letter_id = NULL, &$more = NULL)
{
	$more = FALSE;
	if (!db_table_exists('dh_letter_log'))
		return array();
	$where = 'l.ll_center = :c';
	$args = array(':c' => $centre_id);
	if ($letter_id !== NULL)
	{
		$where .= " and l.ll_area = 'letter' and l.ll_record = :r";
		$args[':r'] = $letter_id;
	}
	$res = db_query("select l.*, u.name from dh_letter_log l left join users u on u.uid = l.ll_user
		where $where order by l.ll_tstamp desc, l.ll_id asc limit 1001", $args)->fetchAll();
	$more = count($res) > 1000;
	$res = array_slice($res, 0, 1000);

	$fields = dh_letter_log_fields();
	// Names for the ids in the log: course types and centres; and which letters still belong here.
	$ids = array('coursetype' => array(), 'centre' => array());
	$letters = array();
	foreach ($res as $r)
	{
		if ($r->ll_area == 'letter')
			$letters[$r->ll_record] = (int) $r->ll_record;
		$kind = isset($fields[$r->ll_field]) ? $fields[$r->ll_field][1] : '';
		if (isset($ids[$kind]))
			foreach (array($r->ll_old, $r->ll_new) as $v)
				if ($v !== NULL && ctype_digit((string) $v))
					$ids[$kind][$v] = (int) $v;
	}
	$names = array('coursetype' => array(), 'centre' => array());
	if ($ids['coursetype'])
		$names['coursetype'] = db_query("select td_id, td_val1 from dh_type_detail where td_id in (:ids)", array(':ids' => array_values($ids['coursetype'])))->fetchAllKeyed();
	if ($ids['centre'])
		$names['centre'] = db_query("select c_id, c_name from dh_center where c_id in (:ids)", array(':ids' => array_values($ids['centre'])))->fetchAllKeyed();
	$live = $letters ? db_query("select l_id, l_id from dh_letter where l_id in (:ids) and l_center = :c",
		array(':ids' => array_values($letters), ':c' => $centre_id))->fetchAllKeyed() : array();

	$rows = array();
	$group = NULL;
	foreach ($res as $r)
	{
		$f = isset($fields[$r->ll_field]) ? $fields[$r->ll_field] : array($r->ll_field, 'text');
		$by = check_plain($r->name ? $r->name : ($r->ll_user ? 'user '.$r->ll_user : 'unknown'));
		if ($r->ll_area == 'field')
			$item = 'Merge field ['.check_plain($r->ll_name).']';
		else
			$item = isset($live[$r->ll_record]) ? l($r->ll_name, "letters/$centre_id/view/".$r->ll_record) : check_plain($r->ll_name);

		// A new letter / field, or a letter removed from the database: one row listing its fields.
		if ($r->ll_action == 'create' || ($r->ll_action == 'delete' && $r->ll_area == 'letter'))
		{
			$created = $r->ll_action == 'create';
			$part = check_plain($f[0]).': '._dh_letter_log_value($f[1], $created ? $r->ll_new : $r->ll_old, $names);
			$key = $r->ll_area.'|'.$r->ll_record.'|'.$r->ll_action.'|'.$r->ll_tstamp;
			if ($group === $key)
			{
				$rows[count($rows) - 1][$created ? 'new' : 'old'] .= '<br>'.$part;
				continue;
			}
			$group = $key;
			$label = $created ? ($r->ll_area == 'field' ? 'Added' : 'Created') : 'Removed from the database';
			$rows[] = array('ts' => $r->ll_tstamp, 'ord' => (int) $r->ll_id, 'item' => $item,
				'what' => '<span class="cl-act">'.$label.'</span>',
				'old' => $created ? '' : $part, 'new' => $created ? $part : '', 'by' => $by);
			continue;
		}
		$group = NULL;
		if ($r->ll_field == 'l_deleted')
			$what = '<span class="cl-act">'.((string) $r->ll_new === '1' ? 'Made inactive' : 'Made active').'</span>';
		elseif ($r->ll_field == 'lf_deleted')
			$what = '<span class="cl-act">'.((string) $r->ll_new === '1' ? 'Removed' : 'Added back').'</span>';
		else
			$what = check_plain($f[0]);
		if ($f[1] == 'html' || $f[1] == 'lines')
			list($old, $new) = _dh_letter_log_diff($r->ll_old, $r->ll_new, $f[1] == 'html');
		else
			list($old, $new) = array(_dh_letter_log_value($f[1], $r->ll_old, $names), _dh_letter_log_value($f[1], $r->ll_new, $names));
		$rows[] = array('ts' => $r->ll_tstamp, 'ord' => (int) $r->ll_id, 'item' => $item, 'what' => $what, 'old' => $old, 'new' => $new, 'by' => $by);
	}
	return $rows;
}

/**
 * The closed "Change Log" section: for the Letters page ($letter_id NULL: every letter and merge
 * field of the centre) or for one letter's page. '' for someone who cannot manage letters.
 */
function dh_letter_log_section($centre_id, $letter_id = NULL)
{
	if (!user_access('manage letters'))
		return '';
	$more = FALSE;
	$rows = dh_letter_log_rows($centre_id, $letter_id, $more);
	if (!$rows)
		$html = '<p>No changes recorded yet.</p>';
	else
	{
		$table = array();
		foreach ($rows as $r)
		{
			$line = array(date('d M Y H:i', strtotime($r['ts'])), $r['by']);
			if ($letter_id === NULL)
				$line[] = $r['item'];
			$line[] = $r['what'];
			$line[] = array('data' => $r['old'], 'class' => array('cl-val'));
			$line[] = array('data' => $r['new'], 'class' => array('cl-val'));
			$table[] = $line;
		}
		$header = $letter_id === NULL ? array('When', 'By', 'Letter or field', 'What', 'Old', 'New') : array('When', 'By', 'What', 'Old', 'New');
		$html = theme('table', array('header' => $header, 'rows' => $table, 'attributes' => array('class' => array('dh-centre-log', 'dh-course-log', 'dh-letter-log'))));
		if ($more)
			$html .= '<p class="cl-note">Showing the latest 1,000 changes.</p>';
	}
	$n = count($rows);
	drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/manageapp.css');
	$log = array('#type' => 'fieldset', '#id' => 'letter-change-log',
		'#title' => 'Change Log ('.($more ? '1,000+' : $n).' '.($n == 1 ? 'change' : 'changes').')',
		'#collapsible' => TRUE, '#collapsed' => TRUE,
		'log' => array('#markup' => $html));
	return drupal_render($log);
}
