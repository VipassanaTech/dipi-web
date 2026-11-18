<?php
/* Step 1 move of referral person details (design "Referral Person Details", section "Moving existing referrals").
   Loaded only by deploy/2026-11-referral-person-move.php and tests/test-referral-move.php.
   $dry = count only, write nothing. $ids = NULL for every referral, or an array of referral ids (tests). */

// The backup of each referral's old r_student. DDL commits: never call this inside a transaction.
function dh_refmove_prepare()
{
	db_query('CREATE TABLE IF NOT EXISTS dh_referral_move_bak (
		r_id BIGINT(20) NOT NULL, old_student BIGINT(20) NOT NULL, moved_at DATETIME NOT NULL, PRIMARY KEY (r_id)
	) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci');
}

function _dh_refmove_ids($ids, $col)
{
	if ($ids === NULL)
		return array('', array());
	return array(" and $col in (:ids)", array(':ids' => $ids ? array_values(array_map('intval', $ids)) : array(0)));
}

// Copy each not-yet-moved referral's copy (dh_student + its lowest course-count row) into the r_* columns.
function dh_refmove_details($dry, $ids = NULL)
{
	list($w, $args) = _dh_refmove_ids($ids, 'r.r_id');
	$sum = array(
		'to_move' => (int) db_query("select count(*) from dh_referral r join dh_student s on s.s_id = r.r_student where r.r_moved is null $w", $args)->fetchField(),
		'no_copy' => (int) db_query("select count(*) from dh_referral r left join dh_student s on s.s_id = r.r_student where r.r_moved is null and s.s_id is null $w", $args)->fetchField(),
		'already_moved' => (int) db_query("select count(*) from dh_referral r where r.r_moved is not null $w", $args)->fetchField());
	if ($dry)
		return $sum;
	$now = date('Y-m-d H:i:s');
	db_query("insert ignore into dh_referral_move_bak (r_id, old_student, moved_at)
		select r.r_id, r.r_student, :now from dh_referral r join dh_student s on s.s_id = r.r_student where r.r_moved is null $w", array(':now' => $now) + $args);
	$set = array();
	foreach (dh_referral_person_map() as $f => $c)
		if (strpos($f, 'sci_') === 0)
			$set[] = "r.$c = ifnull(i.$f, 0)";
		elseif ($f === 's_dob')
			$set[] = "r.r_dob = case when s.s_dob < '1900-01-01' then null else s.s_dob end";     // a zero date is no date
		else
			$set[] = "r.$c = s.$f";
	$set[] = 'r.r_photo = s.s_photo';
	$set[] = 'r.r_moved = :now';
	$sum['moved'] = db_query("update dh_referral r join dh_student s on s.s_id = r.r_student
		left join (select sci_student, min(sci_id) as sci_id from dh_student_course_input
			where sci_student in (select r_student from dh_referral) group by sci_student) m on m.sci_student = r.r_student
		left join dh_student_course_input i on i.sci_id = m.sci_id
		set " . implode(', ', $set) . " where r.r_moved is null $w", array(':now' => $now) + $args)->rowCount();
	return $sum;
}

// Link each moved referral to the person's real student record, and record the old copies in dh_referral_copy.
// Pass 1: a real record (not any referral copy) = twin; else a copy holding course records = own (linked to itself).
// Pass 2, for the rest: the same, but the own copies now count as real records (a person referred twice).
function dh_refmove_links($dry, $ids = NULL)
{
	list($w, $args) = _dh_refmove_ids($ids, 'r.r_id');
	$refs = db_query("select r.*, b.old_student from dh_referral r join dh_referral_move_bak b on b.r_id = r.r_id
		where r.r_moved is not null $w order by r.r_id", $args)->fetchAll(PDO::FETCH_ASSOC);
	$all = array();
	foreach ($refs as $r)
		if ($r['old_student'])
			$all[(int) $r['old_student']] = TRUE;
	$all = array_keys($all);
	$link = $group = $own = array();
	foreach ($refs as $r)
	{
		$id = (int) $r['r_id'];
		$copy = (int) $r['old_student'];
		$twin = dh_referral_link_find(dh_referral_who_row($r), $all);
		if ($twin)
		{
			$link[$id] = $twin;
			$group[$id] = 'twin';
		}
		elseif ($copy && db_query('select 1 from dh_student_course where sc_student = :s limit 1', array(':s' => $copy))->fetchField())
		{
			$link[$id] = $copy;
			$group[$id] = 'own';
			$own[$copy] = TRUE;
		}
	}
	$skip = array_values(array_diff($all, array_keys($own)));
	foreach ($refs as $r)
	{
		$id = (int) $r['r_id'];
		if (isset($group[$id]))
			continue;
		$link[$id] = dh_referral_link_find(dh_referral_who_row($r), $skip);
		$group[$id] = $link[$id] ? 'twin' : 'none';
	}
	$sum = array('links_changed' => 0, 'copies_recorded' => 0);
	$record = array();                                           // copy => array(referral, group)
	foreach ($refs as $r)
	{
		$id = (int) $r['r_id'];
		$k = $group[$id] . '_' . ($r['r_deleted'] ? 'deleted' : (dh_referral_end_ok($r['r_end']) ? 'active' : 'inactive'));
		$sum[$k] = (isset($sum[$k]) ? $sum[$k] : 0) + 1;
		if ((int) $r['r_student'] !== $link[$id])
			$sum['links_changed']++;
		$copy = (int) $r['old_student'];
		if ($copy && !isset($own[$copy]) && !isset($record[$copy]))
			$record[$copy] = array($id, $group[$id]);
	}
	$sum['copies_to_record'] = count($record);
	if ($dry)
		return $sum;
	$txn = db_transaction();
	try
	{
		foreach ($refs as $r)
		{
			$id = (int) $r['r_id'];
			dh_referral_set_link($id, $link[$id], 'move');
			if (!db_query("select 1 from dh_referral_log where rl_referral = :r and rl_event = 'Moved' limit 1", array(':r' => $id))->fetchField())
				add_referral_activity($id, 'Moved', 'Details moved to the referral (' . $group[$id] . ')');
		}
		$now = date('Y-m-d H:i:s');
		foreach ($record as $copy => $x)
			$sum['copies_recorded'] += db_query('insert ignore into dh_referral_copy (rc_student, rc_referral, rc_group, rc_status, rc_created) values (:s, :r, :g, :st, :t)',
				array(':s' => $copy, ':r' => $x[0], ':g' => $x[1], ':st' => 'open', ':t' => $now))->rowCount();
	}
	catch (Exception $e)
	{
		$txn->rollback();
		throw $e;
	}
	return $sum;
}

// Back to the old way, before step 2 only: referrals point at their copies again, edits made since go back into the copies,
// referrals added since get a copy, and the open copy rows go. r_* values and r_moved stay (the old code ignores them).
function dh_refmove_rollback($dry, $ids = NULL)
{
	list($w, $args) = _dh_refmove_ids($ids, 'r.r_id');
	$sum = array(
		'relink' => (int) db_query("select count(*) from dh_referral r join dh_referral_move_bak b on b.r_id = r.r_id where r.r_student <> b.old_student $w", $args)->fetchField(),
		'saved_since' => (int) db_query("select count(*) from dh_referral r join dh_referral_move_bak b on b.r_id = r.r_id where r.r_updated > b.moved_at $w", $args)->fetchField(),
		'added_since' => (int) db_query("select count(*) from dh_referral r left join dh_referral_move_bak b on b.r_id = r.r_id where b.r_id is null and r.r_moved is not null $w", $args)->fetchField());
	if ($dry)
		return $sum;
	$s_set = $i_set = array();
	foreach (dh_referral_person_map() as $f => $c)
		if (strpos($f, 'sci_') === 0)
			$i_set[] = "i.$f = r.$c";
		else
			$s_set[] = "s.$f = r.$c";
	$txn = db_transaction();
	try
	{
		db_query("update dh_student s join dh_referral_move_bak b on b.old_student = s.s_id join dh_referral r on r.r_id = b.r_id
			set " . implode(', ', $s_set) . " where r.r_updated > b.moved_at $w", $args);
		db_query("update dh_student_course_input i join dh_referral_move_bak b on b.old_student = i.sci_student join dh_referral r on r.r_id = b.r_id
			set " . implode(', ', $i_set) . " where r.r_updated > b.moved_at $w", $args);
		db_query("update dh_referral r join dh_referral_move_bak b on b.r_id = r.r_id set r.r_student = b.old_student where r.r_student <> b.old_student $w", $args);
		foreach (db_query("select r.* from dh_referral r left join dh_referral_move_bak b on b.r_id = r.r_id where b.r_id is null and r.r_moved is not null $w", $args)->fetchAll(PDO::FETCH_ASSOC) as $r)
		{
			$s = array('s_created_by' => (int) $r['r_created_by'], 's_updated_by' => (int) $r['r_updated_by'], 's_created' => $r['r_created'], 's_updated' => $r['r_updated']);
			$sci = array();
			foreach (dh_referral_person_map() as $f => $c)
				if (strpos($f, 'sci_') === 0)
					$sci[$f] = (int) $r[$c];
				else
					$s[$f] = $r[$c];
			$sid = (int) db_insert('dh_student')->fields($s)->execute();
			db_insert('dh_student_course_input')->fields($sci + array('sci_student' => $sid))->execute();
			db_update('dh_referral')->fields(array('r_student' => $sid))->condition('r_id', $r['r_id'])->execute();
		}
		list($wc, $argc) = _dh_refmove_ids($ids, 'rc_referral');
		db_query("delete from dh_referral_copy where rc_status = 'open' $wc", $argc);
	}
	catch (Exception $e)
	{
		$txn->rollback();
		throw $e;
	}
	return $sum;
}
