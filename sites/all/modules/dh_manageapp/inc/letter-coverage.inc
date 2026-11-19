<?php
/**
 * Letter coverage: which letter DIPI sends for a status and a course type, and the
 * warnings when a letter change would leave a status + course type with no letter.
 *
 * The rule is dh_get_letter()'s when no letter is picked by hand: l_center = centre,
 * l_event = status, l_course_type in (type, 0), l_deleted = 0, order by l_course_type
 * desc, l_name asc, first row; a letter with an empty subject = no letter. The database
 * compares and sorts (case, spaces at the end, name order), as at send time.
 * dh_get_letter() and dh_send_letter() are not changed by this file.
 */

/** The statuses whose letters are checked, in display order (Review included; no Custom, no long-course review statuses). */
function dh_letter_cover_statuses()
{
	$out = &drupal_static(__FUNCTION__);
	if ($out !== NULL)
		return $out;
	$sys = db_query("select td_key, td_val1 from dh_type_detail where td_type = 'COURSE-SYSTEM-STATUS'")->fetchAllKeyed();
	$list = array();
	foreach (array('STATUS-RECEIVED', 'STATUS-CONFIRMED', 'STATUS-EXPECTED', 'STATUS-RECONFIRMATION', 'STATUS-CLARIFICATIONRESPONSE',
		'STATUS-CANCELLED', 'STATUS-LEFT', 'STATUS-ATTENDED') as $k)
		if (!empty($sys[$k]))
			$list[] = $sys[$k];
	foreach (db_query("select td_key from dh_type_detail where td_type = 'COURSE-STATUS' order by td_val2, td_id")->fetchCol() as $k)
		$list[] = $k;
	$drop = array('custom', 'r-atreview', 'a-atreview');
	if (!empty($sys['STATUS-CUSTOM']))
		$drop[] = _dh_lcov_key($sys['STATUS-CUSTOM']);
	foreach (db_query("select td_key from dh_type_detail where td_type = 'COURSE-LC-STATUS'")->fetchCol() as $k)
		$drop[] = _dh_lcov_key($k);
	$out = array();
	$seen = array();
	foreach ($list as $s)
	{
		$k = _dh_lcov_key($s);
		if ($k === '' || isset($seen[$k]) || in_array($k, $drop, TRUE))
			continue;
		$seen[$k] = TRUE;
		$out[] = $s;
	}
	return $out;
}

/* A status as the database compares it here: upper/lower case and spaces at the end do not count. */
function _dh_lcov_key($s)
{
	return strtolower(rtrim((string) $s));
}

/* TRUE for Review: a letter there is the warning, a missing letter is fine. */
function dh_letter_cover_is_review($status)
{
	return _dh_lcov_key($status) === 'review';
}

/* TRUE for Left and Clarification-Response: most centres send no letter for them, so a missing letter is fine.
   The Letter check shows "No letter (optional)" in grey, not counted; a change is not held; the search page shows
   no note (js/search-letter-note.js). A centre may still add a letter: it is sent as before. */
function dh_letter_cover_is_optional($status)
{
	$keys = &drupal_static(__FUNCTION__);
	if ($keys === NULL)
	{
		$sys = db_query("select td_key, td_val1 from dh_type_detail where td_type = 'COURSE-SYSTEM-STATUS'")->fetchAllKeyed();
		$keys = array('left', 'clarification-response');
		foreach (array('STATUS-LEFT', 'STATUS-CLARIFICATIONRESPONSE') as $k)
			if (!empty($sys[$k]) && !in_array(_dh_lcov_key($sys[$k]), $keys, TRUE))
				$keys[] = _dh_lcov_key($sys[$k]);
	}
	return in_array(_dh_lcov_key($status), $keys, TRUE);
}

/**
 * The centre's active letters for $status in the order DIPI tries them, each
 * array(l_id, l_name, l_course_type, l_subject). $override = one letter as it would be
 * after a change (l_id 0 = a new letter): it takes the place of the stored row. The new
 * values go through the same SQL, so the database still compares and sorts them; the
 * numbers are CAST so the course type still sorts as a number.
 */
