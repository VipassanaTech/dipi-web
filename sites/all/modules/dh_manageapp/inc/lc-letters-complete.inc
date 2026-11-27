<?php
/**
 * One-time: every centre gets all ten LC letters (owner, 27 Nov 2026). Centres created before a letter was added to
 * the template centre 0 (LC Admin Notification, 1 Nov; the three teacher-change letters, 21 Nov) and centres that
 * never had LC letters get the ones they miss, copied from the template with its status (LC Automatic, or the review
 * status), as System. One-time code: the site itself does not load this file; deploy/2026-11-27-lc-letters-complete.php
 * and tests/test-lc-letters-complete.php do.
 */

/* How a centre's letter for LC role $key is found: the review letters by status, the others by exact name (the
   database compares, so capitals and end spaces do not count). Returns array(SQL condition, value). */
function _dh_lc_complete_where($key)
{
	$roles = dh_letter_lc_roles();
	return $roles[$key]['event'] !== NULL ? array('l_event = :v', $roles[$key]['event']) : array('l_name = :v', $roles[$key]['name']);
}

/**
 * What to add: one row per missing letter, array('centre', 'key', 'from' = the template letter id). A centre counts
 * when it has at least one letter. A letter counts as there when the centre has it at all, active or inactive (an
 * inactive LC letter is left as it is, never doubled). Throws when the template lacks an active letter with a subject.
 */
function dh_lc_complete_plan()
{
	$from = array();
	foreach (array_keys(dh_letter_lc_roles()) as $key)
	{
		list($w, $v) = _dh_lc_complete_where($key);
		$id = db_query("select l_id from dh_letter where l_center = 0 and l_deleted = 0 and l_subject <> '' and $w order by l_id limit 1", array(':v' => $v))->fetchField();
		if (!$id)
			throw new Exception("The template centre 0 has no active letter for the LC role $key.");
		$from[$key] = (int) $id;
	}
	$plan = array();
	foreach (db_query("select distinct l_center from dh_letter where l_center > 0 order by l_center")->fetchCol() as $c)
		foreach ($from as $key => $id)
		{
			list($w, $v) = _dh_lc_complete_where($key);
			if (!db_query("select 1 from dh_letter where l_center = :c and $w limit 1", array(':c' => $c, ':v' => $v))->fetchField())
				$plan[] = array('centre' => (int) $c, 'key' => $key, 'from' => $id);
		}
	return $plan;
}

/* Add the letters of $plan as $uid in one transaction: a copy of the template letter (status, course type, name,
   subject, text, SMS), active. The Letter Change Log records each. Returns the new letter ids. */
function dh_lc_complete_apply($plan, $uid)
{
	$now = date('Y-m-d H:i:s');
	$ids = array();
	$txn = db_transaction();
	try
	{
		foreach ($plan as $p)
		{
			$t = db_query("select l_event, l_course_type, l_name, l_subject, l_body, l_sms from dh_letter where l_id = :l and l_center = 0", array(':l' => $p['from']))->fetchAssoc();
			$ids[] = (int) db_insert('dh_letter')->fields(array('l_center' => $p['centre'], 'l_event' => $t['l_event'], 'l_course_type' => $t['l_course_type'],
				'l_name' => $t['l_name'], 'l_subject' => $t['l_subject'], 'l_body' => $t['l_body'], 'l_sms' => $t['l_sms'], 'l_attachment' => '', 'l_deleted' => 0,
				'l_created' => $now, 'l_created_by' => (int) $uid, 'l_updated' => $now, 'l_updated_by' => (int) $uid))->execute();
		}
	}
	catch (Exception $e)
	{
		$txn->rollback();
		throw $e;
	}
	return $ids;
}

/* The undo: delete the letters $ids added by dh_lc_complete_apply, only those nobody changed since (l_updated =
   l_created). Returns array(deleted, kept because changed). */
function dh_lc_complete_rollback($ids)
{
	$ids = array_map('intval', $ids);
	if (!$ids)
		return array(0, 0);
	$same = db_query("select l_id from dh_letter where l_id in (:ids) and l_updated = l_created", array(':ids' => $ids))->fetchCol();
	$n = $same ? db_delete('dh_letter')->condition('l_id', $same, 'IN')->execute() : 0;
	return array((int) $n, count($ids) - count($same));
}
