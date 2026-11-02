<?php
/**
 * One-time long-course (LC) letter fix (design "LC Letter Protection", 1 Nov 2026).
 * One-time code: the module does NOT include this file. Only deploy/2026-11-01-lc-letters-fix.php and
 * tests/test-lc-fix.php load it, after the Drupal bootstrap.
 *
 * Brings the template (centre 0) and every active centre to a complete, working set of the 7 LC letters:
 * a missing letter is added, an only-inactive one is made active, a recommending review letter that carries
 * the area status is corrected, and every review letter's typed link becomes [ReviewLink]. Every write is
 * stamped with the given user (System), so the Letter Change Log records it. The typed link it replaces is
 * built from the two review-link settings, so no address is written in code.
 */

define('DH_LC_FIX_DUMMY_CENTRE', 302);   // the test centre (Dummy Centre): left out
define('DH_LC_FIX_LINE', '<p>To review, please click on this link - [ReviewLink]</p>');

/* Active centres: real centres (not the test centre) with any course since 2024. */
function dh_lc_fix_centres()
{
	return array_map('intval', db_query("select c.c_id from dh_center c where c.c_id > 0 and c.c_type = 'center' and c.c_id <> :d
		and exists (select 1 from dh_course co where co.c_center = c.c_id and co.c_start >= '2024-01-01') order by c.c_id",
		array(':d' => DH_LC_FIX_DUMMY_CENTRE))->fetchCol());
}

/* The condition and value that find one LC role's letters: by status for the review letters, else by exact name. */
function _dh_lc_fix_where($key)
{
	$roles = dh_letter_lc_roles();
	return $roles[$key]['event'] !== NULL ? array('l_event = :v', $roles[$key]['event']) : array('l_name = :v', $roles[$key]['name']);
}

/* The text each LC role is copied from: the template's active letter, else the text most centres' active letters share. */
function dh_lc_fix_sources()
{
	$src = array();
	foreach (array_keys(dh_letter_lc_roles()) as $key)
	{
		list($w, $v) = _dh_lc_fix_where($key);
		$id = db_query("select l_id from dh_letter where l_center = 0 and l_deleted = 0 and $w order by l_id limit 1", array(':v' => $v))->fetchField();
		if (!$id)
			$id = db_query("select min(l_id) from dh_letter where l_center > 0 and l_deleted = 0 and $w group by md5(l_body) order by count(*) desc, min(l_id) limit 1", array(':v' => $v))->fetchField();
		$src[$key] = $id ? db_query("select l_name, l_event, l_course_type, l_subject, l_body, l_sms from dh_letter where l_id = :l", array(':l' => $id))->fetchAssoc() : NULL;
	}
	return $src;
}

/* DH_LC_FIX_LINE before the paragraph/div that holds the last [Footer], else at the end. */
function _dh_lc_fix_add_line($body)
{
	$pos = strrpos($body, '[Footer]');
	if ($pos === FALSE)
		return $body.DH_LC_FIX_LINE;
	$head = substr($body, 0, $pos);
	$cands = array();
	foreach (array('<p', '<div') as $tag)
		if (($i = strrpos($head, $tag)) !== FALSE)
			$cands[] = $i;
	$start = $cands ? max($cands) : $pos;
	return substr($body, 0, $start).DH_LC_FIX_LINE.substr($body, $start);
}

/* A review letter's text with [ReviewLink]: the typed link (setting + [RAuth]/[AAuth]) is replaced; text without it gets
   DH_LC_FIX_LINE. Returns array($body, $how), $how 'replace', 'add' or '' (already has [ReviewLink]). */
function dh_lc_fix_link_body($body, $event)
{
	$body = (string) $body;
	if (strpos($body, '[ReviewLink]') !== FALSE)
		return array($body, '');
	$step = $event == 'A-ATReview' ? 'a' : 'r';
	$typed = dh_letter_review_link($step, $step == 'a' ? '[AAuth]' : '[RAuth]');
	if ($typed !== '' && strpos($body, $typed) !== FALSE)
		return array(str_replace($typed, '[ReviewLink]', $body), 'replace');
	return array(_dh_lc_fix_add_line($body), 'add');
}

/**
 * What the fix would do for $centres (default: the template and every active centre), as a list of actions:
 * status (l_id), activate (key, l_id), add (key), link (l_id, how). array(array('do' => 'settings_missing')) when
 * a review-link setting is empty.
 */
