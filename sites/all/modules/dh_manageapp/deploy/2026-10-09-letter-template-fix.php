<?php
// ============================================================================
// Merge-field fixes in the new-centre template (centre 0), which every new centre copies
// (centre.inc copies its letters and letter fields). Needs inc/letter-check.inc deployed.
//
//  1. Template letters: applies the checker's one-click fixes (e.g. "Dear [firstname]
//     [lastname]" -> "Dear [FirstName] [LastName]") and removes hidden formatting.
//     Existing letter fields are never changed, only missing ones are added (step 2).
//  2. Template letter fields: adds the fields that combined fields such as
//     [Time-[CourseType]] need for every course type in use, so new centres get them:
//     Detail-<type> as empty (no extra paragraph, like Detail-10 Day Course), and
//     Time-/Day-<type> only where a value is given in $values below.
//
// Run from the Drupal root on the command line (php7.3 on prod):
//     php7.3 sites/all/modules/dh_manageapp/deploy/2026-10-09-letter-template-fix.php [--dry-run]
// Re-runnable: fixes only what is still wrong and adds only fields that are missing.
// ============================================================================
if (php_sapi_name() !== 'cli') { echo "Command line only.\n"; exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1'; $_SERVER['HTTP_HOST'] = 'localhost'; $_SERVER['SCRIPT_NAME'] = '/index.php';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
$dry = in_array('--dry-run', $argv);
if (!function_exists('dh_letter_check')) { echo "ERROR: inc/letter-check.inc is not deployed\n"; exit(1); }

// Values for Time-/Day- fields of course types the template has none for, as
// 'Time-1 Day Course' => '5:00 p.m.'. Left out = not added (the letter then shows the
// bracket for that course type, as today, and the centre's letter list warns about it).
$values = array(
);

$uid = _dh_email_system_uid();
$now = date('Y-m-d H:i:s');

// 1. Template letters
$fixed_letters = 0; $left = 0;
foreach (db_query("select l_id, l_event, l_course_type, l_subject, l_body, l_sms from dh_letter where l_center = 0 and l_deleted = 0") as $L) {
  $letter = (array) $L;
  $problems = dh_letter_check(0, $letter);
  $new = array();
  foreach (array('subject' => 'l_subject', 'body' => 'l_body', 'sms' => 'l_sms') as $part => $k) {
    $v = dh_letter_apply_fixes($letter[$k], $problems, $part, array('error', 'fixed'));
    if ($v !== $letter[$k])
      $new[$k] = $v;
  }
  foreach ($problems as $p)
    if ($p['level'] == 'error' && $p['fix'] === NULL)
      $left++;
  if ($new) {
    $fixed_letters++;
    echo ($dry ? "would fix" : "fixed") . " template letter {$L->l_id}: " . implode(', ', array_keys($new)) . "\n";
    if (!$dry)
      db_update('dh_letter')->fields($new + array('l_updated' => $now, 'l_updated_by' => $uid))->condition('l_id', $L->l_id)->condition('l_center', 0)->execute();
  }
}
echo "template letters fixed: $fixed_letters; mistakes needing a person: $left\n";

// 2. Template letter fields needed by combined fields (course types in use anywhere in the last year)
drupal_static_reset('dh_letter_known_fields');
$need = array();
foreach (db_query("select l_id, l_event, l_course_type, l_subject, l_body, l_sms from dh_letter where l_center = 0 and l_deleted = 0") as $L)
  foreach (dh_letter_check(0, (array) $L) as $p)
    if ($p['level'] == 'warning' && !empty($p['missing']))
      foreach ($p['missing'] as $name)
        $need[$name] = 1;
ksort($need);
$added = 0; $skipped = array();
foreach (array_keys($need) as $name) {
  if (isset($values[$name]))
    $value = $values[$name];
  elseif (strpos($name, 'Detail-') === 0)
    $value = ' ';
  else {
    $skipped[] = $name;
    continue;
  }
  $exists = db_query("select count(*) from dh_letter_fields where lf_center = 0 and lf_deleted = 0 and lf_name = :n", array(':n' => $name))->fetchField();
  if ($exists)
    continue;
  $added++;
  echo ($dry ? "would add" : "added") . " template field [$name]\n";
  if (!$dry)
    db_insert('dh_letter_fields')->fields(array('lf_center' => 0, 'lf_name' => $name, 'lf_value' => $value, 'lf_deleted' => 0))->execute();
}
echo "template fields added: $added\n";
if ($skipped)
  echo "no value given, not added (" . count($skipped) . "): " . implode(', ', $skipped) . "\n";
echo $dry ? "dry run: nothing changed\n" : "done\n";
