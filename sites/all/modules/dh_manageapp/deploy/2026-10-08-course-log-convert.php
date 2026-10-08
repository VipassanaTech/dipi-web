<?php
// ============================================================================
// Convert the OLD text course log (dh_log, l_module 'dh_course') into dh_course_log
// (one row per changed field). RUN ONCE after 2026-10-08-course-change-log.sql,
// from the Drupal root, on the command line only:
//     php7.3 sites/all/modules/dh_manageapp/deploy/2026-10-08-course-log-convert.php dry   (counts only, writes nothing)
//     php7.3 sites/all/modules/dh_manageapp/deploy/2026-10-08-course-log-convert.php run   (writes)
// "run" first deletes rows it converted earlier (crl_dh_log_id set), so it can be re-run safely.
// dh_log itself is never changed.
//
// What is converted: lines "CHANGE <Label> changed from <old> To <new> ...", written by the
// old dh_course_au trigger. Skipped:
//   - fields the new log does not keep: Processed, Finalized-Tstamp, URI (sync bookkeeping),
//     Name (built from type + description + dates), CID;
//   - lines with no field detail (2018-2021 "PUSH/PULL CHANGE", "name CHANGE", ...) and the
//     auto Full/Waitlist debug lines ("Gender: ..., ConfigCount ...").
// Who: the old line's l_user, except AT confirmed counts and Auto-Confirm, which only automatic
// jobs ever set (the old trigger credited them to whoever last saved the course): System.
// A value that itself contains " To " is split using the field's next change (or the course's
// current value); if that cannot decide, the whole text is kept as the new value (old empty).
// ============================================================================
if (php_sapi_name() !== 'cli') { echo "Command line only.\n"; exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1'; $_SERVER['HTTP_HOST'] = 'localhost'; $_SERVER['SCRIPT_NAME'] = '/index.php';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_DATABASE);

$mode = isset($argv[1]) ? $argv[1] : 'dry';
if (!in_array($mode, array('dry', 'run'))) { echo "Usage: php ... dry|run\n"; exit(1); }
$map = array(
  'CourseType' => 'c_course_type', 'Centre' => 'c_center', 'Cancelled' => 'c_cancelled', 'EnrolDate' => 'c_enrol_date',
  'DateChange' => 'c_date_change', 'End' => 'c_end', 'Start' => 'c_start', 'OM' => 'c_status_om', 'NM' => 'c_status_nm',
  'OF' => 'c_status_of', 'NF' => 'c_status_nf', 'ListOnly' => 'c_list_only', 'SVR M' => 'c_status_svr_m', 'SVR F' => 'c_status_svr_f',
  'Form-Langs' => 'c_form_langs', 'Comments' => 'c_comments', 'Description' => 'c_description', 'Deleted' => 'c_deleted',
  'Combined-Seat-Course' => 'c_combined_seat_course', 'AT-M-COUNT' => 'c_at_m_count', 'AT-F-COUNT' => 'c_at_f_count',
  'AT-M-CONF' => 'c_at_m_conf', 'AT-F-CONF' => 'c_at_f_conf', 'Finalized' => 'c_finalized', 'Status' => 'c_status',
  'Auto-Confirm' => 'c_auto_confirm',
);
$skip = array('Processed', 'Finalized-Tstamp', 'URI', 'Name', 'CID');
$labels = array_merge(array_keys($map), $skip);
usort($labels, function ($a, $b) { return strlen($b) - strlen($a); });   // longest first: Finalized-Tstamp before Finalized
$marker = '/ (' . implode('|', array_map(function ($l) { return preg_quote($l, '/'); }, $labels)) . ') changed from /';
$system_fields = array('c_at_m_conf', 'c_at_f_conf', 'c_auto_confirm');
$sys = (int) db_query("select td_val1 from dh_type_detail where td_type='COURSE-APPLICANT' and td_key='COURSE-SYSTEM-UID'")->fetchField();

// Parse one old line into array(label => array of possible (old, new) splits).
function _crl_parse($msg, $marker) {
  $parts = preg_split($marker, $msg, -1, PREG_SPLIT_DELIM_CAPTURE);
  $out = array();
  for ($i = 1; $i + 1 < count($parts); $i += 2) {
    $seg = $parts[$i + 1];
    $pos = array(); $o = 0;
    while (($p = strpos($seg, ' To ', $o)) !== FALSE) { $pos[] = $p; $o = $p + 1; }
    $cands = array();
    foreach ($pos as $p) $cands[] = array(substr($seg, 0, $p), substr($seg, $p + 4));
    $out[] = array($parts[$i], $cands, $seg);
  }
  return $out;
}

$stats = array('lines' => 0, 'with_fields' => 0, 'rows' => 0, 'ambiguous' => 0, 'resolved_chain' => 0, 'unresolved' => 0, 'no_to' => 0, 'by_field' => array(), 'system_credited' => 0);
if ($mode == 'run') {
  if (!db_table_exists('dh_course_log')) { echo "dh_course_log does not exist - run 2026-10-08-course-change-log.sql first.\n"; exit(1); }
  $del = db_query("delete from dh_course_log where crl_dh_log_id is not null")->rowCount();
  echo "deleted $del rows converted earlier\n";
}
$last_course = -1; $last_id = 0; $buffer = array();
$flush = function () use (&$buffer, $mode) {
  if ($mode == 'run' && $buffer) {
    foreach (array_chunk($buffer, 500) as $chunk) {
      $q = db_insert('dh_course_log')->fields(array('crl_course', 'crl_center', 'crl_action', 'crl_field', 'crl_old', 'crl_new', 'crl_user', 'crl_tstamp', 'crl_dh_log_id'));
      foreach ($chunk as $r) $q->values($r);
      $q->execute();
    }
  }
  $buffer = array();
};
// Process course by course (needed to resolve ambiguous splits along each field's chain of changes).
$courses = db_query("select distinct l_identifier from dh_log where l_module = 'dh_course' and l_msg like 'CHANGE %' order by l_identifier")->fetchCol();
foreach ($courses as $course) {
  $lines = db_query("select l_id, l_center, l_user, l_tstamp, l_msg from dh_log where l_module = 'dh_course' and l_identifier = :c and l_msg like 'CHANGE %' order by l_id", array(':c' => $course))->fetchAll();
  $parsed = array();                    // per line: list of [col, cands, seg]
  $chain = array();                     // col => list of [line index, segment index]
  foreach ($lines as $li => $l) {
    $stats['lines']++;
    $segs = array();
    foreach (_crl_parse($l->l_msg, $marker) as $s) {
      if (!isset($map[$s[0]])) continue;              // skipped label
      $col = $map[$s[0]];
      $segs[] = array($col, $s[1], $s[2]);
      $chain[$col][] = array($li, count($segs) - 1);
    }
    $parsed[$li] = $segs;
    if ($segs) $stats['with_fields']++;
  }
  // Resolve each field's chain from the newest change backwards.
  $current = NULL;
  foreach ($chain as $col => $refs) {
    $next_old = NULL; $have_next = FALSE;
    for ($k = count($refs) - 1; $k >= 0; $k--) {
      list($li, $si) = $refs[$k];
      $cands = $parsed[$li][$si][1];
      $choice = NULL;
      if (count($cands) == 1) $choice = $cands[0];
      elseif (count($cands) == 0) { $stats['no_to']++; $choice = array(NULL, $parsed[$li][$si][2]); }
      else {
        $stats['ambiguous']++;
        $target = NULL;
        if ($have_next) $target = $next_old;
        else {
          if ($current === NULL) $current = db_query("select * from dh_course where c_id = :c", array(':c' => $course))->fetchAssoc();
          $target = ($current && array_key_exists($col, $current)) ? (string) $current[$col] : NULL;
        }
        foreach ($cands as $c) if ($target !== NULL && $c[1] === $target) { $choice = $c; break; }
        if ($choice) $stats['resolved_chain']++;
        else { $stats['unresolved']++; $choice = array(NULL, $parsed[$li][$si][2]); }
      }
      $parsed[$li][$si][3] = $choice;
      $next_old = $choice[0]; $have_next = ($choice[0] !== NULL);
    }
  }
  foreach ($lines as $li => $l) {
    if (empty($parsed[$li])) continue;
    $action = 'update';
    foreach ($parsed[$li] as $s) if ($s[0] == 'c_deleted' && isset($s[3])) $action = ($s[3][1] === '1') ? 'delete' : 'restore';
    foreach ($parsed[$li] as $s) {
      $uid = (int) $l->l_user;
      if (in_array($s[0], $system_fields)) { if ($uid != $sys) $stats['system_credited']++; $uid = $sys; }
      $buffer[] = array((int) $course, (int) $l->l_center, $action, $s[0], $s[3][0], $s[3][1], $uid, $l->l_tstamp, (int) $l->l_id);
      $stats['rows']++;
      $stats['by_field'][$s[0]] = isset($stats['by_field'][$s[0]]) ? $stats['by_field'][$s[0]] + 1 : 1;
    }
  }
  if (count($buffer) >= 5000) $flush();
}
$flush();
arsort($stats['by_field']);
echo "mode $mode | courses " . count($courses) . " | old CHANGE lines {$stats['lines']} | lines with kept fields {$stats['with_fields']} | rows " . ($mode == 'run' ? 'written' : 'that would be written') . " {$stats['rows']}\n";
echo "values containing ' To ': {$stats['ambiguous']} (decided by the field's next change / current value: {$stats['resolved_chain']}, undecided - kept whole: {$stats['unresolved']}) | segments with no ' To ': {$stats['no_to']}\n";
echo "AT-count / Auto-Confirm rows re-credited to System: {$stats['system_credited']}\n";
echo "rows by field: " . json_encode($stats['by_field']) . "\n";
if ($mode == 'run') echo "dh_course_log now holds " . db_query("select count(*) from dh_course_log where crl_dh_log_id is not null")->fetchField() . " converted rows\n";