function dh_lc_fix_plan($centres = NULL)
{
	if (dh_letter_review_link('r', '') === '' || dh_letter_review_link('a', '') === '')
		return array(array('do' => 'settings_missing'));
	if ($centres === NULL)
		$centres = array_merge(array(0), dh_lc_fix_centres());
	$plan = array();
	foreach ($centres as $c)
	{
		$c = (int) $c;
		// A recommending review letter that carries the area status (Mitra Upakram): correct its status.
		$wrong = array_map('intval', db_query("select l_id from dh_letter where l_center = :c and l_event = 'A-ATReview' and l_name = 'Recommending Review' and l_body like :r",
			array(':c' => $c, ':r' => '%[RAuth]%'))->fetchCol());
		foreach ($wrong as $id)
			$plan[] = array('do' => 'status', 'centre' => $c, 'l_id' => $id);
		foreach (array_keys(dh_letter_lc_roles()) as $key)
		{
			list($w, $v) = _dh_lc_fix_where($key);
			if (($key == 'r_review' && $wrong) || db_query("select 1 from dh_letter where l_center = :c and l_deleted = 0 and $w limit 1", array(':c' => $c, ':v' => $v))->fetchField())
				continue;
			$off = (int) db_query("select max(l_id) from dh_letter where l_center = :c and l_deleted = 1 and $w", array(':c' => $c, ':v' => $v))->fetchField();
			$plan[] = $off ? array('do' => 'activate', 'centre' => $c, 'key' => $key, 'l_id' => $off) : array('do' => 'add', 'centre' => $c, 'key' => $key);
		}
		// Every review letter (active or not, and the one corrected above) gets [ReviewLink].
		foreach (db_query("select l_id, l_event, l_body from dh_letter where l_center = :c and l_event in ('R-ATReview', 'A-ATReview') order by l_id", array(':c' => $c)) as $l)
		{
			list(, $how) = dh_lc_fix_link_body($l->l_body, in_array((int) $l->l_id, $wrong) ? 'R-ATReview' : $l->l_event);
			if ($how !== '')
				$plan[] = array('do' => 'link', 'centre' => $c, 'l_id' => (int) $l->l_id, 'how' => $how);
		}
	}
	return $plan;
}

/**
 * Apply a plan as $uid (System): status fixes and activations, then the link switches, then the added letters (copied
 * from dh_lc_fix_sources(), read after the switches, with [ReviewLink] ensured). Returns counts by action.
 */
function dh_lc_fix_apply($plan, $uid)
{
	$now = date('Y-m-d H:i:s');
	$n = array('status' => 0, 'activate' => 0, 'link' => 0, 'add' => 0);
	foreach ($plan as $a)
	{
		if ($a['do'] == 'status')
			db_update('dh_letter')->fields(array('l_event' => 'R-ATReview', 'l_updated' => $now, 'l_updated_by' => $uid))->condition('l_id', $a['l_id'])->execute();
		elseif ($a['do'] == 'activate')
			db_update('dh_letter')->fields(array('l_deleted' => 0, 'l_updated' => $now, 'l_updated_by' => $uid))->condition('l_id', $a['l_id'])->execute();
		else
			continue;
		$n[$a['do']]++;
	}
	foreach ($plan as $a)
	{
		if ($a['do'] != 'link')
			continue;
		$l = db_query("select l_event, l_body from dh_letter where l_id = :l", array(':l' => $a['l_id']))->fetchObject();
		list($body, $how) = dh_lc_fix_link_body($l->l_body, $l->l_event);
		if ($how === '')
			continue;
		db_update('dh_letter')->fields(array('l_body' => $body, 'l_updated' => $now, 'l_updated_by' => $uid))->condition('l_id', $a['l_id'])->execute();
		$n['link']++;
	}
	$src = NULL;
	foreach ($plan as $a)
	{
		if ($a['do'] != 'add')
			continue;
		if ($src === NULL)
			$src = dh_lc_fix_sources();
		if (empty($src[$a['key']]))
			throw new Exception('No source text for the LC letter '.$a['key']);
		$s = $src[$a['key']];
		if (in_array($s['l_event'], array('R-ATReview', 'A-ATReview')))
			list($s['l_body'], ) = dh_lc_fix_link_body($s['l_body'], $s['l_event']);
		db_insert('dh_letter')->fields(array('l_center' => $a['centre'], 'l_event' => $s['l_event'], 'l_course_type' => 0, 'l_name' => $s['l_name'],
			'l_subject' => $s['l_subject'], 'l_body' => $s['l_body'], 'l_sms' => $s['l_sms'], 'l_deleted' => 0,
			'l_created' => $now, 'l_created_by' => $uid, 'l_updated' => $now, 'l_updated_by' => $uid))->execute();
		$n['add']++;
	}
	return $n;
}
