<?php
// Back-fill student records of finalized courses through the same sync as finalize. NEVER sends email.
//   php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-backfill-student-records.php --batch=3            dry run, per course
//   php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-backfill-student-records.php --batch=3 --apply    do it (prints the batch id)
//   php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-backfill-student-records.php --courses=60099,62604   named courses (dry run)
//   php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-backfill-student-records.php --undo=<batch id>     what undo would do; --apply undoes
// --limit=N runs only the first N courses; --no-files skips the photo and PDF copies (dry run and apply; local tests).
// A dry run counts the photos and PDFs (S3 heads only). The tool stops after the first course with a file failure.
// Prints counts and course ids only. There is no option to send email.
if (php_sapi_name() !== 'cli') { echo "Command line only.\n"; exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1'; $_SERVER['HTTP_HOST'] = 'localhost'; $_SERVER['SCRIPT_NAME'] = '/index.php';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

$o = array('apply' => in_array('--apply', $argv), 'nofiles' => in_array('--no-files', $argv));
foreach (array_slice($argv, 1) as $a)
{
  if ($a === '--apply' || $a === '--no-files') continue;
  elseif (preg_match('/^--batch=([1-4])$/', $a, $m)) $o['batch'] = (int) $m[1];
  elseif (preg_match('/^--limit=([1-9]\d*)$/', $a, $m)) $o['limit'] = (int) $m[1];
  elseif (preg_match('/^--courses=([\d,]+)$/', $a, $m)) $o['courses'] = array_values(array_filter(array_map('intval', explode(',', $m[1]))));
  elseif (preg_match('/^--undo=([\w-]+)$/', $a, $m)) $o['undo'] = $m[1];
  else { echo "Unknown option: $a (a mistyped option could run a whole batch, so nothing was done)\n"; exit(1); }
}
if (!empty($o['batch']) && !empty($o['courses'])) { echo "Use --batch or --courses, not both.\n"; exit(1); }
if (!lock_acquire('dh_student_backfill', 6 * 3600)) { echo "Another back-fill is running.\n"; exit(1); }
if (!empty($o['undo']))
{
  $n = dh_backfill_undo($o['undo'], $o['apply']);
  echo ($o['apply'] ? 'undone' : 'undo would reverse') . " for {$o['undo']}: " . json_encode($n) . "\n";
  lock_release('dh_student_backfill');
  exit(0);
}
// Email is off in the sync calls below, in code. A dry run only counts files; an apply copies them inside S3.
$files = $o['nofiles'] ? 'none' : ($o['apply'] ? 's3copy' : 'count');
$courses = !empty($o['courses']) ? $o['courses'] : (!empty($o['batch']) ? dh_backfill_courses($o['batch']) : array());
if (!empty($o['limit'])) $courses = array_slice($courses, 0, $o['limit']);
if (!$courses) { echo "No courses. Use --batch=1..4 or --courses=id,id\n"; lock_release('dh_student_backfill'); exit(1); }
$batch = 'bf-' . (!empty($o['batch']) ? 'b' . $o['batch'] : 'list') . '-' . date('Ymd-Hi');
echo ($o['apply'] ? "APPLY, batch id $batch" : 'DRY RUN') . ': ' . count($courses) . " courses, email OFF, files $files\n";
$keys = array('students', 'added', 'updated', 'removed', 'copies', 'students_removed', 'files_copied', 'files_missing', 'files_failed', 'bytes', 'errors');
$tot = array_fill_keys($keys, 0); $failed = array(); $stop = 0; $t0 = microtime(TRUE);
foreach ($courses as $i => $c)
{
  $info = db_query('select c_center, c_start from dh_course where c_id = :c and c_finalized = 1', array(':c' => $c))->fetchAssoc();
  if (!$info) { echo "  course $c: not finalized, skipped\n"; continue; }
  if ($o['apply'])
  {
    $s = dh_course_sync($c, array('mail' => FALSE, 'files' => $files, 'batch' => $batch, 'uid' => dh_system_uid()));
    dh_course_log_note($c, 'finalize_summary', "Back-fill $batch (no emails): " . dh_course_sync_summary_text($s), dh_system_uid());
  }
  else
    $s = dh_backfill_dry_run($c, $files);
  $s += array_fill_keys($keys, 0);
  foreach ($keys as $k) $tot[$k] += $s[$k];
  if ($s['errors']) $failed[] = $c;
  printf("  %d/%d course %d (centre %d, %s): %d students, +%d, =%d, -%d, copies -%d, students -%d, files %d (%.1f MB), missing %d, failed %d%s\n",
    $i + 1, count($courses), $c, $info['c_center'], substr($info['c_start'], 0, 10), $s['students'], $s['added'], $s['updated'], $s['removed'],
    $s['copies'], $s['students_removed'], $s['files_copied'], $s['bytes'] / 1048576, $s['files_missing'], $s['files_failed'], $s['errors'] ? ", ERRORS {$s['errors']}" : '');
  if ($s['files_failed'] > 0) { $stop = $c; $stopn = $s['files_failed']; break; }
  usleep(200000);
}
echo 'TOTAL: ' . json_encode($tot) . ', ' . round(microtime(TRUE) - $t0) . "s\n";
if ($failed) echo 'Courses with errors' . ($o['apply'] ? ' (see the Finalization Error log)' : '') . ': ' . implode(',', $failed) . "\n";
if ($stop)
{
  echo "STOP: course $stop had $stopn file failures (S3 settings or permissions?); fix and run again\n";
  lock_release('dh_student_backfill');
  exit(1);
}
lock_release('dh_student_backfill');
