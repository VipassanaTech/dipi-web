<?php
/**
 * Unified resource-allocation engine (cells + dining). See
 * docs/superpowers/specs/2026-09-13-unified-allocation-engine-design.md.
 */

/** Resource descriptor: 'cell' or 'dining'. */
function dh_alloc_descriptor($resource) {
  if ($resource === 'cell') {
    return array('resource'=>'cell','name'=>'Cell','has'=>'cs_has_cells','config'=>'cs_cells_config',
      'main_col'=>'aa_cell','group_col'=>'aa_group_cell',
      'fixed_col'=>'aa_cell_fixed','group_fixed_col'=>'aa_group_cell_fixed',
      'batch_col'=>'aa_cell_batch');
  }
  return array('resource'=>'dining','name'=>'Dining','has'=>'cs_has_dining','config'=>'cs_dining_config',
    'main_col'=>'aa_dining','group_col'=>'aa_group_dining',
    'fixed_col'=>'aa_dining_fixed','group_fixed_col'=>'aa_group_dining_fixed',
    'batch_col'=>'aa_dining_batch');
}

/** The plan a course uses for a resource ('cell'|'dining'|'seat'): 'main' or
 *  'group'. Stored per course in c_<res>_plan (0=main default, 1=group). This is
 *  the single source of truth that drives the course page, lists, workbench
 *  default and the zero-day popup so a course shows only the plan it uses. */
function dh_course_plan($course, $resource) {
  $map = array('cell' => 'c_cell_plan', 'dining' => 'c_dining_plan', 'seat' => 'c_seat_plan');
  if (!isset($map[$resource])) return 'main';
  $v = db_query("select ".$map[$resource]." from dh_course where c_id=:c", array(':c' => (int) $course))->fetchField();
  return ((int) $v === 1) ? 'group' : 'main';
}

/** Persist a course's plan for a resource ('main'|'group'). */
function dh_course_plan_set($course, $resource, $plan) {
  $map = array('cell' => 'c_cell_plan', 'dining' => 'c_dining_plan', 'seat' => 'c_seat_plan');
  if (!isset($map[$resource])) return;
  db_update('dh_course')->fields(array($map[$resource] => ($plan === 'group') ? 1 : 0))->condition('c_id', (int) $course)->execute();
}

/** Expand "1-40, 55, A1-A5" into an array of labels (letter-prefix aware). */
function dh_alloc_expand($str) {
  $out = array();
  if ($str === '' || $str === null) return $out;
  foreach (explode(',', $str) as $v) {
    $v = trim($v);
    if ($v === '') continue;
    $p = explode('-', $v);
    if (count($p) > 1) {
      $prefix = preg_replace('/[0-9]/', '', $p[0]);
      $a = (int) preg_replace('/[A-Za-z-]/', '', $p[0]);
      $b = (int) preg_replace('/[A-Za-z-]/', '', $p[1]);
      for ($i = $a; $i <= $b; $i++) $out[] = $prefix . $i;
    } else {
      $out[] = $p[0];
    }
  }
  return $out;
}

/** Build a pool descriptor for one INI section.
 *  A "<Key>Rev = 1" flag reverses that range's expanded order (e.g. "1-100"
 *  with CellsRev becomes 100,99,...,1). Reserved has no reverse - it is only a
 *  set removed from the pool, never auto-assigned. */
function dh_alloc_pool($ini, $key) {
  $sec = (isset($ini[$key]) && is_array($ini[$key])) ? $ini[$key] : array();
  $expand = function($k) use ($sec) {
    $arr = dh_alloc_expand(isset($sec[$k]) ? $sec[$k] : '');
    if (!empty($sec[$k.'Rev'])) $arr = array_reverse($arr);
    return $arr;
  };
  $reserved = dh_alloc_expand(isset($sec['Reserved']) ? $sec['Reserved'] : '');
  $sep = (isset($sec['Old']) && $sec['Old'] !== '') || (isset($sec['New']) && $sec['New'] !== '');
  if ($sep) {
    return array('sep'=>true, 'cells'=>array(),
      'old'=>$expand('Old'),
      'new'=>$expand('New'),
      'reserved'=>$reserved);
  }
  return array('sep'=>false, 'old'=>array(), 'new'=>array(),
    'cells'=>$expand('Cells'),
    'reserved'=>$reserved);
}