function _dh_lcov_candidates($centre, $status, $override = NULL)
{
	$args = array(':c' => (int) $centre, ':e' => (string) $status);
	$from = 'dh_letter';
	if ($override !== NULL)
	{
		$from = "(select l_id, l_center, l_event, l_course_type, l_name, l_subject, l_deleted from dh_letter where l_center = :c1 and l_id <> :oid
			union all select cast(:nid as signed), cast(:nc as signed), :nev, cast(:nct as signed), :nnm, :nsb, cast(:ndel as signed)) x";
		$args += array(':c1' => (int) $centre, ':oid' => (int) $override['l_id'], ':nid' => (int) $override['l_id'], ':nc' => (int) $centre,
			':nev' => (string) $override['l_event'], ':nct' => (int) $override['l_course_type'], ':nnm' => (string) $override['l_name'],
			':nsb' => (string) $override['l_subject'], ':ndel' => (int) $override['l_deleted']);
	}
	return db_query("select l_id, l_name, l_course_type, l_subject from $from where l_center = :c and l_event = :e and l_deleted = 0
		order by l_course_type desc, l_name asc", $args)->fetchAll(PDO::FETCH_ASSOC);
}

/* The letter DIPI sends for course type $ctype from $cands (in DIPI's order), or NULL. */
function dh_letter_cover_pick($cands, $ctype)
{
	foreach ($cands as $r)
	{
		if ($r['l_course_type'] === NULL)
			continue;
		$t = (int) $r['l_course_type'];
		if ($t === (int) $ctype || $t === 0)
			return ($r['l_subject'] == '') ? NULL : $r;   // the send code stops at an empty subject too
	}
	return NULL;
}

/**
 * For each checked status and each course type the centre runs: the letter DIPI sends,
 * array('l_id' => int, 'l_name' => string), or NULL for no letter.
 */
function dh_letter_coverage($centre, $override = NULL)
{
	$types = dh_letter_course_types_to_cover($centre, '', 0);
	$out = array();
	foreach (dh_letter_cover_statuses() as $status)
	{
		$cands = _dh_lcov_candidates($centre, $status, $override);
		foreach ($types as $tid => $tname)
		{
			$p = dh_letter_cover_pick($cands, $tid);
			$out[$status][$tid] = $p ? array('l_id' => (int) $p['l_id'], 'l_name' => $p['l_name']) : NULL;
		}
	}
	return $out;
}

/**
 * What to warn about before a letter change at $centre.
 * $action: 'add', 'copy', 'edit', 'inactive' or 'active'. $id: the letter for edit, inactive
 * and active (not used for add and copy). $fields: the new form values for add, copy and edit
 * (any of l_event, l_course_type, l_name, l_subject).
 * Returns array('gaps' => array(status => array(type id => type name)), 'review' => bool):
 * gaps = status + course types that have a letter now and would have none after the change;
 * review = after the change the letter is active with status Review.
 * Never held: the template centre 0, another centre's letter, a locked long-course letter.
 */
function dh_letter_change_warnings($centre, $action, $id, $fields = array())
{
	$none = array('gaps' => array(), 'review' => FALSE);
	$centre = (int) $centre;
	if ($centre <= 0)
		return $none;
	if (in_array($action, array('edit', 'inactive', 'active'), TRUE))
	{
		$new = db_query("select l_id, l_event, l_course_type, l_name, l_subject, l_deleted from dh_letter where l_id = :l and l_center = :c",
			array(':l' => (int) $id, ':c' => $centre))->fetchAssoc();
		if (!$new || dh_letter_lc_role($new) !== NULL)
			return $none;
	}
	elseif (in_array($action, array('add', 'copy'), TRUE))
		$new = array('l_id' => 0, 'l_event' => '', 'l_course_type' => 0, 'l_name' => '', 'l_subject' => '', 'l_deleted' => 0);
	else
		return $none;

	if ($action == 'inactive')
		$new['l_deleted'] = 1;
	elseif ($action == 'active')
		$new['l_deleted'] = 0;
	else
		foreach (array('l_event', 'l_course_type', 'l_name', 'l_subject') as $k)
			if (isset($fields[$k]))
				$new[$k] = $fields[$k];
	if (dh_letter_lc_role($new) !== NULL)
		return $none;

	$out = $none;
	$out['review'] = empty($new['l_deleted']) && dh_letter_cover_is_review($new['l_event']);
	$types = dh_letter_course_types_to_cover($centre, '', 0);
	$before = dh_letter_coverage($centre);
	$after = dh_letter_coverage($centre, $new);
	foreach ($before as $status => $bytype)
	{
		if (dh_letter_cover_is_review($status) || dh_letter_cover_is_optional($status))
			continue;
		foreach ($bytype as $tid => $l)
			if ($l !== NULL && $after[$status][$tid] === NULL)
				$out['gaps'][$status][$tid] = $types[$tid];
	}
	return $out;
}

/* TRUE when a change has something to warn about. */
function dh_letter_has_warnings($w)
{
	return !empty($w['gaps']) || !empty($w['review']);
}

/**
 * The red message for a held change (HTML; status and course type names escaped).
 * $how: 'save' (letter form), 'inactive' or 'active' (Letters list links).
 */
