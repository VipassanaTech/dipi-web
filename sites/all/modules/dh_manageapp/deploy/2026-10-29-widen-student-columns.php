<?php
// ============================================================================
// Course finalize copies each attended applicant into dh_student, dh_student_course and
// dh_student_course_input. Some of those columns are shorter than the application columns
// they copy from (address, mobile, company, occupation, emergency contact, room...). The
// database refuses the whole row when text does not fit, so finalize skipped that student:
// no student or course record and no Attended email.
//
// This script widens every such column to the size of the column it is copied from.
// It reads both sides from the database it runs on, so it works on local and prod even
// though their columns differ. Widening keeps every value as it is.
//
// Run from the Drupal root, on the command line only:
//     sudo -u www-data php7.3 sites/all/modules/dh_manageapp/deploy/2026-10-29-widen-student-columns.php
//         -> dry run: shows what would change and whether a table has to be rebuilt
//     sudo -u www-data php7.3 sites/all/modules/dh_manageapp/deploy/2026-10-29-widen-student-columns.php --apply
// Re-runnable: columns that are already wide enough are left alone.
//
// Most changes are instant. Taking a column past 63 characters (utf8mb4) makes the database
// rebuild the table: reads keep working, saves to that table wait until it is done. Run
// --apply at a quiet time; it refuses to start while a course finalize is running.
// ============================================================================
if (php_sapi_name() !== 'cli') { echo "Command line only.\n"; exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1'; $_SERVER['HTTP_HOST'] = 'localhost'; $_SERVER['SCRIPT_NAME'] = '/index.php';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

$apply = in_array('--apply', $argv);
// Tables to change; a test run can point them at copies, e.g. --table=dh_student:tmp_copy
$tables = array('dh_student' => 'dh_student', 'dh_student_course' => 'dh_student_course', 'dh_student_course_input' => 'dh_student_course_input');
foreach ($argv as $a)
  if (preg_match('/^--table=(\w+):(\w+)$/', $a, $m) && isset($tables[$m[1]])) $tables[$m[1]] = $m[2];
// --only=dh_student limits the run to one of the three tables
$only = '';
foreach ($argv as $a) if (preg_match('/^--only=(\w+)$/', $a, $m) && isset($tables[$m[1]])) $only = $m[1];

// The columns finalize_course() (inc/course.inc) writes, and where each value comes from.
$sources = array('dh_applicant', 'dh_applicant_course', 'dh_applicant_attended', 'dh_applicant_extra');
$map = array();
// dh_student: every column except the ones finalize sets itself, copied from the a_ column
foreach (db_query("desc {$tables['dh_student']}")->fetchCol() as $c)
  if (!in_array($c, array('s_id', 's_updated', 's_created', 's_created_by', 's_updated_by', 's_first')))
    $map['dh_student'][$c] = array(str_replace('s_', 'a_', $c));
$map['dh_student_course'] = array(
  'sc_group' => array('aa_group'), 'sc_acco' => array('aa_section', 'aa_acco'),   // section-room
  'sc_laundry' => array('aa_laundry'), 'sc_valuable' => array('aa_valuable'), 'sc_cell' => array('aa_cell'),
  'sc_comment' => array('aa_comment'), 'sc_desc_physical' => array('ae_desc_physical'), 'sc_desc_mental' => array('ae_desc_mental'),
  'sc_addiction_past' => array('ae_desc_addiction_past'), 'sc_addiction_current' => array('ae_desc_addiction_current'),
  'sc_desc_other_technique' => array('ae_desc_other_technique'), 'sc_desc_medication' => array('ae_desc_medication'),
  'sc_teach_other_details' => array('ae_teach_other_details'));
$map['dh_student_course_input'] = array(
  'sci_first_location_str' => array('ac_first_teacher_str'), 'sci_first_teacher_str' => array('ac_first_teacher_str'));

function column_info($table) {
  $info = array();
  foreach (db_query("select column_name c, data_type d, character_maximum_length len, character_set_name cs from information_schema.columns where table_schema = database() and table_name = :t", array(':t' => $table)) as $o)
    $info[$o->c] = $o;
  return $info;
}
$src = array();
foreach ($sources as $t) $src += column_info($t);
$texts = array('tinytext' => 1, 'text' => 2, 'mediumtext' => 3, 'longtext' => 4);
$bytes_per_char = array('utf8mb4' => 4, 'utf8mb3' => 3, 'utf8' => 3);

$failed = false;
foreach ($map as $logical => $cols) {
  if ($only && $logical != $only) continue;
  $table = $tables[$logical];
  $info = column_info($table);
  $create = db_query("show create table $table")->fetchAssoc();
  $lines = explode("\n", $create['Create Table']);
  $row_format = db_query("select row_format from information_schema.tables where table_schema = database() and table_name = :t", array(':t' => $table))->fetchField();
  $mods = array(); $rebuild = false;
  foreach ($cols as $col => $from) {
    if (!isset($info[$col])) continue;
    $to = $info[$col];
    if (!in_array($to->d, array('varchar', 'char')) && !isset($texts[$to->d])) continue;   // numbers, dates: nothing to widen
    $need = 0; $need_text = ''; $ok = true;
    foreach ($from as $s) {
      if (!isset($src[$s])) { $ok = false; break; }
      if (in_array($src[$s]->d, array('varchar', 'char'))) $need += (int) $src[$s]->len;
      elseif (isset($texts[$src[$s]->d])) $need_text = $src[$s]->d;
      else { $ok = false; break; }
    }
    if (!$ok) continue;
    $need += count($from) - 1;   // the '-' between section and room
    $new = '';
    if ($need_text) {
      if (!isset($texts[$to->d]) || $texts[$to->d] < $texts[$need_text]) $new = $need_text;
    }
    elseif (in_array($to->d, array('varchar', 'char')) && $to->len < $need) $new = "varchar($need)";
    if (!$new) continue;
    // Keep the column exactly as defined (NULL, default, collation, comment); change only the type.
    // Drupal runs MySQL in ANSI mode, so the definition names the column "col" rather than `col`.
    $line = null;
    $name = '[`"]' . preg_quote($col, '/') . '[`"]';
    foreach ($lines as $l) if (preg_match("/^$name /", trim($l))) $line = rtrim(trim($l), ',');
    if (!$line || !preg_match("/^$name (varchar|char|tinytext|text|mediumtext|longtext)(\(\d+\))?( |$)/", $line)) {
      echo "  !! $table.$col: could not read its definition, left alone\n"; $failed = true; continue;
    }
    $mods[$col] = 'MODIFY ' . preg_replace("/^$name (varchar|char|tinytext|text|mediumtext|longtext)(\(\d+\))?/", "`$col` $new", $line);
    // Instant only when a varchar stays on the same side of 255 bytes (any length on REDUNDANT rows).
    $bpc = isset($bytes_per_char[$to->cs]) ? $bytes_per_char[$to->cs] : 4;
    $instant = ($to->d == 'varchar' && strpos($new, 'varchar') === 0 &&
      (strtoupper($row_format) == 'REDUNDANT' || (($to->len * $bpc > 255) == ((int) substr($new, 8) * $bpc > 255))));
    if (!$instant) $rebuild = true;
    $was = in_array($to->d, array('varchar', 'char')) ? "{$to->d}({$to->len})" : $to->d;
    echo "  $table.$col: $was -> $new" . ($instant ? '' : '  (needs a table rebuild)') . "\n";
  }
  if (!$mods) { echo "$table: nothing to widen\n"; continue; }

  $mb = round(db_query("select (data_length + index_length) / 1048576 from information_schema.tables where table_schema = database() and table_name = :t", array(':t' => $table))->fetchField());
  echo "$table: " . count($mods) . " column(s), " . ($rebuild ? "REBUILD of about $mb MB" : 'instant') . "\n";
  $sql = "ALTER TABLE $table " . implode(', ', $mods) . ', ' . ($rebuild ? 'ALGORITHM=COPY, LOCK=SHARED' : 'ALGORITHM=INSTANT');
  if (!$apply) { echo "  $sql\n"; continue; }

  // Do not start while a course finalize is writing these tables.
  $running = array();
  exec('pgrep -f "action.php [0-9]+ Finalize"', $running);
  if ($running) { echo "STOP: a course finalize is running (pid " . implode(',', $running) . "). Try again later.\n"; exit(1); }

  $before = db_query("select count(*) from $table")->fetchField();
  // Give up after 15 s if another query holds the table, instead of making every page wait behind us.
  db_query("SET SESSION lock_wait_timeout = 15");
  // Drupal's session mode refuses dates like 0000-00-00, and dh_student's own s_updated default is
  // one, so the database would refuse to rebuild the table. Keep strict mode (anything odd stops
  // the change rather than altering a value) but allow the zero dates already in the table.
  db_query("SET SESSION sql_mode = 'STRICT_ALL_TABLES,NO_ENGINE_SUBSTITUTION'");
  $t0 = microtime(true);
  try { db_query($sql); }
  catch (PDOException $e) { echo "FAILED on $table, nothing changed in it: " . $e->getMessage() . "\n"; exit(1); }
  $secs = round(microtime(true) - $t0, 1);
  $after = db_query("select count(*) from $table")->fetchField();
  $now = column_info($table);
  $bad = array();
  foreach ($mods as $col => $m)
    if (stripos($m, "`$col` " . (in_array($now[$col]->d, array('varchar', 'char')) ? "{$now[$col]->d}({$now[$col]->len})" : $now[$col]->d)) === false) $bad[] = $col;
  echo "$table: done in {$secs}s, rows $before -> $after" . ($bad ? ', NOT WIDENED: ' . implode(',', $bad) : ', all columns widened') . "\n";
  if ($before != $after || $bad) $failed = true;
}
if (!$apply) echo "Dry run only. Add --apply to make the change.\n";
exit($failed ? 1 : 0);