/** True if the config defines any server section. */
function dh_alloc_has_server_config($ini) {
  if (!is_array($ini)) return false;
  foreach ($ini as $k => $v) {
    if (strpos($k, '_SERVER') !== false) return true;
  }
  return false;
}

/** Resolve the INI section key for one attendee, or null (server w/o section). */
function dh_alloc_effective_key($ini, $gender, $group, $class, $mode) {
  $base = (strtoupper($gender) === 'M') ? 'MALE' : 'FEMALE';
  if ($class === 'server') $base .= '_SERVER';
  if ($mode === 'group') {
    $g = (int) $group;
    if ($g > 0 && isset($ini[$base . '_' . $g])) return $base . '_' . $g;
  }
  if (isset($ini[$base])) return $base;
  // Students always resolve to their base (pool may be empty -> blank);
  // servers only when a server section exists.
  return ($class === 'server') ? null : $base;
}

/** Return human-readable warnings about a parsed config (empty = clean). */
function dh_alloc_validate($ini) {
  $warn = array();
  if (!is_array($ini)) return $warn;
  foreach ($ini as $key => $sec) {
    if (!is_array($sec)) continue;
    $avail = array_merge(
      dh_alloc_expand(isset($sec['Cells']) ? $sec['Cells'] : ''),
      dh_alloc_expand(isset($sec['Old']) ? $sec['Old'] : ''),
      dh_alloc_expand(isset($sec['New']) ? $sec['New'] : '')
    );
    $counts = array_count_values($avail);
    foreach ($counts as $v => $c) {
      if ($c > 1) $warn[] = "[$key] duplicate value '$v' in available list.";
    }
    foreach (dh_alloc_expand(isset($sec['Reserved']) ? $sec['Reserved'] : '') as $r) {
      if ($r !== '' && !in_array($r, $avail)) $warn[] = "[$key] reserved value '$r' is outside the available range (has no effect).";
    }
  }
  return $warn;
}

/**
 * Assign a shareable pool to an ordered list of items, cycling into batches.
 * Returns map item => array(seat_or_'', batch_index). Empty pool -> all ('',0).
 */
function dh_alloc_assign_batches($items, $pool) {
  $out = array();
  $n = count($pool);
  $i = 0;
  foreach ($items as $it) {
    if ($n === 0) { $out[$it] = array('', 0); continue; }
    $out[$it] = array($pool[$i % $n], intdiv($i, $n));
    $i++;
  }
  return $out;
}

/**
 * Allocate a resource to a course's attendees. ONE cell-style batch algorithm,
 * shared by cells AND dining:
 *  - seniority order, per gender/group section, old/new aware;
 *  - a combined "Cells" range is split into an old-portion (the first cells, one
 *    per non-fixed old student) and a new-portion (the rest); Old/New ranges are
 *    used directly when the section is split;
 *  - config-Reserved (keep-aside) and any Fixed value are removed from the pool;
 *    Fixed rows keep their value and consume neither a slot nor a batch step;
 *  - when a group has more people than pool seats the pool CYCLES into batches
 *    (recorded in batch_col when the resource has one - aa_cell_batch for cells;
 *    dining has no batch column so the seat number simply repeats).
 * Returns rows written. See docs/.../unified-allocation-engine-design.md.
 */
