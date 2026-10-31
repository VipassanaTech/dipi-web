<?php
// Restore of the 2020 migrated students (design and plan: private Claude Docs). NEVER sends email; prints counts and numbers only.
//   php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-restore-students.php                       dry run, every old number
//   php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-restore-students.php --apply               the real run (prints the batch id)
//   php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-restore-students.php --numbers=82481,90210  only these numbers
//   php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-restore-students.php --limit=2000          only the first 2000 numbers (name order)
//   php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-restore-students.php --undo=<batch id>     what the undo would reverse; add --apply to undo
// A run that stops can be run again: numbers already restored or merged count as done. Holds the back-fill lock.
if (php_sapi_name() !== 'cli') { echo "Command line only.\n"; exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1'; $_SERVER['HTTP_HOST'] = 'localhost'; $_SERVER['SCRIPT_NAME'] = '/index.php';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

$o = array('apply' => in_array('--apply', $argv), 'numbers' => array(), 'limit' => 0);
foreach (array_slice($argv, 1) as $a)
{
  if ($a === '--apply') continue;
  elseif (preg_match('/^--numbers=([\d,]+)$/', $a, $m)) $o['numbers'] = array_values(array_filter(array_map('intval', explode(',', $m[1]))));
  elseif (preg_match('/^--limit=([1-9]\d*)$/', $a, $m)) $o['limit'] = (int) $m[1];
  elseif (preg_match('/^--undo=(restore-\d{8}-\d{6})$/', $a, $m)) $o['undo'] = $m[1];
  else { echo "Unknown option: $a (a mistyped option could run everything, so nothing was done)\n"; exit(1); }
}
if (!lock_acquire('dh_student_backfill', 3600)) { echo "A back-fill or restore is running.\n"; exit(1); }
if (!empty($o['undo']))
{
  $n = dh_backfill_undo($o['undo'], $o['apply']);
  $x = array();
  foreach ($n as $k => $v) if ($k[0] !== '_') $x[$k] = $v;
  echo ($o['apply'] ? 'undo of' : 'undo would reverse') . " {$o['undo']}, log rows per action: " . json_encode($x) . "\n";
  if ($o['apply'])
    echo "  rows undone {$n['_undone']}, skipped {$n['_skipped']}, failed {$n['_failed']}" . ($n['_failed'] ? ' (sl_id: ' . implode(',', array_slice($n['_failed_ids'], 0, 20)) . ')' : '') . "\n";
  lock_release('dh_student_backfill');
  exit(($o['apply'] && $n['_failed']) ? 1 : 0);
}
if (!db_table_exists('dh_restore_source') || !db_query('select 1 from dh_restore_source limit 1')->fetchField())
{
  echo "dh_restore_source is missing or empty: fill it first (deploy/2026-11-restore-source-fill.sql).\n";
  lock_release('dh_student_backfill');
  exit(1);
}
$batch = $o['apply'] ? 'restore-' . date('Ymd-His') : 'restore-dry-run';
$start = microtime(TRUE);
echo ($o['apply'] ? "APPLY, batch $batch" : 'DRY RUN (each chunk rolled back)') . "\n";
$t = dh_restore_run($o['apply'], array('batch' => $batch, 'numbers' => $o['numbers'], 'limit' => $o['limit'], 'progress' => function ($t) use ($start) {
  lock_acquire('dh_student_backfill', 3600);                     // extends the lock this run holds
  $done = $t['restored'] + $t['merged'] + $t['unclear'] + $t['done'] + $t['failed'];
  echo "  $done / {$t['numbers']} numbers, " . round(microtime(TRUE) - $start) . "s\n";
}));
echo "numbers: {$t['numbers']}\n";
foreach (array('restored', 'merged', 'unclear', 'done', 'failed', 'rooms', 'seats', 'copies') as $k) echo "  $k: {$t[$k]}\n";
echo '  merged by: ' . json_encode($t['how']) . "\n";
if ($t['unclear_ids']) echo '  unclear numbers: ' . implode(',', $t['unclear_ids']) . "\n";
if ($t['failed_ids']) echo '  failed numbers: ' . implode(',', $t['failed_ids']) . " (messages in the Drupal log, type StudentRestore)\n";
echo '  time: ' . round(microtime(TRUE) - $start) . "s\n";
if ($o['apply']) echo "Undo: php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-restore-students.php --undo=$batch --apply\n";
lock_release('dh_student_backfill');
exit($t['failed'] ? 1 : 0);
