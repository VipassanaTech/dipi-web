<?php
/**
 * One-time move of the automatic LC letters to their own status, LC Automatic (owner, 26 Nov 2026;
 * inc/letter-protect.inc DH_LETTER_LC_AUTO_STATUS). Before it they were "Custom" (and "Rejected" at 3 centres), so
 * centres saw them in the Custom letter lists. One-time code: the site itself does not load this file;
 * deploy/2026-11-26-lc-automatic-status.php and tests/test-lc-custom-lists.php do.
 */

/**
 * The letters to move: at every centre and the template centre 0, active and inactive, the LC letters DIPI finds by
 * name (the database compares the names, so capitals and end spaces do not count) that are not LC Automatic yet. A
 * letter with such a name under a review status is that review letter (dh_letter_lc_role) and stays. Rows: l_id,
 * l_center, l_event (the status before the move, which the undo puts back).
 */
function dh_lc_auto_targets()
{
	return db_query("select l_id, l_center, l_event from dh_letter where l_name in (:n) and l_event not in ('R-ATReview', 'A-ATReview', :s) order by l_id",
		array(':n' => dh_letter_lc_names(), ':s' => DH_LETTER_LC_AUTO_STATUS))->fetchAll(PDO::FETCH_ASSOC);
}

/* Give the letters of $rows (dh_lc_auto_targets) the status LC Automatic, as $uid, in one transaction. The Letter
   Change Log records each change. Returns how many letters changed. */
function dh_lc_auto_apply($rows, $uid)
{
	$now = date('Y-m-d H:i:s');
	$n = 0;
	$txn = db_transaction();
	try
	{
		foreach (array_chunk(array_map(function ($r) { return (int) $r['l_id']; }, $rows), 500) as $ids)
			$n += db_update('dh_letter')->fields(array('l_event' => DH_LETTER_LC_AUTO_STATUS, 'l_updated' => $now, 'l_updated_by' => (int) $uid))
				->condition('l_id', $ids, 'IN')->condition('l_event', DH_LETTER_LC_AUTO_STATUS, '<>')->execute();
	}
	catch (Exception $e)
	{
		$txn->rollback();
		throw $e;
	}
	return $n;
}

/* The undo: each letter of $rows (the backup saved before the move) gets its old status back, as $uid, only while it
   is still LC Automatic. One transaction. Returns how many letters changed. */
function dh_lc_auto_undo($rows, $uid)
{
	$now = date('Y-m-d H:i:s');
	$n = 0;
	$txn = db_transaction();
	try
	{
		foreach ($rows as $r)
			$n += db_update('dh_letter')->fields(array('l_event' => (string) $r['l_event'], 'l_updated' => $now, 'l_updated_by' => (int) $uid))
				->condition('l_id', (int) $r['l_id'])->condition('l_event', DH_LETTER_LC_AUTO_STATUS)->execute();
	}
	catch (Exception $e)
	{
		$txn->rollback();
		throw $e;
	}
	return $n;
}