function dh_alloc_run($centre, $course, $desc, $mode) {
  $course = (int) $course;
  $col = ($mode === 'group') ? $desc['group_col'] : $desc['main_col'];
  $fixed_col = ($mode === 'group') ? $desc['group_fixed_col'] : $desc['fixed_col'];
  $batch_col = $desc['batch_col'];
  $cfg = db_query("select ".$desc['config']." from dh_center_setting where cs_center=:c", array(':c'=>$centre))->fetchField();
  $ini = ($cfg !== '' && $cfg !== null) ? parse_ini_string($cfg, true, INI_SCANNER_RAW) : array();
  if (!is_array($ini)) $ini = array();
  $servers = dh_alloc_has_server_config($ini);

  // Attendees in seniority/course order (students; + servers when eligible).
  $type_filter = $servers ? "and a_type in ('Student','Sevak')" : "and a_type='Student'";
  $q = "select aa_id, a_gender, a_old, aa_group, a_type, $col as cur, $fixed_col as fixed
    from dh_applicant
    left join dh_applicant_course on a_id=ac_applicant
    left join dh_applicant_attended on a_id=aa_applicant
    left join dh_teacher on (ac_teacher_code = CONCAT(t_code, '-', t_gender))
    where a_course='$course' and a_attended=1 $type_filter
    order by a_gender, IFNULL(ac_teacher,0) desc, IFNULL(t_seniority,0) desc, IFNULL(t_year_appointed,".date('Y')."),
      IFNULL(ac_60d,0) desc, IFNULL(ac_45d,0) desc, IFNULL(ac_30d,0) desc, IFNULL(ac_20d,0) desc,
      IFNULL(ac_tsc,0) desc, IFNULL(ac_spl,0) desc, IFNULL(ac_stp,0) desc, IFNULL(ac_10d,0) desc,
      IFNULL(ac_teen,0) desc, TIMESTAMPDIFF(YEAR, a_dob, CURDATE()) desc, a_id";
  $rows = db_query($q)->fetchAll();

  $classof = function($r){ return (strtolower($r->a_type) === 'sevak') ? 'server' : 'student'; };
  $keyof = function($r) use ($ini, $mode, $classof) {
    return dh_alloc_effective_key($ini, $r->a_gender, $r->aa_group, $classof($r), $mode);
  };
  $isfixed = function($r){ return $r->fixed && trim((string)$r->cur) !== ''; };

  // Fixed (non-empty) values are reserved out of their section's pool.
  $fixedvals = array();
  foreach ($rows as $r) {
    if ($isfixed($r)) { $k = $keyof($r); if ($k !== null) $fixedvals[$k][] = trim((string)$r->cur); }
  }
  // Bucket rows by section key -> old/new, preserving seniority order.
  $buckets = array();
  foreach ($rows as $r) {
    $k = $keyof($r);
    if ($k === null) continue;
    $buckets[$k][$r->a_old ? 'old' : 'new'][] = $r;
  }

  $updates = array();
  // Assign one old/new group around its pool, cycling into batches. Fixed rows
  // keep their value and neither take a seat nor advance the batch counter.
  $assign = function($group_rows, $pool) use (&$updates, $col, $batch_col, $isfixed) {
    $n = count($pool); $batch = 0; $count = 0;
    foreach ($group_rows as $r) {
      if ($n > 0 && $count >= $n) { $batch++; $count = 0; }
      if ($isfixed($r)) continue;
      $seat = ($n > 0) ? $pool[$count] : '';
      $f = array($col => ($seat === '' ? null : $seat));
      if ($batch_col) $f[$batch_col] = $batch;
      $updates[$r->aa_id] = $f;
      $count++;
    }
  };

  foreach ($buckets as $k => $on) {
    $old = isset($on['old']) ? $on['old'] : array();
    $new = isset($on['new']) ? $on['new'] : array();
    $p = dh_alloc_pool($ini, $k);
    $remove = array_merge($p['reserved'], isset($fixedvals[$k]) ? $fixedvals[$k] : array());
    $clean = function($arr) use ($remove) {
      return array_values(array_filter($arr, function($x) use ($remove) { return $x !== '' && !in_array($x, $remove); }));
    };
    if ($p['sep']) {
      $old_pool = $clean($p['old']);
      $new_pool = $clean($p['new']);
    } else {
      $cells = $clean($p['cells']);
      // Split the combined pool: old students take the first cells (one per
      // non-fixed old student), new students take the remainder.
      $need_old = 0; foreach ($old as $r) { if (!$isfixed($r)) $need_old++; }
      $old_pool = array_slice($cells, 0, $need_old);
      $new_pool = array_slice($cells, $need_old);
    }
    $assign($old, $old_pool);
    $assign($new, $new_pool);
  }

  foreach ($updates as $aa_id => $fields) {
    db_update('dh_applicant_attended')->fields($fields)->condition('aa_id', $aa_id)->execute();
  }
  return count($updates);
}
