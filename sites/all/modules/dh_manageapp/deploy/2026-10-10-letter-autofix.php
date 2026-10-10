<?php
// ============================================================================
// One-time auto-fix of merge-field mistakes in the centres' existing letters (user,
// 2026-10-10: "auto-fix existing letters - yes"). The centres' letter fields (Manage
// Fields) are not changed (user: "don't change existing merge fields in center"); a
// mistake inside one shows on the letters list for the centre to fix. Applies only the
// fixes the letter editor offers in one click ("Fix ... for me"):
//   - wrong capital letters ([firstname] -> [FirstName]), spaces inside ([First Name]),
//     one letter off or two letters swapped ([CentrName], [Satrt Date]),
//   - extra brackets ([[FirstName]]), a bracket closed too late ([EndDate 1.00pm]),
//   - a combined field whose outer text is mistyped ([time-[CourseType]]),
//   - hidden formatting inside a field.
// Everything else (unknown fields, missing letter fields) is left for the centre, which
// sees it on its letters list. The template (centre 0) is done by
// 2026-10-09-letter-template-fix.php. Needs inc/letter-check.inc and the backup table
// (2026-10-10-letter-fix-backup.sql).
//
// Run from the Drupal root on the command line (php7.3 on prod). Shows what it would do;
// writes only with --apply:
//     php7.3 sites/all/modules/dh_manageapp/deploy/2026-10-10-letter-autofix.php [--apply]
// Every changed column goes to dh_letter_fix_backup first (before + after). Re-runnable:
// a fixed letter has nothing left to fix.
// ============================================================================
if (php_sapi_name() !== 'cli') { echo "Command line only.\n"; exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1'; $_SERVER['HTTP_HOST'] = 'localhost'; $_SERVER['SCRIPT_NAME'] = '/index.php';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
$apply = in_array('--apply', $argv);
if (!function_exists('dh_letter_check')) { echo "ERROR: inc/letter-check.inc is not deployed\n"; exit(1); }
if ($apply && !db_table_exists('dh_letter_fix_backup')) { echo "ERROR: run 2026-10-10-letter-fix-backup.sql first\n"; exit(1); }

$uid = _dh_email_system_uid();
$now = date('Y-m-d H:i:s');
$levels = array('error', 'fixed');
$backup = function ($table, $row, $centre, $column, $before, $after) use ($now) {
  db_insert('dh_letter_fix_backup')->fields(array('b_table' => $table, 'b_row' => $row, 'b_center' => $centre,
    'b_column' => $column, 'b_before' => $before, 'b_after' => $after, 'b_at' => $now))->execute();
};

// 1. Letters
$n_letters = 0; $n_fixes = 0; $centres = array(); $skipped_long = 0;
$letters = db_query("select l_id, l_center, l_event, l_course_type, l_subject, l_body, l_sms from dh_letter where l_deleted = 0 and l_center <> 0 order by l_id")->fetchAll(PDO::FETCH_ASSOC);
foreach ($letters as $L) {
  $problems = dh_letter_check($L['l_center'], $L);
  $new = array();
  foreach (array('subject' => 'l_subject', 'body' => 'l_body', 'sms' => 'l_sms') as $part => $k) {
    $v = dh_letter_apply_fixes((string) $L[$k], $problems, $part, $levels);
    if ($v === (string) $L[$k])
      continue;
    if ($k == 'l_subject' && drupal_strlen($v) > 150) { $skipped_long++; continue; } // l_subject is varchar(150)
    $new[$k] = $v;
  }
  if (!$new)
    continue;
  foreach ($problems as $p)
    if ($p['fix'] !== NULL && in_array($p['level'], $levels) && in_array($p['part'], array('subject', 'body', 'sms')))
      $n_fixes++;
  $n_letters++;
  $centres[$L['l_center']] = 1;
  if ($apply) {
    $txn = db_transaction();
    foreach ($new as $k => $v)
      $backup('dh_letter', $L['l_id'], $L['l_center'], $k, (string) $L[$k], $v);
    db_update('dh_letter')->fields($new + array('l_updated' => $now, 'l_updated_by' => $uid))->condition('l_id', $L['l_id'])->execute();
    unset($txn);
  }
}
printf("letters: %s %d letters in %d centres (%d fixes)%s\n", $apply ? 'fixed' : 'would fix', $n_letters, count($centres), $n_fixes,
  $skipped_long ? "; subject left as is in $skipped_long (would be over 150 characters)" : '');

// 2. What is left for the centres (shown on their letters list)
drupal_static_reset();
$left = 0; $lcentres = array();
foreach (db_query("select l_id, l_center, l_event, l_course_type, l_subject, l_body, l_sms from dh_letter where l_deleted = 0 and l_center <> 0") as $r) {
  $c = dh_letter_check_counts(dh_letter_check($r->l_center, (array) $r));
  if ($c['error'] + $c['fixed'] + $c['field']) { $left++; $lcentres[$r->l_center] = 1; }
}
printf("letters with a red label %s: %d in %d centres\n", $apply ? 'now' : 'today', $left, count($lcentres));
echo $apply ? "done (backup: dh_letter_fix_backup, b_at = '$now')\n" : "preview only: run with --apply to fix\n";
