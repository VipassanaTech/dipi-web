<?php
/* Step 2: merge or remove the old referral copies still open in dh_referral_copy (design "Referral Person Details",
   section "Cleaning up the old copies"). Loaded only by deploy/2026-11-referral-copy-cleanup.php and its test.
   Every change is a dh_student_log row with the batch id, so dh_refclean_undo($batch) puts it all back. */

// What to do with one open copy: merge (into the person's real record), keep (it is the only record with courses),
// remove (no course, no patrika, no referral), stay (something still points to it).
function dh_refclean_plan($copy)
{
	$copy = (int) $copy;
	if (db_query('select 1 from dh_referral where r_student = :s limit 1', array(':s' => $copy))->fetchField())
		return array('do' => 'stay', 'into' => 0, 'why' => 'a referral links to it');
	if (db_query('select 1 from dh_student_course where sc_student = :s limit 1', array(':s' => $copy))->fetchField())
	{
		$into = (int) db_query('select r.r_student from dh_referral_copy c join dh_referral r on r.r_id = c.rc_referral
			where c.rc_student = :s and r.r_student > 0 and r.r_student <> :s2', array(':s' => $copy, ':s2' => $copy))->fetchField();
		if (!$into)
		{
			$a = array();
			foreach (db_query('select * from dh_student where s_id = :s', array(':s' => $copy))->fetchAssoc() as $k => $v)
				if (strpos($k, 's_') === 0)
					$a['a_' . substr($k, 2)] = $v;
			$into = dh_referral_link_find($a);           // open copies are never found
		}
		return $into ? array('do' => 'merge', 'into' => $into, 'why' => 'the real record exists')
			: array('do' => 'keep', 'into' => 0, 'why' => 'the only record with courses');
	}
	if (db_query('select 1 from dh_patrika where p_student = :s limit 1', array(':s' => $copy))->fetchField())
		return array('do' => 'stay', 'into' => 0, 'why' => 'a patrika subscription points to it');
	return array('do' => 'remove', 'into' => 0, 'why' => 'no course, no patrika, no referral');
}

// Move everything of the copy to the real record, fill the real record's empty fields (decision 6), remove the copy.
function dh_refclean_merge($copy, $into, array $log)
{
	$copy = (int) $copy;
	$into = (int) $into;
	foreach (db_query('select * from dh_student_course where sc_student = :s', array(':s' => $copy))->fetchAll(PDO::FETCH_ASSOC) as $sc)
		if (db_query('select 1 from dh_student_course where sc_student = :t and sc_course = :c limit 1', array(':t' => $into, ':c' => $sc['sc_course']))->fetchField())
		{
			dh_student_log(array('sl_student' => $copy, 'sl_action' => 'copy-removed', 'sl_old' => json_encode($sc)) + $log);
			db_delete('dh_student_course')->condition('sc_id', $sc['sc_id'])->execute();
		}
		else
		{
			dh_student_log(array('sl_student' => $into, 'sl_action' => 'course-updated', 'sl_old' => json_encode($sc), 'sl_new' => json_encode(array('sc_student' => $into))) + $log);
			db_update('dh_student_course')->fields(array('sc_student' => $into))->condition('sc_id', $sc['sc_id'])->execute();
		}
	if (!db_query('select 1 from dh_student_course_input where sci_student = :t limit 1', array(':t' => $into))->fetchField()
		&& ($sci = db_query('select * from dh_student_course_input where sci_student = :s order by sci_id limit 1', array(':s' => $copy))->fetchAssoc()))
	{
		dh_student_log(array('sl_student' => $into, 'sl_action' => 'sci-updated', 'sl_old' => json_encode($sci), 'sl_new' => json_encode(array('sci_student' => $into))) + $log);
		db_update('dh_student_course_input')->fields(array('sci_student' => $into))->condition('sci_id', $sci['sci_id'])->execute();
	}
	foreach (db_query('select p_id from dh_patrika where p_student = :s', array(':s' => $copy))->fetchCol() as $pid)
	{
		dh_student_log(array('sl_student' => $copy, 'sl_action' => 'patrika-moved', 'sl_old' => (string) $pid, 'sl_new' => (string) $into) + $log);
		db_update('dh_patrika')->fields(array('p_student' => $into))->condition('p_id', $pid)->execute();
	}
	$from = db_query('select * from dh_student where s_id = :s', array(':s' => $copy))->fetchAssoc();
	$to = db_query('select * from dh_student where s_id = :t', array(':t' => $into))->fetchAssoc();
	$ids = array('s_aadhar' => 'aadhar', 's_pancard' => 'pancard', 's_voter_id' => 'voter_id', 's_passport' => 'passport');
	$set = array();
	foreach (dh_student_fields() as $f)
	{
		if (strpos($f, 's_') !== 0)
			continue;
		$v = trim((string) $from[$f]);
		$was = trim((string) $to[$f]);
		if ($f === 's_dob')
		{
			if (!dh_dob_known($v)) $v = '';
			if (!dh_dob_known($was)) $was = '';
		}
		if (isset($ids[$f]) && $v !== '' && dh_id_is_junk($ids[$f], $v))
			$v = '';
		if ($v === '' || $was !== '')
			continue;
		$set[$f] = $from[$f];
		dh_student_log(array('sl_student' => $into, 'sl_action' => 'updated', 'sl_field' => $f, 'sl_old' => (string) $to[$f], 'sl_new' => (string) $from[$f]) + $log);
	}
	if ($set)
		db_update('dh_student')->fields($set)->condition('s_id', $into)->execute();
	$sum = array('students_removed' => 0);
	return dh_student_remove_if_empty($copy, $log, $sum);
}

// Run the cleanup over the open copies ($only = these copy ids, for tests). $apply FALSE = count only.
function dh_refclean_run($apply, $only = NULL)
{
	global $user;
	$batch = 'refcopy-' . date('Ymd-His');
	$log = array('sl_batch' => $batch, 'sl_user' => (int) $user->uid);
	$w = '';
	$args = array();
	if ($only !== NULL)
	{
		$w = ' and rc_student in (:only)';
		$args[':only'] = $only ? array_values(array_map('intval', $only)) : array(0);
	}
	$sum = array('merge' => 0, 'keep' => 0, 'remove' => 0, 'stay' => 0, 'merged' => 0, 'removed' => 0, 'errors' => 0, 'unexpected' => 0, 'batch' => $apply ? $batch : '');
	foreach (db_query("select rc_student from dh_referral_copy where rc_status = 'open' $w order by rc_student", $args)->fetchCol() as $copy)
	{
		$p = dh_refclean_plan($copy);
		$sum[$p['do']]++;
		if (!$apply || $p['do'] === 'stay')
			continue;
		$txn = db_transaction();
		try
		{
			$status = 'kept';
			$res = NULL;
			if ($p['do'] === 'merge')
			{
				$res = dh_refclean_merge($copy, $p['into'], $log);
				if ($res === 'removed')
				{
					$status = 'merged';
					$sum['merged']++;
				}
			}
			elseif ($p['do'] === 'remove')
			{
				$s = array('students_removed' => 0);
				$res = dh_student_remove_if_empty($copy, $log, $s);
				if ($res === 'removed')
				{
					$status = 'removed';
					$sum['removed']++;
				}
			}
			if ($res !== NULL && $res !== 'removed')
			{
				$sum['unexpected']++;
				watchdog('referral', 'Copy cleanup: student @s was not removed (result: @r), kept', array('@s' => $copy, '@r' => is_scalar($res) ? (string) $res : gettype($res)), WATCHDOG_WARNING);
			}
			db_update('dh_referral_copy')->fields(array('rc_status' => $status, 'rc_batch' => $batch, 'rc_updated' => date('Y-m-d H:i:s')))->condition('rc_student', $copy)->execute();
		}
		catch (Exception $e)
		{
			$txn->rollback();
			$sum['errors']++;
			watchdog('referral', 'Copy cleanup failed for student @s: @m', array('@s' => $copy, '@m' => $e->getMessage()), WATCHDOG_ERROR);
		}
		unset($txn);
	}
	return $sum;
}

// Put a whole cleanup batch back (the student log undo), and open its copies again.
function dh_refclean_undo($batch)
{
	$res = dh_backfill_undo($batch, TRUE);
	db_update('dh_referral_copy')->fields(array('rc_status' => 'open', 'rc_batch' => NULL, 'rc_updated' => date('Y-m-d H:i:s')))->condition('rc_batch', $batch)->execute();
	return $res;
}