function dh_letter_warnings_html($w, $how = 'save')
{
	$parts = array();
	if (!empty($w['gaps']))
	{
		$lines = '';
		foreach ($w['gaps'] as $status => $types)
			$lines .= '<li><strong>'.check_plain($status).'</strong> &ndash; '.check_plain(implode(', ', $types)).'</li>';
		$lead = ($how == 'inactive') ? t('If this letter is made inactive, these will get <strong>no letter</strong>:')
			: t('After this change these will get <strong>no letter</strong>:');
		$parts[] = '<p>'.$lead.'</p><ul class="lcov-gaps">'.$lines.'</ul><p>'.t('To keep them covered, undo this change or make another letter for them.').'</p>';
	}
	if (!empty($w['review']))
		$parts[] = '<p>'.t('Review is an internal status. This letter will be emailed to every applicant moved to Review.').'</p>';
	if ($how == 'inactive')
		$parts[] = '<p>'.t('Click <strong>Make inactive anyway</strong> to do it.').'</p>';
	elseif ($how == 'active')
		$parts[] = '<p>'.t('Click <strong>Make active anyway</strong> to do it.').'</p>';
	else
		$parts[] = '<p>'.t('To keep this change, tick <strong>Save anyway</strong> below and save again. If you chose an attachment, choose it again.').'</p>';
	return '<div class="lcov-warn">'.implode('', $parts).'</div>';
}

/* The warnings in one plain line, for the watchdog record. */
function dh_letter_warnings_text($w)
{
	$bits = array();
	foreach ($w['gaps'] as $status => $types)
		$bits[] = $status.': '.implode(', ', $types);
	if (!empty($w['review']))
		$bits[] = 'letter for Review';
	return implode('; ', $bits);
}

/* One watchdog row when someone goes ahead anyway (watchdog records who and when). */
function dh_letter_gap_record($centre, $letter_id, $action, $w)
{
	watchdog('letter_gap', 'Done anyway at centre @c, letter @l (@a): @w',
		array('@c' => (int) $centre, '@l' => (int) $letter_id, '@a' => $action, '@w' => dh_letter_warnings_text($w)), WATCHDOG_NOTICE);
}

/**
 * The "Letter check" box on the Letters page: every checked status against every course type
 * the centre runs, with the letter DIPI sends or "No letter". Folded by default; the summary
 * line gives the result. Review is the last row and is never red. '' for the template centre 0
 * and for a centre with no course types.
 */
function dh_letter_check_box($centre)
{
	$centre = (int) $centre;
	if ($centre <= 0)
		return '';
	$types = dh_letter_course_types_to_cover($centre, '', 0);
	if (!$types)
		return '';
	$rows = array();
	$last = array();
	$n_bad = 0;
	foreach (dh_letter_coverage($centre) as $status => $bytype)
	{
		$review = dh_letter_cover_is_review($status);
		$optional = dh_letter_cover_is_optional($status);
		$row = array(array('data' => check_plain($status), 'header' => TRUE));
		$bad = FALSE;
		foreach ($types as $tid => $tname)
		{
			$l = $bytype[$tid];
			if ($l !== NULL)
				$row[] = l($l['l_name'], "letters/$centre/view/".$l['l_id']);
			elseif ($review)
				$row[] = '&ndash;';
			elseif ($optional)
				$row[] = array('data' => t('No letter (optional)'), 'class' => array('lcov-optional'));
			else
			{
				$row[] = array('data' => t('No letter'), 'class' => array('lcov-none'));
				$bad = TRUE;
			}
		}
		if ($bad)
			$n_bad++;
		if ($review)
			$last[] = $row;
		else
			$rows[] = $row;
	}
	$header = array(t('Status'));
	foreach ($types as $tname)
		$header[] = check_plain($tname);
	$summary = $n_bad
		? format_plural($n_bad, 'Letter check: 1 status has course types with no letter', 'Letter check: @count statuses have course types with no letter')
		: t('Letter check: every status has a letter');
	return '<details class="lcov"><summary class="'.($n_bad ? 'lcov-bad' : 'lcov-good').'">'.$summary.'</summary>'
		.'<p class="lcov-help">'.t('The letter DIPI sends for each status and each course type you run. "No letter" means the applicant gets no letter for that status. Review is an internal status, so it needs no letter. Left and Clarification-Response are optional: most centres send no letter for them.').'</p>'
		.'<div class="lcov-scroll">'.theme('table', array('header' => $header, 'rows' => array_merge($rows, $last), 'attributes' => array('class' => array('lcov-table')))).'</div></details>';
}
