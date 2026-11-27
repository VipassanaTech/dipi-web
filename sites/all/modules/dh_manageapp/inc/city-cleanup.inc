<?php
/**
 * One-time city clean-up (owner, 27 Nov 2026). Until the dipi-submit.php fix of 19 Nov 2026 a city with an apostrophe
 * (Cox's Bazar) was never found, so each application added it again; older code also saved some names with HTML
 * codes or a backslash (Alcarr&agrave;s, Connah\&#039;s Quay). This keeps one row per city (same country, state and
 * name, compared the way the database compares names: capitals, accents and end spaces do not count) with a clean
 * name, points every record at it and removes the copies. One-time code: the site itself does not load this file;
 * deploy/2026-11-27-city-cleanup.php and tests/test-city-cleanup.php do.
 */

/* Tables that keep a city id: table => array(row id column, city column). A table that is not there is skipped
   (dh_applicant_deleted is on prod only). Backup tables of earlier fixes (*_bak_*, *_map_*) are left as they are. */
function dh_city_cleanup_refs()
{
	return array(
		'dh_applicant' => array('a_id', 'a_city'),
		'dh_applicant_deleted' => array('a_id', 'a_city'),
		'dh_student' => array('s_id', 's_city'),
		'dh_patrika' => array('p_id', 'p_city'),
		'dh_pin_code' => array('pc_id', 'pc_city'),
		'dh_center' => array('c_id', 'c_city'),
		'dh_referral' => array('r_id', 'r_city'),
		'dh_lc_admin' => array('la_id', 'la_city'),
	);
}

/* The tables of dh_city_cleanup_refs() that exist here. */
function _dh_city_cleanup_tables()
{
	static $have = NULL;
	if ($have === NULL)
	{
		$have = array();
		foreach (dh_city_cleanup_refs() as $t => $c)
			if (db_table_exists($t))
				$have[$t] = $c;
	}
	return $have;
}

/* A city name without HTML codes (&eacute; &#039;, also when coded twice), backslashes or extra spaces. */
function dh_city_clean_name($name)
{
	$n = (string) $name;
	for ($i = 0; $i < 3 && strpos($n, '&') !== FALSE; $i++)
	{
		$d = html_entity_decode($n, ENT_QUOTES | ENT_HTML5, 'UTF-8');
		if ($d === $n)
			break;
		$n = $d;
	}
	$n = str_replace('\\', '', $n);
	$s = preg_replace('/\s+/u', ' ', $n);
	return trim($s === NULL ? $n : $s);
}

/* The key of a city: country, state and the database's sort key of the name (equal keys = the database finds one
   when looking for the other, as dipi-submit.php and the application form do). */
function _dh_city_key($country, $state, $name)
{
	static $coll = NULL;
	if ($coll === NULL)
	{
		$coll = (string) db_query("select collation_name from information_schema.columns where table_schema = database() and table_name = 'dh_city' and column_name = 'c_name'")->fetchField();
		if (!preg_match('/^[a-z0-9_]+$/', $coll))
			throw new Exception('Cannot read the collation of dh_city.c_name.');
	}
	$w = db_query("select hex(weight_string(convert(:n using utf8mb4) collate $coll))", array(':n' => $name))->fetchField();
	return drupal_strtoupper(trim((string) $country)) . '|' . drupal_strtoupper(trim((string) $state)) . '|' . $w;
}

/* The spelling a merged city keeps: the one most rows use, preferring one not written all in small or all in capital
   letters (Cox's Bazar over COX'S BAZAR and cox's bazar); on a tie the one with accents (São Paulo over Sao Paulo),
   then the oldest row's. */
function _dh_city_pick_name($names)
{
	$count = array();
	foreach ($names as $n)
		$count[$n] = isset($count[$n]) ? $count[$n] + 1 : 1;
	$best = NULL;
	foreach ($count as $n => $c)
	{
		$n = (string) $n;
		$mixed = (drupal_strtolower($n) !== $n && drupal_strtoupper($n) !== $n) ? 1 : 0;
		$score = array($mixed, $c, preg_match_all('/[^\x00-\x7F]/u', $n));
		if ($best === NULL || $score > $best[0])
			$best = array($score, $n);
	}
	return $best[1];
}

/**
 * What to do, one entry per city that has copies or a name to clean: array('keep' => the oldest row id, 'name' => the
 * name it gets, 'old_name' => its name now, 'drop' => the ids of its copies, 'country', 'state'). Looks at every city
 * whose name has an apostrophe, a backslash or an HTML code, together with the rows that already have its clean name.
 * $country limits it to one country (the tests use a made-up one).
 */
