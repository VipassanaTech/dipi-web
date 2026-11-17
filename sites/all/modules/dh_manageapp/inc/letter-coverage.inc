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
