<?php
// Back-fill student records of finalized courses through the same sync as finalize. NEVER sends email.
//   php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-backfill-student-records.php --batch=3            dry run, per course
//   php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-backfill-student-records.php --batch=3 --apply    do it (prints the batch id)
//   php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-backfill-student-records.php --courses=60099,62604   named courses (dry run)
//   php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-backfill-student-records.php --undo=<batch id>     what undo would do; --apply undoes
// --limit=N runs only the first N courses; --no-files skips the photo and PDF copies (dry run and apply; local tests).
// A dry run counts the photos and PDFs (S3 heads only). The tool stops after the first course with a file failure
// or an error that stopped the sync, and exits 1; it also exits 1 when any course had errors or an undo row failed.
// A batch skips courses that already carry a back-fill Change Log row (not undone); name them with --courses to run again.
// A course whose finalize is running (lock dh_finalize_<id>) is skipped and listed at the end.
// An apply dates the attended students as emailed (a_attended_mail, no email sent), so a later Finalize again never emails them.
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
// One hour at a time: it is extended before every course, so a long run keeps it and a killed run frees it within the hour.
if (!lock_acquire('dh_student_backfill', 3600)) { echo "Another back-fill is running.\n"; exit(1); }
if (!empty($o['undo']))
{
  $n = dh_backfill_undo($o['undo'], $o['apply']);
  $x = array();
  foreach ($n as $k => $v) if ($k[0] !== '_') $x[$k] = $v;
  echo ($o['apply'] ? 'undo of' : 'undo would reverse') . " {$o['undo']}, log rows per action: " . json_encode($x) . "\n";
  $code = 0;
  if ($o['apply'])
  {
    echo "  rows undone {$n['_undone']}, skipped {$n['_skipped']} (changed since, row gone, or a guard kept it), failed {$n['_failed']}; "
      . count($n['_courses']) . " courses got a Change Log note\n";
    if ($n['_failed'])
    {
      $ids = $n['_failed_ids'];
      echo '  failed log rows (sl_id): ' . implode(',', array_slice($ids, 0, 20)) . (count($ids) > 20 ? ' and ' . (count($ids) - 20) . ' more' : '') . "\n";
      $code = 1;
    }
  }
  lock_release('dh_student_backfill');
  exit($code);
}
// Email is off in the sync calls below, in code. A dry run only counts files; an apply copies them inside S3.
$files = $o['nofiles'] ? 'none' : ($o['apply'] ? 's3copy' : 'count');
$courses = !empty($o['courses']) ? $o['courses'] : (!empty($o['batch']) ? dh_backfill_courses($o['batch']) : array());
if (!empty($o['limit'])) $courses = array_slice($courses, 0, $o['limit']);
if (!$courses) { echo "No courses. Use --batch=1..4 or --courses=id,id\n"; lock_release('dh_student_backfill'); exit(1); }
$batch = 'bf-' . (!empty($o['batch']) ? 'b' . $o['batch'] : 'list') . '-' . date('Ymd-His');     // seconds, so two runs never share an id
echo ($o['apply'] ? "APPLY, batch id $batch" : 'DRY RUN') . ': ' . count($courses) . " courses, email OFF, files $files\n";
$keys = array('students', 'added', 'updated', 'removed', 'removed_kept', 'copies', 'students_removed', 'files_copied', 'files_missing', 'files_failed', 'bytes', 'errors');
$tot = array_fill_keys($keys, 0); $failed = array(); $busy = array(); $stop = ''; $t0 = microtime(TRUE);
foreach ($courses as $i => $c)
{
  if (!lock_acquire('dh_student_backfill', 3600)) { $stop = 'the lock was lost (another run took over); nothing more was done'; break; }
  // The course's finalize lock (finalize_course() holds it too): never sync a course while a finalize of it runs. No waiting.
  if (!lock_acquire(dh_finalize_lock_name($c), 3600)) { echo "  course $c busy (finalize running), skipped\n"; $busy[] = $c; continue; }
  try
  {
    $info = db_query('select c_center, c_start from dh_course where c_id = :c and c_finalized = 1', array(':c' => $c))->fetchAssoc();
    if (!$info) { echo "  course $c: not finalized, skipped\n"; continue; }
    if ($o['apply'])
    {
      try
      {
        // mark_mailed: the attended students are dated as emailed (no email is sent), so a later Finalize again never emails them.
        $s = dh_course_sync($c, array('mail' => FALSE, 'mark_mailed' => TRUE, 'files' => $files, 'batch' => $batch, 'uid' => dh_system_uid()));
        dh_course_log_note($c, 'finalize_summary', "Back-fill $batch (no emails): " . dh_course_sync_summary_text($s), dh_system_uid());
      }
      catch (Throwable $e)
      {
        // Only the course id, the exception class and (database errors) the SQLSTATE go to the screen: a message can echo an ID number or a name.
        $code = ($e instanceof PDOException) ? ' (SQLSTATE ' . (isset($e->errorInfo[0]) ? $e->errorInfo[0] : $e->getCode()) . ')' : '';
        echo "  course $c: STOPPED by an error: " . get_class($e) . $code . "; the full message is in the Recent log messages (type StudentBackfill)\n";
        try { watchdog('StudentBackfill', 'Back-fill @b, course @c: @m', array('@b' => $batch, '@c' => $c, '@m' => $e->getMessage()), WATCHDOG_ERROR); }
        catch (Throwable $w) { }                                  // the database may be gone: the STOP line and the lock release must still happen
        $stop = "course $c stopped with an error; what was done can be undone with --undo=$batch; fix the cause, then run again with --courses=$c";
        break;
      }
    }
    else
      $s = dh_backfill_dry_run($c, $files);
    $s += array_fill_keys($keys, 0);
    foreach ($keys as $k) $tot[$k] += $s[$k];
    if ($s['errors']) $failed[] = $c;
    printf("  %d/%d course %d (centre %d, %s): %d students, +%d, =%d, -%d (kept %d), copies -%d, students -%d, files %d (%.1f MB), missing %d, failed %d%s\n",
      $i + 1, count($courses), $c, $info['c_center'], substr($info['c_start'], 0, 10), $s['students'], $s['added'], $s['updated'], $s['removed'], $s['removed_kept'],
      $s['copies'], $s['students_removed'], $s['files_copied'], $s['bytes'] / 1048576, $s['files_missing'], $s['files_failed'], $s['errors'] ? ", ERRORS {$s['errors']}" : '');
    if ($s['files_failed'] > 0) { $stop = "course $c had {$s['files_failed']} file failures (S3 settings or permissions?); fix the cause, then run again with --courses=$c"; break; }
  }
  finally
  {
    try { lock_release(dh_finalize_lock_name($c)); } catch (Throwable $w) { }
  }
  usleep(200000);
}
echo 'TOTAL: ' . json_encode($tot) . ', ' . round(microtime(TRUE) - $t0) . "s\n";
if ($tot['removed_kept']) echo "  kept: {$tot['removed_kept']} removed course records belong to students who keep other courses (worth a look: one person may be two students)\n";
if ($failed) echo "Courses with errors (see the 'Finalization Error' and 'StudentBackfill' logs): " . implode(',', $failed) . "\n";
if ($busy) echo 'Courses skipped because a finalize was running (run them again later with --courses): ' . implode(',', $busy) . "\n";
if ($stop) echo "STOP: $stop\n";
try { lock_release('dh_student_backfill'); } catch (Throwable $w) { }      // (a lost database frees it when the hour is up)
exit(($stop || $failed) ? 1 : 0);