function dh_city_cleanup_plan($country = NULL)
{
	$where = "(c_name like :q or c_name like :b or c_name like :h)";
	$args = array(':q' => "%'%", ':b' => '%\\\\%', ':h' => '%&%;%');
	if ($country !== NULL)
	{
		$where .= " and c_country = :co";
		$args[':co'] = $country;
	}
	$rows = array();
	foreach (db_query("select c_id, c_country, c_state, c_name from dh_city where $where order by c_id", $args) as $r)
	{
		$clean = dh_city_clean_name($r->c_name);
		if ($clean === '')
			continue;
		$rows[$r->c_id] = $r;
		foreach (db_query("select c_id, c_country, c_state, c_name from dh_city where c_country = :co and c_state = :s and c_name = :n",
			array(':co' => $r->c_country, ':s' => $r->c_state, ':n' => $clean)) as $o)
			$rows[$o->c_id] = $o;
	}
	$groups = array();
	foreach ($rows as $id => $r)
	{
		$r->clean = dh_city_clean_name($r->c_name);
		if ($r->clean !== '')
			$groups[_dh_city_key($r->c_country, $r->c_state, $r->clean)][] = (int) $id;
	}
	$plan = array();
	foreach ($groups as $ids)
	{
		sort($ids);
		$keep = $ids[0];
		$drop = array_slice($ids, 1);
		$name = _dh_city_pick_name(array_map(function ($i) use ($rows) { return $rows[$i]->clean; }, $ids));
		if (!$drop && $name === $rows[$keep]->c_name)
			continue;
		$plan[] = array('keep' => $keep, 'name' => $name, 'old_name' => $rows[$keep]->c_name, 'drop' => $drop,
			'country' => $rows[$keep]->c_country, 'state' => $rows[$keep]->c_state);
	}
	usort($plan, function ($a, $b) { return $a['keep'] - $b['keep']; });
	return $plan;
}

/* The map copy id => kept id of $plan. */
function _dh_city_cleanup_map($plan)
{
	$to_keep = array();
	foreach ($plan as $g)
		foreach ($g['drop'] as $d)
			$to_keep[(int) $d] = (int) $g['keep'];
	return $to_keep;
}

/* The records that point at one of the cities $drop right now, found with a plain read (it locks nothing): table =>
   list of array('id', 'city'), dh_patrika also 'p_updated' and 'p_updated_by'. */
function _dh_city_cleanup_pointing($drop)
{
	$refs = array();
	foreach (_dh_city_cleanup_tables() as $t => $c)
	{
		$refs[$t] = array();
		$more = $t == 'dh_patrika' ? ', p_updated, p_updated_by' : '';
		foreach (array_chunk($drop, 500) as $ids)
			foreach (db_query("select $c[0] id, $c[1] city$more from $t where $c[1] in (:d) order by $c[0]", array(':d' => $ids))->fetchAll(PDO::FETCH_ASSOC) as $r)
				$refs[$t][] = $r;
	}
	return $refs;
}

/* Everything the undo needs, read before any change: the city rows of $plan (all columns) and, per table, the records
   that point at a copy (see _dh_city_cleanup_pointing). */
function dh_city_cleanup_snapshot($plan)
{
	$keep = array();
	foreach ($plan as $g)
		$keep[] = $g['keep'];
	$drop = array_keys(_dh_city_cleanup_map($plan));
	$snap = array('cities' => array(), 'refs' => array());
	foreach (array_chunk(array_merge($keep, $drop), 500) as $ids)
		foreach (db_query("select * from dh_city where c_id in (:i) order by c_id", array(':i' => $ids))->fetchAll(PDO::FETCH_ASSOC) as $r)
			$snap['cities'][] = $r;
	$snap['refs'] = _dh_city_cleanup_pointing($drop);
	return $snap;
}

/* Point the records $list of table $t at their kept city, by their own row ids, 200 at a time. Each statement is its
   own short step: it locks only those rows, for a moment. dh_patrika rows get updated time and user (its trigger logs
   the change by that user). Returns how many records changed. */
function _dh_city_cleanup_move($t, $list, $to_keep, $uid, $now)
{
	$tables = _dh_city_cleanup_tables();
	list($idc, $col) = $tables[$t];
	$by = array();
	foreach ($list as $r)
		if (isset($to_keep[(int) $r['city']]))
			$by[(int) $r['city']][] = (int) $r['id'];
	$n = 0;
	foreach ($by as $copy => $ids)
		foreach (array_chunk($ids, 200) as $chunk)
		{
			$f = array($col => $to_keep[$copy]);
			if ($t == 'dh_patrika')
				$f += array('p_updated' => $now, 'p_updated_by' => (int) $uid);
			$n += db_update($t)->fields($f)->condition($idc, $chunk, 'IN')->condition($col, $copy)->execute();
		}
	return $n;
}

/**
 * Do $plan as $uid WITHOUT long locks. On prod dh_applicant has no index on a_city, so one UPDATE by city reads and
 * locks the whole table, and inside one transaction that held up every application save (27 Nov 2026). So: the
 * records of $snap are moved by their own row ids, a few at a time; records that took a copy after $snap are found
 * with a plain read (no lock) and moved the same way; then one short step on dh_city alone deletes the copies and
 * gives the kept cities their clean names; a last read moves any record that took a copy in between. Safe to stop and
 * run again. Returns array('renamed', 'deleted', 'refs' => table => records moved, 'late' => table => records found
 * after $snap, as _dh_city_cleanup_pointing gives them, for the undo).
 */
function dh_city_cleanup_apply($plan, $snap, $uid)
{
	$to_keep = _dh_city_cleanup_map($plan);
	$tables = _dh_city_cleanup_tables();
	$now = date('Y-m-d H:i:s');
	$done = array('renamed' => 0, 'deleted' => 0, 'refs' => array_fill_keys(array_keys($tables), 0), 'late' => array());
	foreach ($snap['refs'] as $t => $list)
		if (isset($tables[$t]))
			$done['refs'][$t] += _dh_city_cleanup_move($t, $list, $to_keep, $uid, $now);
	$late = function () use (&$done, $to_keep, $uid, $now)
	{
		if ($to_keep)
			foreach (_dh_city_cleanup_pointing(array_keys($to_keep)) as $t => $list)
				if ($list)
				{
					$done['late'][$t] = array_merge(isset($done['late'][$t]) ? $done['late'][$t] : array(), $list);
					$done['refs'][$t] += _dh_city_cleanup_move($t, $list, $to_keep, $uid, $now);
				}
	};
	$late();
	$txn = db_transaction();
	try
	{
		foreach ($plan as $g)
		{
			if ($g['drop'])
				$done['deleted'] += db_delete('dh_city')->condition('c_id', $g['drop'], 'IN')->execute();
			if ($g['name'] !== $g['old_name'])
				$done['renamed'] += db_update('dh_city')->fields(array('c_name' => $g['name']))->condition('c_id', $g['keep'])->execute();
		}
	}
	catch (Exception $e)
	{
		$txn->rollback();
		throw $e;
	}
	unset($txn);   // the dh_city step is saved here
	$late();
	return $done;
}

/* The undo, from the backup array('plan', 'snap', 'late') written around dh_city_cleanup_apply: the deleted cities
   come back with their own ids and the kept cities get their old names back (one short step on dh_city), then each
   record points at its old city again, by its row id. A record or name changed since is left as it is (counted in
   'kept'). Returns counts. */
function dh_city_cleanup_undo($backup)
{
	$res = array('cities_back' => 0, 'names_back' => 0, 'refs_back' => 0, 'kept' => 0);
	$to_keep = _dh_city_cleanup_map($backup['plan']);
	$txn = db_transaction();
	try
	{
		foreach ($backup['snap']['cities'] as $r)
			if (isset($to_keep[(int) $r['c_id']]) && !db_query("select 1 from dh_city where c_id = :i", array(':i' => $r['c_id']))->fetchField())
			{
				db_insert('dh_city')->fields($r)->execute();
				$res['cities_back']++;
			}
		foreach ($backup['plan'] as $g)
			if ($g['name'] !== $g['old_name'])
			{
				$now = db_query("select c_name from dh_city where c_id = :i", array(':i' => $g['keep']))->fetchField();
				if ($now !== FALSE && (string) $now === $g['name'])
				{
					db_update('dh_city')->fields(array('c_name' => $g['old_name']))->condition('c_id', $g['keep'])->execute();
					$res['names_back']++;
				}
				else
					$res['kept']++;
			}
	}
	catch (Exception $e)
	{
		$txn->rollback();
		throw $e;
	}
	unset($txn);
	$tables = _dh_city_cleanup_tables();
	$lists = array($backup['snap']['refs'], isset($backup['late']) ? $backup['late'] : array());
	foreach ($lists as $refs)
		foreach ($refs as $t => $list)
		{
			if (!isset($tables[$t]))
				continue;
			list($idc, $col) = $tables[$t];
			foreach ($list as $r)
			{
				$f = array($col => $r['city']);
				if ($t == 'dh_patrika')
					$f += array('p_updated' => $r['p_updated'], 'p_updated_by' => $r['p_updated_by']);
				$n = db_update($t)->fields($f)->condition($idc, $r['id'])->condition($col, $to_keep[(int) $r['city']])->execute();
				$res[$n ? 'refs_back' : 'kept']++;
			}
		}
	return $res;
}
